// Audit and prepare translation DATA. This does not edit native/manager/smali
// runtime sources, install an APK, or write to the original game-data folder.
import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';
import { execFileSync } from 'node:child_process';
import { fileURLToPath } from 'node:url';

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
assert.ok(!fs.existsSync(path.join(root, 'src/localization.h')), 'This audit targets the unlocalized baseline. For the integrated fork use tools/generate_localization.mjs --check and tests/localization_test.c; preserve the original audit as provenance.');
const baseArg = process.argv.indexOf('--base-data');
assert.ok(baseArg >= 0 && process.argv[baseArg + 1], 'Usage: node tools/complete_translation_review.mjs --base-data PATH');
const base = path.resolve(process.argv[baseArg + 1]);
const dir = path.join(root, 'translations', 'completion_review'); fs.mkdirSync(dir, { recursive: true });
const hash = b => crypto.createHash('sha256').update(b).digest('hex');
const key = s => hash(s).slice(0, 12);
function save(name, data) { const p = path.join(dir, name); fs.mkdirSync(path.dirname(p), { recursive: true }); fs.writeFileSync(p, data); }
function json(name, data) { save(name, JSON.stringify(data, null, 2) + '\n'); }
function parseCsv(text) {
  let row = [], cell = '', quoted = false; const out = []; text = text.replace(/^\ufeff/, '');
  for (let p = 0; p < text.length; p++) {
    const ch = text[p];
    if (ch === '"') { if (quoted && text[p + 1] === '"') { cell += '"'; p++; } else quoted = !quoted; }
    else if (!quoted && [',', '\r', '\n'].includes(ch)) {
      row.push(cell); cell = ''; if (ch !== ',') { out.push(row); row = []; if (ch === '\r' && text[p + 1] === '\n') p++; }
    } else cell += ch;
  }
  assert.ok(!quoted); if (cell || row.length) { row.push(cell); out.push(row); }
  const header = out.shift(); return { header, rows: out.map(a => Object.fromEntries(header.map((h, i) => [h, a[i] ?? '']))) };
}
function csvString(header, rows) {
  const q = s => '"' + String(s ?? '').replaceAll('"', '""') + '"';
  return '\ufeff' + [header, ...rows.map(r => header.map(h => r[h]))].map(r => r.map(q).join(',')).join('\r\n') + '\r\n';
}
function csv(name, header, rows) { save(name, csvString(header, rows)); }
const sourceCsv = parseCsv(fs.readFileSync(path.join(root, 'translations/japanese_strings.csv'), 'utf8'));
assert.equal(sourceCsv.rows.length, 110); assert.ok(sourceCsv.rows.every(r => r.english));
const drafts = JSON.parse(fs.readFileSync(path.join(root, 'translations/additional_english_drafts.json'), 'utf8'));
function tags(text) { return [...text.matchAll(/<(?:[^<>]|<[^<>]*>)*>|\$\{[^}]*\}|\$[A-Za-z_]\w*|%(?:\d+\$)?[-+0-9.]*[A-Za-z]/g)].map(m => m[0]).sort(); }
function valid(jp, en) { return !!en && JSON.stringify(tags(jp)) === JSON.stringify(tags(en)); }
for (const [jp, en] of Object.entries(drafts)) assert.ok(valid(jp, en), 'Draft placeholders: ' + jp);

// All sixteen story banks, every text AND speaker slot. Keep byte spans so
// generated DATs can preserve untouched fields and any trailing metadata exactly.
const banks = [], official = new Map(), speakerOfficial = new Map(), missing = new Map(), missingSpeakers = new Map();
const decoder = new TextDecoder('utf-8', { fatal: true });
for (let bank = 0; bank < 16; bank++) {
  const name = `msg${bank}.dat`, bytes = fs.readFileSync(path.join(base, name)), count = bytes.readUInt16BE(0), records = []; let p = 2;
  for (let id = 0; id < count; id++) {
    const slots = [];
    for (let k = 0; k < 12; k++) {
      assert.ok(p + 2 <= bytes.length); const start = p, len = bytes.readUInt16BE(p); p += 2; assert.ok(p + len < bytes.length);
      const text = decoder.decode(bytes.subarray(p, p + len)); p += len; assert.equal(bytes[p++], 0); slots.push({ text, start, end: p });
    }
    records.push(slots);
    for (const [jpSlot, enSlot, lookup, gaps] of [[0, 2, official, missing], [1, 3, speakerOfficial, missingSpeakers]]) {
      const jp = slots[jpSlot].text, en = slots[enSlot].text; if (!jp) continue;
      const ref = { bank, id, slot: enSlot, source: `${name}:${id}:${enSlot === 2 ? 'text' : 'speaker'}` };
      if (en) { const list = lookup.get(jp) ?? []; list.push({ ...ref, english: en }); lookup.set(jp, list); }
      else { const list = gaps.get(jp) ?? []; list.push(ref); gaps.set(jp, list); }
    }
  }
  banks.push({ name, bytes, count, records, parsed_end: p });
}
function translation(jp, speakers = false) {
  const look = speakers ? speakerOfficial : official;
  const candidates = (look.get(jp) ?? []).filter(r => valid(jp, r.english));
  if (candidates.length) return { english: candidates[0].english, method: 'existing_game_English_exact_Japanese_match', from: candidates[0].source,
    alternatives: [...new Set(candidates.map(r => r.english))] };
  if (drafts[jp]) return { english: drafts[jp], method: 'draft_translation', from: 'translations/additional_english_drafts.json', alternatives: [] };
  if (speakers && /^<[^ ]+>$/.test(jp)) return { english: jp, method: 'preserved_control_tag', from: 'original speaker token', alternatives: [] };
  return null;
}

// Read all indexed map event records. Only mod-related maps/events contribute
// to the core runtime patch set; the rest provide an audit of indirect references.
const initialMaps = JSON.parse(fs.readFileSync(path.join(root, 'translations/extended_asset_review/maps/index.json'), 'utf8'));
const coreMaps = new Set(initialMaps.map(m => `${m.chapter}/${m.id}`));
for (const [ch, id] of [[4, 801], [4, 811], [5, 401], [5, 819], [5, 900], [7, 9901], [15, 200]]) coreMaps.add(`${ch}/${id}`);
const references = [], moogleSites = [], scanErrors = [], mapNames = new Set([69, 70, 38]); let mapCount = 0, eventCount = 0, commandCount = 0;
function textRefs(b) {
  const rows = [], add = (id, kind, bank = 0, confidence = 'decoded_operand') => rows.push({ id, kind, table: bank === 0 ? 'story' : 'system', confidence });
  if (b[0] === 0 && b.length >= 8) add(b.readInt16BE(1), 'message', b[6] & 0x10 ? 0 : 1);
  if (b[0] === 1 && b.length >= 3) add(b.readInt16BE(1), 'caption');
  if (b[0] === 0x3b && b.length >= 6) add(b.readInt16BE(1), 'yes_no', b[3] & 1);
  if (b[0] === 0x3c && b.length >= 3) for (let i = 0; i < b[2]; i++) { assert.ok(7 + i * 4 <= b.length); add(b.readInt16BE(3 + i * 4), 'menu_choice', b[1] & 1); }
  if (b[0] === 0x4f && b.length >= 4) add(b.readInt16BE(2), 'shop_title', 0, 'shop_title_candidate');
  if (b[0] === 0x79 && b.length >= 4) add(b.readInt16BE(2), 'assigned_name');
  if (b[0] === 0xa0 && b.length >= 5) add(b.readInt16BE(3), 'script_button', b[2]);
  return rows;
}
for (const headerName of fs.readdirSync(base).filter(n => /^mpkh\d+\.dat$/.test(n)).sort()) {
  const group = Number(headerName.match(/\d+/)[0]), index = fs.readFileSync(path.join(base, headerName)); let ip = 8;
  const n = size => { const value = index.readUIntBE(ip, size); ip += size; return value; };
  for (let pack = 0; pack < index[7]; pack++) {
    const entries = n(index[4]), file = `mpk${group}_${pack}.dat`, bytes = fs.readFileSync(path.join(base, file)); let offset = 0;
    for (let i = 0; i < entries; i++) {
      const map = n(index[5]), size = n(index[6]), chunk = bytes.subarray(offset, offset + size); offset += size; mapCount++;
      try {
        assert.equal(chunk.length, size); const ep = chunk.readUInt32BE(0) + 4; assert.ok(ep < chunk.length); let p = ep, count = chunk[p++];
        if (coreMaps.has(`${group}/${map}`) && chunk.length >= 8) mapNames.add(chunk.readUInt16BE(6));
        for (let e = 0; e < count; e++) {
          assert.ok(p + 0x41 <= chunk.length); const h = chunk.subarray(p, p + 0x3f), id = chunk.readUInt16BE(p), count = chunk.readUInt16BE(p + 0x3f), commands = []; p += 0x41; eventCount++;
          for (let block = 0; block < count; block++) { assert.ok(p < chunk.length); const len = chunk[p++]; assert.ok(p + len <= chunk.length); commands.push(chunk.subarray(p, p + len)); p += len; commandCount++; }
          let moogle = false, dest = -1;
          for (const b of commands) if (b.length === 0x39 && b[0] === 0x6b && b[1] === 2 && b[2] === 9 && b[4] === 0) dest = b.readInt32BE(5);
          if (h.readUInt16BE(0x2b) === 12 && h[7] === 1 && h[8] === 0) {
            const first = commands[0];
            moogle = !!(first && [3000, 1400, 1600, 2000, 1100, 1900].includes(dest) && first[0] === 0x3d && first[3] === 1 && first.readUInt16BE(6) === 5);
            moogle ||= !commands.length && ((id === 22 && h[2] === 58 && h[3] === 25) || (id === 3 && h[2] === 6 && h[3] === 4) || (id === 4 && h[2] === 5 && h[3] === 3));
          }
          if (moogle) moogleSites.push({ group, map, event: id, dest });
          const core = coreMaps.has(`${group}/${map}`) || moogle;
          for (const [block, b] of commands.entries()) for (const ref of textRefs(b)) references.push({ group, map, event: id, block, core, opcode: b[0], ...ref });
        }
        assert.equal(p, chunk.length);
      } catch (error) { scanErrors.push({ group, map, file, error: error.message }); }
    }
    assert.equal(offset, bytes.length, file);
  }
}
// The three bundled replacement scripts reference these same six messages.
for (const bank of [7, 8, 9]) for (const id of [33, 34, 35, 36, 37, 38]) references.push({ group: bank, map: null, event: null, block: null, core: true, opcode: null, id, kind: 'bundled_moogle_replacement', table: 'story', confidence: 'bundled_script_reference' });
const scopedJapanese = new Set(), coreReferences = [];
for (const r of references.filter(r => r.core && r.table === 'story' && r.id >= 0 && r.id < banks[r.group]?.count)) {
  const record = banks[r.group].records[r.id]; if (record[0].text) { scopedJapanese.add(record[0].text); coreReferences.push(r); }
}

const all = sourceCsv.rows.map(r => ({ ...r, scope: 'mod_source', method: 'existing_draft', references: r.locations }));
const gapRows = [], remaining = [], fieldPatches = [], coreUntranslated = [];
for (const [jp, refs] of missing) {
  const tr = translation(jp), scope = scopedJapanese.has(jp) ? 'mod_related_story' : 'supplemental_base_gap';
  const row = { id: `story-gap-${key(jp)}`, category: 'story_text', japanese: jp, english: tr?.english ?? '', scope,
    method: tr?.method ?? 'not_translated_original_credit_or_unreviewed', references: refs.map(r => r.source).join(' | '), provenance: tr?.from ?? '', english_alternatives: tr ? JSON.stringify(tr.alternatives) : '' };
  gapRows.push(row);
  if (tr) all.push(row); else { remaining.push(row); if (scope === 'mod_related_story') coreUntranslated.push(row); }
  if (tr && scope === 'mod_related_story') for (const ref of refs) fieldPatches.push({ ...ref, japanese: jp, english: tr.english, reason: 'fill_empty_English_story_text', expected_english: '' });
}
const speakerRows = [];
for (const [jp, refs] of missingSpeakers) {
  const tr = translation(jp, true), core = refs.some(r => scopedJapanese.has(banks[r.bank].records[r.id][0].text));
  const row = { id: `speaker-${key(jp)}`, category: 'speaker', japanese: jp, english: tr?.english ?? '', scope: core ? 'mod_related_speaker' : 'supplemental_base_speaker', method: tr?.method ?? 'untranslated', references: refs.map(r => r.source).join(' | '), provenance: tr?.from ?? '' };
  speakerRows.push(row); if (tr) all.push(row); else if (core) coreUntranslated.push(row);
  if (tr && core) for (const ref of refs) fieldPatches.push({ ...ref, japanese: jp, english: tr.english, reason: 'fill_empty_English_speaker', expected_english: '' });
}
// The actual award is 7 JP, while the current English text still says 6.
for (const bank of [10, 13]) for (const id of [22, 26]) {
  const r = banks[bank].records[id], english = r[2].text.replace(/\b6 JP\b/, '7 JP'); assert.notEqual(english, r[2].text);
  fieldPatches.push({ bank, id, slot: 2, japanese: r[0].text, english, reason: 'match_mod_7JP_reward', expected_english: r[2].text });
  all.push({ id: `jp-award-${bank}-${id}`, category: 'JP_award', japanese: r[0].text, english, scope: 'mod_behavior_consistency', method: 'existing_game_English_with_7JP_correction', references: `msg${bank}.dat:${id}:text` });
}

const system = fs.readFileSync(path.join(base, 'system_message.dat'));
function systemSection(section, slots) {
  let p = system.readUInt32LE(section * 4); const count = system.readUInt16BE(p); p += 2; const result = [];
  for (let id = 0; id < count; id++) {
    const record = [];
    for (let k = 0; k < slots; k++) { const start = p, n = system.readUInt16BE(p); p += 2; const text = decoder.decode(system.subarray(p, p + n)); p += n; assert.equal(system[p++], 0); record.push({ text, start, end: p }); }
    result.push(record);
  }
  return result;
}
const locations = systemSection(11, 6), mapNamePatches = [];
for (const id of mapNames) {
  const r = locations[id]; if (!r || !r[0].text || r[1].text) continue;
  const english = r[0].text === 'チャレンジダンジョン' ? 'Challenge Dungeon' : null;
  if (!english) { coreUntranslated.push({ category: 'map_name', id, japanese: r[0].text }); continue; }
  mapNamePatches.push({ section: 11, id, slot: 1, japanese: r[0].text, english, start: r[1].start, end: r[1].end });
  all.push({ id: `map-name-${id}`, category: 'map_name', japanese: r[0].text, english, scope: 'mod_related_map_name', method: 'draft_translation', references: `system_message.dat:section11:record${id}:English` });
}
// Verify copied enemy ability names already have English; don't invent a second name.
const magic = systemSection(9, 12), copiedMagic = [];
for (const [to, from] of [[73, 432], [74, 433], [76, 431], [77, 435], [78, 393], [79, 434], [80, 469], [81, 437], [204, 30], [205, 6], [206, 5]]) copiedMagic.push({ new_id: to, copy_from: from, japanese: magic[from][0].text, english: magic[from][2].text });
assert.ok(copiedMagic.every(r => r.english));
json('copied_ability_names.json', copiedMagic);

// Independent literal-location check for directly written Japanese UI text.
const tracked = execFileSync('git', ['ls-files', '-z'], { cwd: root }).toString('utf8').split('\0').filter(Boolean);
const runtimeFiles = tracked.filter(n => /^(src\/.*\.(?:c|h|smali)|manager\/app\/src\/main\/.*\.(?:kt|java|xml))$/.test(n));
const knownLocations = new Set(JSON.parse(fs.readFileSync(path.join(root, 'translations/catalog.json'), 'utf8')).flatMap(r => r.occurrences.map(o => `${o.file}:${o.line}`)));
const uncoveredLiteralLines = [];
for (const file of runtimeFiles) {
  if (/_tab\.h$/.test(file)) continue; // Decoded graphic tables / bytecode audited separately.
  let text = fs.readFileSync(path.join(root, file), 'utf8');
  if (/\.(?:c|h|kt|java)$/.test(file)) text = text.replace(/\/\*[\s\S]*?\*\/|\/\/[^\n]*|"(?:\\[\s\S]|[^"\\])*"/g, m => m.startsWith('"') ? m : m.replace(/[^\n]/g, ' '));
  text.split('\n').forEach((line, i) => {
    const isLiteral = file.endsWith('.smali') ? /const-string/.test(line) : file.endsWith('.xml') ? /android:label=/.test(line) : /"/.test(line);
    if (isLiteral && /"[^"\n]*[\u3040-\u30ff\u3400-\u9fff]/.test(line) && !knownLocations.has(`${file}:${i + 1}`)) uncoveredLiteralLines.push({ file, line: i + 1, source: line.trim() });
  });
}
// Translation files: C is always Japanese, D is always English.
const columns = ['id', 'category', 'japanese', 'english', 'scope', 'method', 'references', 'provenance', 'english_alternatives'];
csv('all_completed_translations.csv', columns, all);
csv('mod_related_translations.csv', columns, all.filter(r => r.scope.startsWith('mod_')));
csv('remaining_original_base_gaps.csv', columns, remaining);
csv('base_story_gap_audit.csv', columns, gapRows);
csv('speaker_audit.csv', columns, speakerRows);
json('field_overrides.json', fieldPatches); json('map_name_overrides.json', mapNamePatches);
json('core_text_references.json', coreReferences); json('all_map_text_references.json', references);
json('moogle_entrance_sites.json', moogleSites);
json('completed_catalog.json', all);
assert.ok(all.every(r => valid(r.japanese, r.english)), 'Completed translations must preserve tokens');
assert.equal(coreUntranslated.length, 0, JSON.stringify(coreUntranslated));
assert.equal(uncoveredLiteralLines.length, 0, JSON.stringify(uncoveredLiteralLines));
assert.equal(scanErrors.length, 0, JSON.stringify(scanErrors));

// Complete the older review CSVs too, without changing their Japanese/source columns.
const csvUpdates = [];
for (const name of ['asset_review/restored_dialogue.csv', 'extended_asset_review/maps/dialogue_candidates.csv']) {
  const location = path.join(root, 'translations', name), parsed = parseCsv(fs.readFileSync(location, 'utf8')); let changed = 0;
  for (const r of parsed.rows) {
    const tr = translation(r.japanese, r.category === 'restored_speaker');
    if (tr && (name.startsWith('asset_review') || !r.english)) { if (r.english !== tr.english) changed++; r.english = tr.english; }
  }
  assert.ok(parsed.rows.every(r => r.english)); fs.writeFileSync(location, csvString(parsed.header, parsed.rows)); csvUpdates.push({ file: name, rows: parsed.rows.length, changed });
}
// Keep the earlier JSON/gallery views consistent with their now-completed CSVs.
const dialoguePath = path.join(root, 'translations/asset_review/dialogue.json');
const dialogue = JSON.parse(fs.readFileSync(dialoguePath, 'utf8'));
const galleryPath = path.join(root, 'translations/asset_review/gallery.html');
let gallery = fs.readFileSync(galleryPath, 'utf8');
const html = s => s.replaceAll('&', '&amp;').replaceAll('<', '&lt;').replaceAll('>', '&gt;').replaceAll('"', '&quot;');
for (const r of dialogue) {
  const tr = translation(r.japanese); assert.ok(tr);
  gallery = gallery.replace(`<pre>${html(r.english_draft)}</pre>`, `<pre>${html(tr.english)}</pre>`);
  r.english_draft = tr.english; r.translation_method = tr.method; r.translation_provenance = tr.from;
  for (const speaker of r.speakers) if (speaker.japanese && !speaker.english) speaker.english = translation(speaker.japanese, true)?.english ?? '';
}
gallery = gallery.replace('English below is a draft, not an existing official translation.', 'Four passages reuse matching official English from another bank; two are new translation drafts. See completion_review for provenance and the expanded audit.');
fs.writeFileSync(dialoguePath, JSON.stringify(dialogue, null, 2) + '\n');
fs.writeFileSync(galleryPath, gallery);
const candidatePath = path.join(root, 'translations/extended_asset_review/maps/dialogue_candidates.json');
const candidates = JSON.parse(fs.readFileSync(candidatePath, 'utf8'));
for (const r of candidates) if (!r.english) { const tr = translation(r.japanese); assert.ok(tr); r.english = tr.english; r.translation_method = tr.method; r.translation_provenance = tr.from; }
fs.writeFileSync(candidatePath, JSON.stringify(candidates, null, 2) + '\n');
const internalLocation = path.join(root, 'translations/internal_references.csv'), internal = parseCsv(fs.readFileSync(internalLocation, 'utf8'));
const internalEnglish = { '単体の行動を遅くする': 'Slows one target.', '魔人': 'Magus', 'DEBUG用': 'For debugging', 'メモリーズ': 'Recollect' };
for (const r of internal.rows) r.english = internalEnglish[r.japanese] ?? r.japanese.replace('メモリーズ', 'Recollect');
fs.writeFileSync(internalLocation, csvString(internal.header, internal.rows));
// Synchronize the machine-readable source catalog with edited reference cells.
const sourceCatalogPath = path.join(root, 'translations/catalog.json'), sourceCatalog = JSON.parse(fs.readFileSync(sourceCatalogPath, 'utf8'));
for (const r of sourceCatalog) { const updated = internal.rows.find(x => x.id === r.id); if (updated) r.english = updated.english; }
fs.writeFileSync(sourceCatalogPath, JSON.stringify(sourceCatalog, null, 2) + '\n');

// Generate integration data, with stable lifetime and no pointers into temporary buffers.
const cs = text => '"' + [...Buffer.from(text, 'utf8')].map(b => '\\x' + b.toString(16).padStart(2, '0')).join('') + '"';
save('integration/native_english.h', `/* Generated translation data only. Include from native mod code. */\n#ifndef FFLMOD_NATIVE_ENGLISH_DATA_H\n#define FFLMOD_NATIVE_ENGLISH_DATA_H\nstatic const struct { const char *key, *english; } fflmod_native_english[] = {\n${sourceCsv.rows.filter(r => r.category === 'in_game').map(r => `    { ${cs(r.id)}, ${cs(r.english)} },`).join('\n')}\n};\n#endif\n`);
save('integration/story_english.h', `/* Apply to non-owning active message pointer tables, in English mode only. */\n#ifndef FFLMOD_STORY_ENGLISH_DATA_H\n#define FFLMOD_STORY_ENGLISH_DATA_H\nstatic const struct { int bank, id, speaker; const char *english; } fflmod_story_english[] = {\n${fieldPatches.map(r => `    { ${r.bank}, ${r.id}, ${r.slot === 3 ? 1 : 0}, ${cs(r.english)} },`).join('\n')}\n};\nstatic const struct { int id; const char *english; } fflmod_map_names_english[] = {\n${mapNamePatches.map(r => `    { ${r.id}, ${cs(r.english)} },`).join('\n')}\n};\n#endif\n`);
json('integration/manager_and_save_strings.json', sourceCsv.rows.filter(r => r.category !== 'in_game'));

// Optional game-format review files. Patch ONLY core English slots. Preserve all
// other language slots and the trailing metadata byte-for-byte; test by reparsing.
function encodedString(text) { const body = Buffer.from(text, 'utf8'); assert.ok(body.length <= 0x7fff); const len = Buffer.alloc(2); len.writeUInt16BE(body.length); return Buffer.concat([len, body, Buffer.from([0])]); }
const datFiles = [];
for (const bank of banks) {
  const changes = fieldPatches.filter(r => r.bank === Number(bank.name.match(/\d+/)[0])); if (!changes.length) continue;
  const patchMap = new Map(); for (const r of changes) { assert.equal(bank.records[r.id][r.slot].text, r.expected_english); const k = `${r.id}/${r.slot}`; if (patchMap.has(k)) assert.equal(patchMap.get(k), r.english); patchMap.set(k, r.english); }
  const chunks = [bank.bytes.subarray(0, 2)];
  for (const [id, slots] of bank.records.entries()) for (const [slot, value] of slots.entries()) chunks.push(patchMap.has(`${id}/${slot}`) ? encodedString(patchMap.get(`${id}/${slot}`)) : bank.bytes.subarray(value.start, value.end));
  chunks.push(bank.bytes.subarray(bank.parsed_end)); const result = Buffer.concat(chunks);
  let p = 2;
  for (const [id, slots] of bank.records.entries()) for (const [slot, value] of slots.entries()) { const n = result.readUInt16BE(p); p += 2; const text = decoder.decode(result.subarray(p, p + n)); p += n; assert.equal(result[p++], 0); assert.equal(text, patchMap.get(`${id}/${slot}`) ?? value.text); }
  assert.ok(result.subarray(p).equals(bank.bytes.subarray(bank.parsed_end)));
  save('patched_message_dat/' + bank.name, result); datFiles.push({ name: bank.name, original_sha256: hash(bank.bytes), patched_sha256: hash(result), changed_slots: patchMap.size });
}
// system_message has a little-endian section TOC: adjust following section
// offsets when the English map-name string grows; preserve all section payloads.
if (mapNamePatches.length) {
  // Word 0 is a section count, followed by count+1 offsets. It is not
  // a payload offset; including it in the payload checks would compare TOC bytes.
  const sectionCount = system.readUInt32LE(0), offsets = [];
  assert.ok(sectionCount > 0 && sectionCount < 32);
  for (let p = 0; p < (sectionCount + 2) * 4; p += 4) offsets.push(system.readUInt32LE(p));
  assert.equal(offsets[1], (sectionCount + 2) * 4);
  assert.equal(offsets.at(-1), system.length);
  const pieces = [], ordered = [...mapNamePatches].sort((a, b) => a.start - b.start); let p = 0;
  for (const r of ordered) { pieces.push(system.subarray(p, r.start), encodedString(r.english)); p = r.end; } pieces.push(system.subarray(p));
  const result = Buffer.concat(pieces);
  for (let i = 0; i < offsets.length; i++) { const delta = ordered.filter(r => r.end <= offsets[i]).reduce((n, r) => n + encodedString(r.english).length - (r.end - r.start), 0); result.writeUInt32LE(offsets[i] + delta, i * 4); }
  assert.equal(result.readUInt32LE(0), sectionCount);
  for (let i = 1; i + 1 < offsets.length; i++) if (i !== 11) assert.ok(result.subarray(result.readUInt32LE(i * 4), result.readUInt32LE((i + 1) * 4)).equals(system.subarray(offsets[i], offsets[i + 1])));
  let cp = result.readUInt32LE(44); assert.equal(result.readUInt16BE(cp), locations.length); cp += 2;
  for (const [id, record] of locations.entries()) for (const [slot, value] of record.entries()) {
    const n = result.readUInt16BE(cp); cp += 2; const text = decoder.decode(result.subarray(cp, cp + n)); cp += n; assert.equal(result[cp++], 0);
    assert.equal(text, mapNamePatches.find(r => r.id === id && r.slot === slot)?.english ?? value.text);
  }
  save('patched_message_dat/system_message.dat', result); datFiles.push({ name: 'system_message.dat', original_sha256: hash(system), patched_sha256: hash(result), changed_slots: mapNamePatches.length });
}
json('patched_message_dat/index.json', datFiles);
const stats = { source_commit: execFileSync('git', ['rev-parse', 'HEAD'], { cwd: root }).toString().trim(), runtime_files: runtimeFiles.length,
  story_banks: banks.length, story_records: banks.reduce((n, b) => n + b.count, 0), maps_scanned: mapCount, events_scanned: eventCount, commands_scanned: commandCount,
  moogle_entrance_sites: moogleSites.length, core_text_reference_sites: coreReferences.length,
  unique_base_text_gaps: missing.size, base_text_gaps_completed: gapRows.filter(r => r.english).length,
  base_text_gaps_official_reuse: gapRows.filter(r => r.method.startsWith('existing_game')).length,
  base_text_gaps_new_drafts: gapRows.filter(r => r.method === 'draft_translation').length,
  original_credit_or_other_gaps_remaining: remaining.length, speaker_gap_entries: speakerRows.length, speakers_completed: speakerRows.filter(r => r.english).length,
  core_missing_translations: coreUntranslated.length, uncovered_literal_lines: uncoveredLiteralLines.length, map_scan_errors: scanErrors.length,
  completed_catalog_entries: all.length, core_field_override_slots: fieldPatches.length, map_name_overrides: mapNamePatches.length,
  copied_ability_names_verified_English: copiedMagic.length, patched_message_files: datFiles.length, csv_updates: csvUpdates,
  limitations: ['Static reference coverage is not execution reachability or a complete playthrough.', 'Original staff credit blocks and other unscoped gaps are inventoried separately.', 'Runtime hook wiring, native Android build, and visual layout verification are not performed by this data generator.'] };
json('audit_summary.json', stats); json('scan_errors.json', scanErrors); json('uncovered_literals.json', uncoveredLiteralLines);
console.log(JSON.stringify(stats, null, 2));
