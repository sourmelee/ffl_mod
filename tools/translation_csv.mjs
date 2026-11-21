import assert from 'node:assert/strict';
export function parseTranslationCsv(text) {
  const records = []; let cells = [], cell = '', quoted = false;
  text = text.replace(/^\ufeff/, '');
  for (let i = 0; i < text.length; i++) {
    const ch = text[i];
    if (ch === '"') {
      if (quoted && text[i + 1] === '"') { cell += '"'; i++; } else quoted = !quoted;
    } else if (!quoted && [',', '\n', '\r'].includes(ch)) {
      cells.push(cell); cell = '';
      if (ch !== ',') { records.push(cells); cells = []; if (ch === '\r' && text[i + 1] === '\n') i++; }
    } else cell += ch;
  }
  assert.ok(!quoted, 'Unclosed CSV field');
  if (cell || cells.length) { cells.push(cell); records.push(cells); }
  const header = records.shift();
  return records.map(r => Object.fromEntries(header.map((name, i) => [name, r[i] ?? ''])));
}
