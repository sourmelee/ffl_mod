// Source-only Japanese text inventory. Does not modify game or mod code.
import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import { fileURLToPath } from 'node:url';

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
if (fs.existsSync(path.join(root, 'src/localization.h'))) {
  throw Error('This baseline extractor would replace the reviewed catalog after resource migration. Edit translations/japanese_strings.csv and run tools/generate_localization.mjs instead.');
}
const japanese = /[\u3040-\u30ff\u3400-\u9fff\uff66-\uff9f]/u;
const rows = new Map();
const walk = dir => fs.readdirSync(dir, { withFileTypes: true }).flatMap(e =>
  e.isDirectory() ? walk(path.join(dir, e.name)) : [path.join(dir, e.name)]);

function add(file, source, pos, text, category, encoding, symbol = '') {
  if (!japanese.test(text)) return;
  const key = category + '\0' + text;
  const row = rows.get(key) ?? {
    id: category.replaceAll('_', '-') + '-' + crypto.createHash('sha256').update(key).digest('hex').slice(0, 10),
    category, japanese: text, english: '', occurrences: [],
  };
  row.occurrences.push({ file, line: source.slice(0, pos).split('\n').length, encoding, symbol });
  rows.set(key, row);
}

function cBytes(literal) {
  const bytes = [];
  const text = literal.slice(1, -1);
  for (let i = 0; i < text.length;) {
    if (text[i] !== '\\') {
      const cp = String.fromCodePoint(text.codePointAt(i));
      bytes.push(...Buffer.from(cp)); i += cp.length; continue;
    }
    i++;
    if (text[i] === 'x') {
      const m = text.slice(i + 1).match(/^[0-9a-f]+/i);
      if (!m) throw Error('Invalid C hex escape');
      const value = parseInt(m[0], 16);
      if (value > 255) throw Error('C hex escape exceeds one byte');
      bytes.push(value); i += 1 + m[0].length;
    } else if (/[0-7]/.test(text[i] ?? '')) {
      const m = text.slice(i).match(/^[0-7]{1,3}/)[0];
      bytes.push(parseInt(m, 8)); i += m.length;
    } else if (text[i] === '\n') { i++; }
    else {
      bytes.push(({ n: 10, r: 13, t: 9, b: 8, f: 12, v: 11, a: 7 })[text[i]] ?? text.charCodeAt(i)); i++;
    }
  }
  return bytes;
}

function nativeStrings(file, source) {
  // Tokenize comments and character constants too, so their quotes cannot become entries.
  const token = /\/\*[\s\S]*?\*\/|\/\/[^\n]*|"(?:\\[\s\S]|[^"\\])*"|'(?:\\[\s\S]|[^'\\])*'/g;
  const strings = [...source.matchAll(token)].filter(m => m[0][0] === '"');
  for (let i = 0; i < strings.length; i++) {
    const first = strings[i]; let last = first; let bytes = cBytes(first[0]);
    while (i + 1 < strings.length) {
      const next = strings[i + 1];
      const between = source.slice(last.index + last[0].length, next.index)
        .replace(/\/\*[\s\S]*?\*\/|\/\/[^\n]*/g, '').trim();
      if (between) break;
      bytes.push(...cBytes(next[0])); last = next; i++;
    }
    let text, encoding;
    for (const enc of ['utf-8', 'shift_jis']) {
      try { text = new TextDecoder(enc, { fatal: true }).decode(Uint8Array.from(bytes)); encoding = enc; break; }
      catch { /* Try the source's other supported encoding. */ }
    }
    if (!text || !japanese.test(text)) continue;
    const before = source.slice(Math.max(0, first.index - 700), first.index);
    const declarations = [...before.matchAll(/\b(k_\w+)\s*(?:\[[^\]]*\])?\s*=\s*/g)];
    const decl = declarations.at(-1);
    const tail = decl ? before.slice(decl.index + decl[0].length) : '';
    const symbol = decl && !/[;}]/.test(tail) ? decl[1] : '';
    const line = source.slice(source.lastIndexOf('\n', first.index) + 1, source.indexOf('\n', first.index));
    const reference = /^k_slowOld|^k_mem(?:SJ|U8)$/.test(symbol) || ['魔人', 'DEBUG用'].includes(text);
    const category = /LOG[IE]\(/.test(line) ? 'diagnostic' : reference ? 'internal_reference' : 'in_game';
    add(file, source, first.index, text, category, encoding, symbol);
  }
}

function stringEnd(source, start, kotlin) {
  for (let i = start + 1; i < source.length; i++) {
    if (source[i] === '\\') { i++; continue; }
    if (source[i] === '"') return i + 1;
    if (kotlin && source.slice(i, i + 2) === '${') {
      let depth = 1; i += 2;
      for (; i < source.length && depth; i++) {
        if (source[i] === '"') { i = stringEnd(source, i, true) - 1; continue; }
        if (source[i] === '{') depth++;
        if (source[i] === '}') depth--;
      }
      i--;
    }
  }
  throw Error('Unterminated source string at ' + start);
}

function uiStrings(file, source, kotlin) {
  for (let i = 0; i < source.length;) {
    if (source.slice(i, i + 2) === '/*') { i = source.indexOf('*/', i + 2) + 2; continue; }
    if (source.slice(i, i + 2) === '//') { const end = source.indexOf('\n', i); i = end < 0 ? source.length : end; continue; }
    if (source[i] !== '"') { i++; continue; }
    const end = stringEnd(source, i, kotlin);
    const raw = source.slice(i + 1, end - 1);
    const text = raw.replace(/\\u([0-9a-f]{4})/gi, (_, h) => String.fromCharCode(parseInt(h, 16)))
      .replace(/\\([nrt"\\])/g, (_, c) => ({ n: '\n', r: '\r', t: '\t', '"': '"', '\\': '\\' })[c]);
    add(file, source, i, text, 'manager', 'utf-8'); i = end;
  }
}

const files = walk(path.join(root, 'src')).concat(walk(path.join(root, 'manager', 'app', 'src', 'main'))).sort();
for (const absolute of files) {
  const file = path.relative(root, absolute).replaceAll('\\', '/');
  if (!/\.(?:c|h|kt|java|smali|xml)$/.test(file)) continue;
  const source = fs.readFileSync(absolute, 'utf8');
  if (/\.(?:c|h)$/.test(file)) nativeStrings(file, source);
  else if (/\.(?:kt|java)$/.test(file)) uiStrings(file, source, file.endsWith('.kt'));
  else if (file.endsWith('.smali')) {
    for (const m of source.matchAll(/const-string(?:\/jumbo)?\s+\w+,\s*("(?:\\.|[^"\\])*")/g)) {
      let text; try { text = JSON.parse(m[1]); } catch { text = m[1].slice(1, -1); }
      add(file, source, m.index, text, 'save_dialog', 'utf-8');
    }
  } else {
    for (const m of source.matchAll(/android:label="([^"]*)"/g)) add(file, source, m.index, m[1], 'manager', 'utf-8', 'android:label');
  }
}

const out = path.join(root, 'translations'); fs.mkdirSync(out, { recursive: true });
const catalog = [...rows.values()].sort((a, b) => a.category.localeCompare(b.category) || a.occurrences[0].file.localeCompare(b.occurrences[0].file) || a.occurrences[0].line - b.occurrences[0].line);
const csv = value => '"' + String(value).replaceAll('"', '""') + '"';
function writeCsv(name, list) {
  const header = ['id', 'category', 'japanese', 'english', 'locations', 'symbols', 'encodings'];
  const records = list.map(r => [r.id, r.category, r.japanese, r.english,
    r.occurrences.map(o => `${o.file}:${o.line}`).join(' | '),
    [...new Set(r.occurrences.map(o => o.symbol).filter(Boolean))].join(' | '),
    [...new Set(r.occurrences.map(o => o.encoding))].join(' | ')]);
  fs.writeFileSync(path.join(out, name), '\ufeff' + [header, ...records].map(r => r.map(csv).join(',')).join('\r\n') + '\r\n');
}
// Read our quoted CSV format, including multiline fields, without dependencies.
function readCsv(source) {
  const records = []; let row = [], field = '', quoted = false;
  source = source.replace(/^\ufeff/, '');
  for (let i = 0; i < source.length; i++) {
    const c = source[i];
    if (c === '"') {
      if (quoted && source[i + 1] === '"') { field += '"'; i++; }
      else quoted = !quoted;
    } else if (!quoted && (c === ',' || c === '\n' || c === '\r')) {
      row.push(field); field = '';
      if (c !== ',') {
        records.push(row); row = [];
        if (c === '\r' && source[i + 1] === '\n') i++;
      }
    } else field += c;
  }
  if (quoted) throw Error('Unclosed CSV quote; catalog was not regenerated.');
  if (field || row.length) { row.push(field); records.push(row); }
  return records;
}
// CSV is the editable translation source; retain English cells on regeneration.
const jsonPath = path.join(out, 'catalog.json');
const existing = new Map();
for (const name of ['japanese_strings.csv', 'internal_references.csv']) {
  const location = path.join(out, name);
  if (!fs.existsSync(location)) continue;
  const [header, ...records] = readCsv(fs.readFileSync(location, 'utf8'));
  if (header[0] !== 'id' || !header.includes('english')) throw Error('Unexpected CSV header: ' + name);
  for (const record of records) existing.set(record[0], record[header.indexOf('english')] ?? '');
}
for (const row of catalog) row.english = existing.get(row.id) ?? '';
writeCsv('japanese_strings.csv', catalog.filter(r => !['internal_reference', 'diagnostic'].includes(r.category)));
writeCsv('internal_references.csv', catalog.filter(r => ['internal_reference', 'diagnostic'].includes(r.category)));
fs.writeFileSync(jsonPath, JSON.stringify(catalog, null, 2) + '\n');
for (const category of [...new Set(catalog.map(r => r.category))]) {
  console.log(`${category}: ${catalog.filter(r => r.category === category).length} unique strings`);
}
console.log('Inventory only; runtime sources unchanged. Existing CSV English cells retained by stable ID.');
