import fs from 'node:fs';
import path from 'node:path';
import assert from 'node:assert/strict';

export function extractExtended(api) {
const { root, out, base, save, json, source, read, sha, fnv, array, icp, rgba, png } = api;
assert.ok(base, 'Pass --base-data pointing to the existing raw OBB extraction.');
const quote = x => '"' + String(x ?? '').replaceAll('"', '""') + '"';
const csv = (name, header, rows) => save(name, '\ufeff' + [header, ...rows].map(r => r.map(quote).join(',')).join('\r\n') + '\r\n');
const c = source('fflmod.c');
// Mask comments without shifting locations, and parse brace initializers as data.
const masked = c.replace(/\/\*[\s\S]*?\*\/|\/\/[^\n]*|"(?:\\.|[^"\\])*"/g, s => s.startsWith('"') ? s : s.replace(/[^\n]/g, ' '));
const defines = new Map([...masked.matchAll(/^\s*#define\s+(\w+)\s+(-?(?:0x[0-9a-f]+|\d+))(?:[uUlL]*)\b/gmi)].map(m => [m[1], Number(m[2])]));
function expr(s) {
  s = s.trim();
  if (/^-?(?:0x[0-9a-f]+|\d+)[uUlL]*$/i.test(s)) return Number(s.replace(/[uUlL]+$/, ''));
  if (defines.has(s)) return defines.get(s);
  return s; // A symbol reference is recorded, never evaluated as code.
}
function initializer(text, start) {
  let p = start + 1, token = '', result = [];
  function flush() { if (token.trim()) result.push(expr(token)); token = ''; }
  while (p < text.length) {
    const ch = text[p];
    if (ch === '{') { flush(); const nested = initializer(text, p); result.push(nested.value); p = nested.end; continue; }
    if (ch === '}') { flush(); return { value: result, end: p + 1 }; }
    if (ch === ',') flush(); else token += ch;
    p++;
  }
  throw Error('Unclosed initializer');
}
const tables = [];
const declarations = /static const\s+((?:struct\s*\{[^}]+\}|[\w\s]+?))\s+(\w+)\s*((?:\[[^\]]*\])+?)\s*=\s*\{/g;
for (const m of masked.matchAll(declarations)) {
  let name = m[2], dims = m[3], p = m.index + m[0].length - 1;
  while (true) {
    const parsed = initializer(masked, p), line = c.slice(0, p).split('\n').length;
    const stem = `${name}_L${line}`;
    const item = { name, source_line: line, declaration_type: m[1].trim(), declared_dimensions: dims, values: parsed.value,
      storage_note: 'Initializer values as written; omitted C zero-fill and ABI padding are not materialized in JSON/CSV.' };
    tables.push(item); json(`tables/${stem}.json`, item);
    const rows = parsed.value.map(v => Array.isArray(v) ? v : [v]);
    csv(`tables/${stem}.csv`, Array.from({ length: Math.max(...rows.map(r => r.length)) }, (_, i) => 'field_' + i), rows.map(r => r.map(x => Array.isArray(x) ? JSON.stringify(x) : x)));
    const flat = parsed.value.flat(Infinity);
    if (/^(?:unsigned char|u8)$/.test(m[1].trim()) && flat.every(x => Number.isInteger(x) && x >= 0 && x <= 255)) {
      const lengths = [...dims.matchAll(/\[([^\]]+)\]/g)].map(x => x[1].trim()).filter(Boolean).map(expr);
      const n = lengths.length && lengths.every(Number.isInteger) ? lengths.reduce((a, b) => a * b, 1) : flat.length;
      assert.ok(flat.length <= n); const bytes = Buffer.alloc(n); Buffer.from(flat).copy(bytes); save(`raw_tables/${stem}.dat`, bytes);
    }
    const next = masked.slice(parsed.end).match(/^\s*,\s*(\w+)\s*((?:\[[^\]]*\])+?)\s*=\s*\{/);
    if (!next) break; name = next[1]; dims = next[2]; p = parsed.end + next[0].length - 1;
  }
}
json('tables/index.json', tables.map(({ values, ...t }) => ({ ...t, rows: values.length })));
const find = (name, line) => { const a = tables.filter(t => t.name === name && (!line || t.source_line === line)); assert.equal(a.length, 1, name + ':' + line); return a[0].values; };
const alarmSource = source('alarm_tab.h'), alarm = [...alarmSource.matchAll(/0x[0-9a-f]+/gi)].map(x => Number(x[0]));
assert.ok(alarm.every((v, i) => i === 0 || v > alarm[i - 1]));
const alarmBytes = Buffer.alloc(alarm.length * 4); alarm.forEach((v, i) => alarmBytes.writeUInt32LE(v, i * 4));
save('raw_tables/alarm_targets.u32le.dat', alarmBytes);
csv('tables/alarm_targets.csv', ['chapter', 'formation', 'priority_tier', 'packed_u32_hex'], alarm.map(v => [v >>> 18, (v >>> 2) & 0xffff, v & 3, '0x' + v.toString(16).padStart(8, '0')]));
csv('tables/knight_tile_fixes.csv', ['map', 'layer', 'x', 'y', 'old_tile', 'new_tile'], find('kn_fix'));
csv('tables/ninja_tile_fixes.csv', ['map', 'layer', 'x', 'y', 'old_tile', 'new_tile'], find('nj_fix'));
csv('tables/return_routes.csv', ['chapter', 'destination_map', 'old_x', 'old_y', 'new_x', 'new_y'], find('k_retFix'));
csv('tables/guest_upgrades.csv', ['member_id', 'magic_ids', 'add_passives', 'remove_passives', 'white_black_magic_flag'], find('k_guestUp').map(r => r.map(x => Array.isArray(x) ? JSON.stringify(x) : x)));
csv('tables/guest_job_passives.csv', ['job_id', 'member_id', 'original_passives', 'upgraded_passives'], find('k_guestJob').map(r => r.map(x => Array.isArray(x) ? JSON.stringify(x) : x)));
csv('tables/styx_magic.csv', ['new_id', 'copy_from_id', 'mp', 'hit', 'target', 'status', 'battle_script', 'category', 'wait', 'sjis_description_symbol', 'utf8_description_symbol'], find('k_styxMagic'));
csv('tables/inherited_passives.csv', ['mastered_job', 'inherited_passive'], find('k_inherit'));
csv('tables/circle_offsets.csv', ['chapter', 'map', 'event', 'x', 'y', 'draw_dy'], find('circ_fix'));
csv('tables/chain_offsets.csv', ['map', 'event_lo', 'event_hi', 'x', 'y', 'draw_dx', 'draw_dy'], find('nj_chain'));
csv('tables/enemy_balance.csv', ['enemy_id', 'name', 'condition', 'level', 'hp', 'exp', 'ap', 'attack', 'attack_multiplier', 'defense', 'speed_min', 'speed_max', 'magic', 'scope'], [
  [92, 'Talos', 'chapter 0', 20, 4000, 2000, 10, 70, 2, 15, 16, 16, '', 'native struct override; blank fields retain original'],
  [107, 'Stone Golem', 'all chapters; original Lv29 and defense29', '', '', '', '', '', '', 20, '', '', '', 'native struct override; blank fields retain original'],
  [372, 'Earth Eater', 'chapter 7', 50, 25000, '', '', 120, 14, '', 30, 30, 50, 'native struct override; blank fields retain original'],
  [421, 'Elder Dragon', 'chapter 8', 60, 32000, '', '', 164, 10, '', 40, 40, 86, 'native struct override; blank fields retain original'],
  [459, 'Yamata-no-Orochi', 'chapter 9', 60, 32000, '', '', '', 11, '', '', '', 90, 'native struct override; blank fields retain original']
]);

// Reproduce the native bsc.dat append and the two guarded no-escape patches.
const bsc = fs.readFileSync(path.join(base, 'bsc.dat')), bscHeader = source('bsc_tab.h');
assert.equal(bsc.length, 146178); const section1 = 0x1ad33;
for (const [id, off, name] of [[469, 28282, 'bsc_469_orig'], [437, 24373, 'bsc_437_orig']]) {
  assert.equal(bsc.readUInt32BE(section1 + id * 4), off); const expected = array(bscHeader, name);
  assert.ok(bsc.subarray(section1 + off, section1 + off + expected.length).equals(expected));
}
for (const id of [80, 81]) assert.equal(bsc.readUInt32BE(section1 + id * 4), 6586);
const b80 = array(bscHeader, 'bsc_80'), b81 = array(bscHeader, 'bsc_81'), patchedBsc = Buffer.concat([bsc, b80, b81]);
patchedBsc.writeUInt32BE(bsc.length - section1, section1 + 80 * 4);
patchedBsc.writeUInt32BE(bsc.length + b80.length - section1, section1 + 81 * 4);
const bscChecks = [];
const start570 = 0x10 + patchedBsc.readUInt32BE(0x10 + 570 * 4) + 32;
const cmp570 = Buffer.from(find('k_escCmp')); assert.ok(patchedBsc.subarray(start570, start570 + cmp570.length).equals(cmp570)); patchedBsc[start570 + 7] = 6;
bscChecks.push({ script: 570, byte_offset: start570 + 7, old: 3, new: 6 });
const start625 = 0x10 + patchedBsc.readUInt32BE(0x10 + 625 * 4), at = find('k_at'), pct = find('k_pct');
for (let i = 0; i < 6; i++) {
  const p = start625 + at[i], pattern = Buffer.from(i < 5 ? find('k_rnd') : find('k_cnt'));
  assert.ok(patchedBsc.subarray(p, p + (i < 5 ? 12 : 13)).equals(pattern.subarray(0, i < 5 ? 12 : 13)));
  if (i < 5) assert.equal(patchedBsc[p + 12], pct[i]);
  assert.ok(patchedBsc.subarray(p + 13, p + 16).equals(Buffer.from([0, 5, 1])));
  assert.equal(at[i] + 16 + patchedBsc.readInt16BE(p + 16), 0x12a);
  bscChecks.push({ script: 625, byte_offset: p + 7, old: patchedBsc[p + 7], new: 6 }); patchedBsc[p + 7] = 6;
}
assert.ok(patchedBsc.subarray(start625 + 0x12a, start625 + 0x12e).equals(Buffer.from(find('k_esc'))));
save('battle/bsc_original.dat', bsc); save('battle/bsc_runtime_image.dat', patchedBsc);
json('battle/bsc_changes.json', { original_sha256: sha(bsc), runtime_sha256: sha(patchedBsc), appended_scripts: [80, 81], no_escape_changes: bscChecks,
  note: 'Exact native append layout; the original header end marker is intentionally unchanged, matching hook_BufLoad. Not a tested installable archive replacement.' });

// Export on-disk boot sections, retaining their original format and byte order.
const boot = fs.readFileSync(path.join(base, 'boot_data.dat')), offsets = [];
for (let p = 0; p + 4 <= boot.length; p += 4) { const v = boot.readUInt32LE(p); offsets.push(v); if (v === boot.length) break; assert.ok(offsets.length < 40); }
assert.equal(offsets.at(-1), boot.length);
const bootSections = [];
for (let i = 1; i + 1 < offsets.length; i++) {
  assert.ok(offsets[i + 1] >= offsets[i]); const b = boot.subarray(offsets[i], offsets[i + 1]); save(`boot_sections/section_${String(i).padStart(2, '0')}.dat`, b);
  bootSections.push({ section: i, offset: offsets[i], bytes: b.length, sha256: sha(b) });
}
json('boot_sections/index.json', bootSections);
const referenceFiles = [];
for (const name of ['capk.dat', 'field_constant.dat', 'form.bin', 'layout.dat', 'chara_set.dat']) {
  const b = fs.readFileSync(path.join(base, name)); save('base_reference/' + name, b);
  referenceFiles.push({ name, bytes: b.length, sha256: sha(b), provenance: 'Original base-game file, not a modded replacement' });
}
json('base_reference/index.json', referenceFiles);
const sjis = new TextDecoder('shift_jis');
for (const [label, section, bodySize] of [['magic', 2, 54], ['passives', 3, 24], ['commands', 4, 25], ['items', 5, 54], ['jobs', 6, 126], ['monsters', 9, 64]]) {
  const b = boot.subarray(offsets[section], offsets[section + 1]), count = b.readUInt16BE(0); let p = 2; const records = [];
  for (let id = 0; id < count; id++) {
    assert.ok(p < b.length, label + ':' + id);
    if (b[p] === 255) { p++; records.push({ id, deleted: true }); continue; }
    const n = b[p++], name = sjis.decode(b.subarray(p, p + n)); p += n; let desc = '';
    if (label !== 'monsters') { const n = b[p++]; desc = sjis.decode(b.subarray(p, p + n)); p += n; }
    assert.ok(p + bodySize <= b.length, label + ':' + id); const body = b.subarray(p, p + bodySize); p += bodySize;
    records.push({ id, name, description: desc, body_hex: body.toString('hex') });
  }
  json(`boot_sections/${label}.json`, records); csv(`boot_sections/${label}.csv`, ['id', 'japanese_name', 'japanese_description', 'deleted', 'body_hex'], records.map(r => [r.id, r.name, r.description, !!r.deleted, r.body_hex]));
}

// Map packet extraction uses the game's mpkh index, never heuristic byte scans.
const maps = [], packsSeen = new Set(), eventMessageRefs = [];
function selected(ch, id) { return ch === 0 ? id >= 3000 && id <= 3002 : ch === 2 ? id >= 1400 && id < 1500 : ch === 3 ? id >= 1600 && id < 1700 : [4, 5, 6].includes(ch) ? id >= 2000 && id < 2100 || ch === 4 && id === 2 : [7, 9].includes(ch) ? id >= 1100 && id < 1200 : ch === 8 && id >= 1900 && id < 2000; }
function parseMap(b) {
  const w = b.readUInt16BE(8), h = b.readUInt16BE(10), n = b[25]; assert.ok(w && h && n <= 8);
  let p = 26 + n * 7; const layers = [];
  for (let i = 0; i < n; i++) { const flags = [...b.subarray(26 + i * 7, 33 + i * 7)]; let tiles = null, attrA = null, attrB = null;
    if (flags[0]) { tiles = p; p += w * h * 2; if (flags[2]) { attrA = p; p += w * h; } if (flags[1]) { attrB = p; p += w * h; } }
    assert.ok(p <= b.length); layers.push({ flags, tiles, attrA, attrB });
  }
  assert.ok(p + 16 <= b.length); return { w, h, layers, tail: p, tilesets: [[b.readInt8(p + 6), b[p + 7]], [b.readInt8(p + 8), b[p + 9]]], event_offset: b.readUInt32BE(0) + 4 };
}
const table = (name, line) => find(name, line);
function patchMap(original, ch, id) {
  const b = Buffer.from(original), m = parseMap(b), changes = [], { w, h, layers, tail } = m;
  const get = (layer, x, y) => x >= 0 && y >= 0 && x < w && y < h && layers[layer]?.tiles !== null && layers[layer]?.tiles !== undefined ? b.readUInt16LE(layers[layer].tiles + (y * w + x) * 2) : 0;
  const set = (layer, x, y, value, why) => { const p = layers[layer].tiles + (y * w + x) * 2, old = b.readUInt16LE(p); if (old !== value) { b.writeUInt16LE(value, p); changes.push({ kind: 'tile', layer, x, y, old, value, why }); } };
  const byte = (p, value, why) => { if (b[p] !== value) { changes.push({ kind: 'byte', offset: p, old: b[p], value, why }); b[p] = value; } };
  const bush = v => v >= 0x30 && v <= 0x36 || v >= 0x40 && v <= 0x44 || v >= 0x50 && v <= 0x54;
  const blank = v => [0x35, 0x36, 0x43, 0x52].includes(v), bushTable = [[0x30, 0x32, 0x34], [0x40, 0x41, 0x44], [0x50, 0x53, 0x54]];
  const fillBush = () => { for (let y = 0; y < h; y++) for (let x = 0; x < w; x++) if (blank(get(0, x, y))) set(0, x, y, bushTable[!bush(get(0, x, y - 1)) ? 0 : !bush(get(0, x, y + 1)) ? 2 : 1][!bush(get(0, x - 1, y)) ? 0 : !bush(get(0, x + 1, y)) ? 2 : 1], 'bush gaps'); };
  for (let i = 0; i < layers.length; i++) if (!layers[i].flags[0]) { byte(26 + i * 7 + 1, 0, 'inactive layer flags'); byte(26 + i * 7 + 2, 0, 'inactive layer flags'); }
  if (m.tilesets[0][0] === 7 && m.tilesets[0][1] === 0 && layers[1]?.tiles !== null && layers[1]?.tiles !== undefined) {
    let n = 0; for (let y = 0; y < h; y++) for (let x = 0; x < w; x++) if (get(1, x, y) === 0xb1) n++;
    if (n >= 40) for (let y = 0; y < h; y++) for (let x = 0; x < w; x++) if (get(1, x, y) === 0xb0) set(1, x, y, 0, 'Dragoon cliff remnant');
  }
  if (ch === 0 && w === 40 && h === 40 && !b[6] && !b[7]) {
    const names = new Map([[0xfab13d2d, 69], [0x0968b9cd, 70], [0xdafdc549, 38]]), hash = fnv(b.subarray(8, 4100));
    if (names.has(hash)) { byte(5, 1, 'Dawn map name display'); byte(6, names.get(hash) >>> 8, 'Dawn map name'); byte(7, names.get(hash) & 255, 'Dawn map name'); if (id === 3001 && b[2624] === 0x2e && b[2625] === 0) byte(2624, 0x77, 'Dawn rock wall'); }
  }
  if (ch === 3 && [59, 61].includes(w) && h === 65 && layers[0]?.tiles !== null) {
    const hash = fnv(b.subarray(8, 4100)); if ([0x638b9f38, 0x3e93510e].includes(hash)) fillBush();
    if (w === 61 && layers.length >= 2 && [0x1b45cf82, 0xc8082f38, 0xbacfb7f5].includes(hash)) for (let y = 0; y < h; y++) for (let x = 0; x < w; x++) {
      if (get(0, x, y) !== 0x24) continue; const v = get(1, x, y); if ([0x9d, 0xad].includes(v)) set(1, x, y, 0xbd, 'hidden route'); else if ([0x9e, 0xae].includes(v)) set(1, x, y, 0xbe, 'hidden route');
    }
  }
  if ([4, 5].includes(ch) && layers[0]?.tiles !== null) for (const [mid, width, height, x, y, old, next] of table('fx', ch === 4 ? 3716 : 3755)) if (id === mid && w === width && h === height && get(0, x, y) === old) set(0, x, y, next, ch === 4 ? 'Bard tile fix' : 'Dark Knight tile fix');
  if (ch === 4 && id === 2 && w === 100 && h === 100 && b[tail + 10] === 1 && !b[tail + 13] && !b[tail + 14] && !b[tail + 15]) { byte(tail + 13, 1, 'Bard cloud background'); byte(tail + 14, 3, 'Bard cloud background'); }
  if (ch === 6 && layers[0]?.tiles !== null) {
    if (id === 2003 && w === 40 && h === 40 || id === 2009 && w === 50 && h === 50) fillBush();
    else if (id === 2006 && w === 40 && h === 40 || id === 2010 && w === 44 && h === 43 || id === 2008 && w === 40 && h === 40) for (let y = 0; y < h; y++) for (let x = 0; x < w; x++) {
      const v = get(0, x, y); if (id === 2008 && v === 0x2e) set(0, x, y, 0x77, 'Memorist cave gap'); else if (id !== 2008 && [0x71, 0x75].includes(v)) set(0, x, y, 1, 'Memorist sand gap');
    }
  }
  if (ch === 7 && table('sz', 3817).some(([mid, width, height]) => mid === id && width === w && height === h)) {
    if (id === 1105) { if (get(0, 12, 25) === 0x18c) set(0, 12, 25, 0x102, 'Dancer exit floor'); }
    else for (let y = 0; y < h; y++) for (let x = 0; x < w; x++) { const v = get(0, x, y); if (v === 0x75) set(0, x, y, 1, 'Dancer sand gap'); else if (v === 0x71) set(0, x, y, get(0, x, y - 1) === 0x51 ? 0x61 : 1, 'Dancer cliff gap'); }
  }
  if ((ch === 8 && id >= 1900 && id <= 1915 || ch === 9 && id >= 1100 && id <= 1115) && table('sz', 3971).some(([mid, width, height]) => mid === id && width === w && height === h)) {
    for (const [mid, layer, x, y, old, next] of table(ch === 8 ? 'kn_fix' : 'nj_fix')) if (mid === id && get(layer, x, y) === old) set(layer, x, y, next, 'castle tile fix');
    if (b[tail + 6] === 30 && b[tail + 8] === 1 && !b[tail + 7] && !b[tail + 9] && !b[tail + 10] && !b[tail + 13] && !b[tail + 14] && !b[tail + 15]) { byte(tail + 13, 1, 'castle background'); byte(tail + 14, 4, 'castle background'); }
  }
  return { bytes: b, changes };
}
const textures = new Map();
function texture(id, variant) {
  const key = `${id}_${variant}`; if (!textures.has(key)) { const name = `mc${id}_${variant}.dat`; if (!fs.existsSync(path.join(base, name))) { textures.set(key, null); return null; } const image = icp(name); textures.set(key, { ...image, pixels: rgba(image.idx, image.palette) }); }
  return textures.get(key);
}
function render(b, m) {
  const width = m.w * 32, height = m.h * 32, pixels = Buffer.alloc(width * height * 4), missing = new Set();
  for (const layer of m.layers) {
    if (layer.tiles === null) continue;
    for (let y = 0; y < m.h; y++) for (let x = 0; x < m.w; x++) {
      const tile = b.readUInt16LE(layer.tiles + (y * m.w + x) * 2); if (!tile) continue;
      const slot = tile >>> 8; if (slot > 1 || m.tilesets[slot][0] < 0) { missing.add(tile); continue; }
      const tex = texture(...m.tilesets[slot]); if (!tex) { missing.add(tile); continue; }
      const sx = (tile & 15) * 32, sy = ((tile & 255) >>> 4) * 32;
      if (sx + 32 > tex.w || sy + 32 > tex.h) { missing.add(tile); continue; }
      for (let yy = 0; yy < 32; yy++) for (let xx = 0; xx < 32; xx++) {
        const s = ((sy + yy) * tex.w + sx + xx) * 4, d = ((y * 32 + yy) * width + x * 32 + xx) * 4, alpha = tex.pixels[s + 3];
        if (!alpha) continue;
        if (alpha === 255 || !pixels[d + 3]) { tex.pixels.copy(pixels, d, s, s + 4); continue; }
        const da = pixels[d + 3] / 255, sa = alpha / 255, a = sa + da * (1 - sa);
        for (let k = 0; k < 3; k++) pixels[d + k] = Math.round((tex.pixels[s + k] * sa + pixels[d + k] * da * (1 - sa)) / a);
        pixels[d + 3] = Math.round(a * 255);
      }
    }
  }
  return { image: png(width, height, pixels), missing: [...missing] };
}
for (const ch of [0, 2, 3, 4, 5, 6, 7, 8, 9]) {
  const index = fs.readFileSync(path.join(base, `mpkh${ch}.dat`)); let p = 8;
  const number = n => { const v = index.readUIntBE(p, n); p += n; return v; };
  for (let pack = 0; pack < index[7]; pack++) {
    const count = number(index[4]), filename = `mpk${ch}_${pack}.dat`, bytes = fs.readFileSync(path.join(base, filename)); let off = 0;
    for (let i = 0; i < count; i++) {
      const id = number(index[5]), size = number(index[6]); assert.ok(off + size <= bytes.length);
      const original = bytes.subarray(off, off + size), originalOffset = off; off += size; if (!selected(ch, id)) continue;
      const m = parseMap(original), fixed = patchMap(original, ch, id), stem = `chapter${ch}_map${id}`;
      save(`maps/original/${stem}.dat`, original); if (fixed.changes.length) save(`maps/static_mapinfo_patched/${stem}.dat`, fixed.bytes);
      json(`maps/metadata/${stem}.json`, { chapter: ch, map: id, pack: filename, offset: originalOffset, size, ...m, original_sha256: sha(original), static_mapinfo_sha256: sha(fixed.bytes), changes: fixed.changes });
      let ep = m.event_offset; assert.ok(ep < original.length);
      const eventCount = original[ep++], events = [];
      for (let eventIndex = 0; eventIndex < eventCount; eventIndex++) {
        assert.ok(ep + 0x41 <= original.length); const header = original.subarray(ep, ep + 0x3f), eventId = original.readUInt16BE(ep), commands = [];
        const commandCount = original.readUInt16BE(ep + 0x3f); ep += 0x41;
        for (let block = 0; block < commandCount; block++) {
          assert.ok(ep < original.length); const length = original[ep++]; assert.ok(ep + length <= original.length);
          const command = original.subarray(ep, ep + length); ep += length;
          commands.push({ block, opcode: command[0], bytes_hex: command.toString('hex') });
          if ([0, 1, 0x3b].includes(command[0]) && command.length >= 3) eventMessageRefs.push({ chapter: ch, map: id, event: eventId, block, opcode: command[0], candidate_message_id: command.readInt16BE(1), bytes_hex: command.toString('hex') });
        }
        events.push({ event_id: eventId, header_hex: header.toString('hex'), commands });
      }
      assert.equal(ep, original.length, stem + ': event records must consume map tail');
      json(`maps/events/${stem}.json`, events); save(`maps/events/${stem}.events.dat`, original.subarray(m.event_offset));
      const preview = render(fixed.bytes, m); save(`maps/previews/${stem}.png`, preview.image);
      for (const [layerId, layer] of m.layers.entries()) if (layer.tiles !== null) {
        save(`maps/layers/${stem}_layer${layerId}.u16le.dat`, fixed.bytes.subarray(layer.tiles, layer.tiles + m.w * m.h * 2));
        if (layer.attrA !== null) save(`maps/layers/${stem}_layer${layerId}_attributeA.u8.dat`, fixed.bytes.subarray(layer.attrA, layer.attrA + m.w * m.h));
        if (layer.attrB !== null) save(`maps/layers/${stem}_layer${layerId}_attributeB.u8.dat`, fixed.bytes.subarray(layer.attrB, layer.attrB + m.w * m.h));
      }
      maps.push({ chapter: ch, id, width: m.w, height: m.h, tilesets: m.tilesets, static_changes: fixed.changes.length, source: filename, offset: originalOffset, bytes: size, original_sha256: sha(original), missing_tiles: preview.missing, preview: `maps/previews/${stem}.png` }); packsSeen.add(filename);
    }
    assert.equal(off, bytes.length, filename + ' index must consume entire pack');
  }
}
assert.equal(maps.length, 90); json('maps/index.json', maps);
json('maps/event_message_candidates.json', eventMessageRefs);
csv('maps/event_message_candidates.csv', ['chapter', 'map', 'event', 'block', 'opcode', 'candidate_message_id', 'command_hex'], eventMessageRefs.map(r => [r.chapter, r.map, r.event, r.block, r.opcode, r.candidate_message_id, r.bytes_hex]));
// Resolve only numeric candidates within a story bank. Opcode flags and script
// reachability still require review; this is an inventory, not a reachability claim.
const dialogueCandidates = new Map();
for (const chapter of [...new Set(eventMessageRefs.map(r => r.chapter))]) {
  const b = fs.readFileSync(path.join(base, `msg${chapter}.dat`)), count = b.readUInt16BE(0);
  const refs = eventMessageRefs.filter(r => r.chapter === chapter && r.candidate_message_id >= 0 && r.candidate_message_id < count);
  if (!refs.length) continue; const byId = new Map(); for (const r of refs) { const list = byId.get(r.candidate_message_id) ?? []; list.push(r); byId.set(r.candidate_message_id, list); }
  let p = 2;
  for (let id = 0; id <= Math.max(...byId.keys()); id++) {
    const slots = [];
    for (let k = 0; k < 12; k++) { const n = b.readUInt16BE(p); p += 2; assert.ok(p + n < b.length); slots.push(new TextDecoder('utf-8', { fatal: true }).decode(b.subarray(p, p + n))); p += n; assert.equal(b[p++], 0); }
    if (!byId.has(id) || !slots[0]) continue;
    const key = slots[0] + '\0' + slots[2], row = dialogueCandidates.get(key) ?? { id: `base-event-message-${chapter}-${id}`, japanese: slots[0], english: slots[2], references: [] };
    row.references.push(...byId.get(id).map(r => `chapter${chapter}/map${r.map}/event${r.event}/block${r.block}/message${id}`)); dialogueCandidates.set(key, row);
  }
}
csv('maps/dialogue_candidates.csv', ['id', 'category', 'japanese', 'english', 'references', 'note'], [...dialogueCandidates.values()].map(r => [r.id, 'base_event_candidate', r.japanese, r.english, r.references.join(' | '), 'Original base-event numeric reference; flags and execution reachability not proven. Blank English means no existing English text.']));
json('maps/dialogue_candidates.json', [...dialogueCandidates.values()]);
csv('maps/index.csv', ['chapter', 'map_id', 'width_tiles', 'height_tiles', 'static_changes', 'source_pack', 'source_offset', 'bytes', 'preview'], maps.map(m => [m.chapter, m.id, m.width, m.height, m.static_changes, m.source, m.offset, m.bytes, m.preview]));

const summary = { source_commit: '7fe105ff91aa028860876e3501b7325ede4567e8', maps: maps.length, mapinfo_modified: maps.filter(m => m.static_changes).length,
  map_changes: maps.reduce((n, m) => n + m.static_changes, 0), source_tables: tables.length, alarm_targets: alarm.length, boot_sections: bootSections.length,
  bsc_original_bytes: bsc.length, bsc_runtime_bytes: patchedBsc.length, map_previews_with_missing_tiles: maps.filter(m => m.missing_tiles.length).length,
  base_reference_files: referenceFiles.length, base_event_message_candidates: eventMessageRefs.length, unique_dialogue_candidates: dialogueCandidates.size,
  dialogue_candidates_without_english: [...dialogueCandidates.values()].filter(r => !r.english).length,
  scope: 'Extracted original challenge-dungeon chunks, static LoadMapInfo-stage patches, base boot tables, and native source recipes. Runtime event/actor/save-dependent hooks are not fully baked into map DATs.' };
json('summary.json', summary);
const esc = s => String(s).replaceAll('&', '&amp;').replaceAll('<', '&lt;').replaceAll('>', '&gt;').replaceAll('"', '&quot;');
save('gallery.html', `<!doctype html><html lang="en"><meta charset="utf-8"><title>FFLmod maps and tables</title><style>body{font:16px system-ui;background:#15191e;color:#eff2f5;margin:32px}h1,h2{color:#eab65f}.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(300px,1fr));gap:20px}figure{background:#222b35;padding:16px;margin:0}img{width:100%;height:300px;object-fit:contain;image-rendering:pixelated;background:#101419}a{color:#9bd3ff}figcaption{padding:10px 0}</style><h1>FFLmod restored dungeon maps</h1><p>89 existing dungeon maps plus the Bard sea map. Static map-info corrections are baked into separately labeled copies. Previews show tile layers only, at a fixed frame, without actors, palette fades, parallax backgrounds, or script effects. These are not newly authored maps.</p><p>${esc(JSON.stringify(summary))}</p><p><a href="README.md">Extraction notes</a> · <a href="tables/index.json">Source-table index</a> · <a href="maps/index.csv">Map inventory</a></p><div class="grid">${maps.map(m => `<figure><a href="${m.preview}"><img loading="lazy" src="${m.preview}" alt="Chapter ${m.chapter}, map ${m.id}"></a><figcaption>Chapter ${m.chapter} · Map ${m.id} · ${m.width} × ${m.height}<br>${m.static_changes} static corrections · <a href="maps/original/chapter${m.chapter}_map${m.id}.dat">Original DAT</a>${m.static_changes ? ` · <a href="maps/static_mapinfo_patched/chapter${m.chapter}_map${m.id}.dat">Map-info patched DAT</a>` : ''}</figcaption></figure>`).join('')}</div></html>`);
save('README.md', `# FFLmod extended data extraction

Source commit: ${summary.source_commit} (1.1.15). The game archives and runtime
source files were not modified. Earlier translations and extracted artwork
were not regenerated or overwritten.

## Maps

- ${maps.length} original map records: 89 challenge-dungeon maps and the Bard sea
  map. They were sliced using mpkh indexes, with whole-pack length checks.
- ${summary.mapinfo_modified} separately labeled map-info-patched records, with
  ${summary.map_changes} static tile/header corrections. These reproduce the
  data edits in hook_LoadMI's map-info repair routines. Guard hashes, map IDs,
  dimensions and expected old values are honored.
- 90 PNG previews, map metadata, individual u16-LE tile layers and u8 attribute
  arrays. Previews use static tile layers with the original palette and a fixed
  tile frame. No NPCs, scripted tint/fog, animated background or event sprites
  are shown. ${summary.map_previews_with_missing_tiles} previews have unhandled tile references;
  their IDs are listed in maps/index.json rather than silently replaced.

These are existing base-game maps restored or repaired by the mod, not whole
new maps bundled in its repository. The patched DATs contain ONLY static
map-info changes, not all runtime behavior. In particular, actor draw offsets,
camera clamps, conditional enemy stats, item/ability data and replacement event
scripts are not all baked into these map files. Runtime event hooks, including
save-point insertion, moogle placement, chain/gate adjustments and reward logic,
remain separate. Do not treat these review exports as tested installable files.

Every original map's event tail was also parsed and consumed to its exact end.
maps/events contains original event headers/commands as JSON and raw .events.dat
files. event_message_candidates lists SetMessage, ScriptSentence and YesNoDialog
operands. The
dialogue_candidates CSV resolves in-range numeric operands to original Japanese
and existing English, with source references. Its ${dialogueCandidates.size} unique
entries include ${[...dialogueCandidates.values()].filter(r => !r.english).length} with blank English.
These are candidates: variable-indirection flags, bank-selection flags and
actual execution reachability have not all been proved. They are not all newly
added or definitely displayed dialogue, and were not merged into the reviewed
translation CSVs.

## Source tables

${tables.length} native initializer tables were extracted to JSON and CSV,
including secondary declarations on the same line. Original line numbers and
C types are retained. Named, column-labeled CSVs cover knight/ninja tile fixes,
return routes, guest upgrades, guest job passives, Styx magic, inherited passives,
circle draw offsets and chain draw offsets. Numeric byte-array initializers
also have .dat dumps. Other structs are kept as data tables, without inventing
native pointer values or ABI padding. JSON records initializer values as written;
C implicit zero-fill is materialized only for byte-array binary dumps.
enemy_balance.csv separately records the five conditional monster-stat recipes.

Alarm has ${alarm.length} decoded chapter/formation/tier targets and an exact
packed u32-little-endian DAT. Tier 1 prioritizes farming/tail/drop-equipment
targets; tier 2 is fang-only, following the author's table comment.

These tables include internal UI/hook matching data as well as gameplay data.
They are source recipes, not proof that every row becomes a separate OBB file.

## Game-format DATs and base tables

- battle/bsc_runtime_image.dat reconstructs the native bsc.dat append: added
  scripts 80/81 and guarded no-escape changes to scripts 570/625. Original
  scripts and matching bytes were verified. The original header end marker is
  left unchanged, matching the native hook rather than claiming a canonical
  archive rebuild. bsc_changes.json records offsets and hashes.
- boot_sections contains ${bootSections.length} raw sections from the existing
  boot_data.dat, plus decoded magic, passive, command, item, job and monster
  records as JSON/CSV. These are explicitly ORIGINAL BASE DATA, not whole modded
  tables. Native gameplay recipes are exported separately because runtime
  structs differ from serialized records and some changes depend on story state.
- base_reference includes original capk.dat, field_constant.dat, form.bin,
  layout.dat and chara_set.dat, with hashes. These are base reference files,
  explicitly not new files supplied by the mod or prepatched replacements.
- All .dat files are local review exports. No APK/OBB was repacked or installed.

The original repository's *.dat ignore rule remains in place. DAT exports are
included in the ZIP download even though Git ignores them locally.

## Reproduce

    node tools/extract_bundled_assets.mjs --extended --base-data "path/to/raw_obb"

This writes only translations/extended_asset_review. It does not touch the
earlier Japanese/English CSVs. summary.json, per-map metadata and per-table
metadata provide the exact scope and provenance. Underlying game data retains
its original ownership, as distinguished from mod code by the repository LICENSE.
`);
console.log(JSON.stringify({ output: out, ...summary }, null, 2));
}
