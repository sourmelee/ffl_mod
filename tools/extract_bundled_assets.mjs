// Extract the mod's embedded payloads and optionally reconstruct graphics from
// an existing raw OBB extraction. Never writes to the base data or game archive.
import fs from 'node:fs';
import path from 'node:path';
import zlib from 'node:zlib';
import assert from 'node:assert/strict';
import crypto from 'node:crypto';
import { fileURLToPath } from 'node:url';

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const extended = process.argv.includes('--extended');
const out = path.join(root, 'translations', extended ? 'extended_asset_review' : 'asset_review');
const arg = process.argv.indexOf('--base-data');
const base = arg >= 0 ? path.resolve(process.argv[arg + 1]) : null;
const read = name => fs.readFileSync(path.join(root, 'src', name));
const source = name => read(name).toString('utf8');
const sha = b => crypto.createHash('sha256').update(b).digest('hex');
const fnv = b => { let h = 0x811c9dc5; for (const c of b) h = Math.imul(h ^ c, 0x01000193) >>> 0; return h; };
const hex = n => '0x' + n.toString(16);
function save(name, data) {
  const p = path.join(out, name); fs.mkdirSync(path.dirname(p), { recursive: true }); fs.writeFileSync(p, data);
}
function json(name, value) { save(name, JSON.stringify(value, null, 2) + '\n'); }
function array(s, name) {
  const m = s.match(new RegExp('\\b' + name + '\\[(\\d+)\\]\\s*=\\s*\\{([^}]+)\\}'));
  assert.ok(m, name);
  const b = Buffer.from([...m[2].matchAll(/0x[0-9a-f]+|\b\d+\b/gi)].map(x => Number(x[0])));
  assert.equal(b.length, +m[1], name + ' length'); return b;
}
function escapedBytes(s) { return Buffer.from([...s.matchAll(/\\x([0-9a-f]{2})/gi)].map(m => parseInt(m[1], 16))); }

// Minimal lossless PNG reader/writer. Source ICP wrappers use 8-bit RGB/RGBA,
// non-interlaced PNGs; all five PNG row filters are supported.
const crcTable = Array.from({ length: 256 }, (_, x) => {
  for (let k = 0; k < 8; k++) x = (x & 1) ? 0xedb88320 ^ (x >>> 1) : x >>> 1; return x >>> 0;
});
function crc(b) { let c = 0xffffffff; for (const x of b) c = crcTable[(c ^ x) & 255] ^ (c >>> 8); return (c ^ 0xffffffff) >>> 0; }
function chunk(tag, data) {
  const b = Buffer.alloc(12 + data.length); b.writeUInt32BE(data.length); b.write(tag, 4); data.copy(b, 8);
  b.writeUInt32BE(crc(b.subarray(4, -4)), b.length - 4); return b;
}
function png(w, h, rgba) {
  assert.equal(rgba.length, w * h * 4);
  const ihdr = Buffer.alloc(13); ihdr.writeUInt32BE(w); ihdr.writeUInt32BE(h, 4); ihdr[8] = 8; ihdr[9] = 6;
  const scan = Buffer.alloc((w * 4 + 1) * h);
  for (let y = 0; y < h; y++) rgba.copy(scan, y * (w * 4 + 1) + 1, y * w * 4, (y + 1) * w * 4);
  return Buffer.concat([Buffer.from('89504e470d0a1a0a', 'hex'), chunk('IHDR', ihdr), chunk('IDAT', zlib.deflateSync(scan)), chunk('IEND', Buffer.alloc(0))]);
}
function unpng(b) {
  assert.equal(b.subarray(0, 8).toString('hex'), '89504e470d0a1a0a');
  let w, h, channels; const idats = [];
  for (let p = 8; p < b.length;) {
    const n = b.readUInt32BE(p), tag = b.toString('ascii', p + 4, p + 8), d = b.subarray(p + 8, p + 8 + n);
    assert.equal(crc(b.subarray(p + 4, p + 8 + n)), b.readUInt32BE(p + 8 + n), 'PNG CRC');
    if (tag === 'IHDR') {
      w = d.readUInt32BE(0); h = d.readUInt32BE(4); assert.equal(d[8], 8); assert.equal(d[12], 0);
      assert.ok([2, 6].includes(d[9])); channels = d[9] === 6 ? 4 : 3;
    } else if (tag === 'IDAT') idats.push(d);
    p += n + 12;
  }
  const raw = zlib.inflateSync(Buffer.concat(idats)), stride = w * channels, pixels = Buffer.alloc(w * h * channels);
  assert.equal(raw.length, (stride + 1) * h);
  for (let y = 0; y < h; y++) {
    const filter = raw[y * (stride + 1)]; assert.ok(filter <= 4);
    for (let x = 0; x < stride; x++) {
      const i = y * stride + x, a = x >= channels ? pixels[i - channels] : 0, b = y ? pixels[i - stride] : 0;
      const c = y && x >= channels ? pixels[i - stride - channels] : 0;
      const p = a + b - c, pa = Math.abs(p - a), pb = Math.abs(p - b), pc = Math.abs(p - c);
      const predict = [0, a, b, Math.floor((a + b) / 2), pa <= pb && pa <= pc ? a : pb <= pc ? b : c][filter];
      pixels[i] = raw[y * (stride + 1) + 1 + x] + predict;
    }
  }
  return { w, h, channels, pixels };
}
function icp(name) {
  assert.ok(base, 'Base data needed for ' + name);
  const b = fs.readFileSync(path.join(base, name)); assert.equal(b.toString('ascii', 0, 3), 'ICP');
  const w = b.readUInt16LE(8), h = b.readUInt16LE(10), inner = unpng(b.subarray(12));
  const rgb = Buffer.alloc(inner.w * inner.h * 3);
  for (let i = 0; i < inner.w * inner.h; i++) inner.pixels.copy(rgb, i * 3, i * inner.channels, i * inner.channels + 3);
  assert.equal(rgb.subarray(0, 3).toString('hex'), '020100', 'ICP channel marker');
  const packed = Buffer.alloc(rgb.length - 3);
  for (let i = 0; i + 5 < rgb.length; i += 3) { packed[i] = rgb[i + 5]; packed[i + 1] = rgb[i + 4]; packed[i + 2] = rgb[i + 3]; }
  const payload = packed.subarray(1); assert.ok(payload.length >= 1024 + w * h);
  return { w, h, palette: Buffer.from(payload.subarray(0, 1024)), idx: Buffer.from(payload.subarray(1024, 1024 + w * h)), source: name, sha256: sha(b) };
}
function rgba(idx, pal) {
  const b = Buffer.alloc(idx.length * 4); for (let i = 0; i < idx.length; i++) pal.copy(b, i * 4, idx[i] * 4, idx[i] * 4 + 4); return b;
}
function indexed(name, w, h, idx, pal) { save(name, png(w, h, rgba(idx, pal))); }
function scale(name, w, h, pixels, factor) {
  const b = Buffer.alloc(w * h * factor * factor * 4);
  for (let y = 0; y < h * factor; y++) for (let x = 0; x < w * factor; x++) {
    const src = (Math.floor(y / factor) * w + Math.floor(x / factor)) * 4;
    pixels.copy(b, (y * w * factor + x) * 4, src, src + 4);
  }
  save(name, png(w * factor, h * factor, b));
}

if (!extended) {
const manifest = { source_commit: '7fe105ff91aa028860876e3501b7325ede4567e8', obb_written: false,
  base_data: base, payloads: [], sprite_patches: [], jobs: [], message_references: [], verified: [] };
for (const name of ['sprite_patch.bin', 'styx_tab.h', 'recolor_tab.h', 'jump_tab.h', 'moogle_tab.h', 'cd_tab.h', 'bsc_tab.h', 'alarm_tab.h']) {
  const b = read(name); manifest.payloads.push({ file: 'src/' + name, bytes: b.length, sha256: sha(b) });
}
const styx = source('styx_tab.h');
const pal = escapedBytes(styx.match(/styx_pal\[1025\]\s*=\s*"([^"]*)"/)[1]); assert.equal(pal.length, 1024); assert.equal(fnv(pal), 0xf0b262c2);
const rows = array(styx, 'styx_rows'), ext = array(styx, 'styx_ext');
save('raw/styx_palette.rgba.bin', pal); save('raw/styx_added_rows.idx.bin', rows); save('raw/styx_extra_sheet.idx.bin', ext);
indexed('graphics/styx_added_rows.png', 256, 150, rows, pal);
indexed('graphics/styx_extra_sheet.png', 256, 64, ext, pal);
const styxPreview = Buffer.alloc(256 * 214 * 4); rgba(rows, pal).copy(styxPreview); rgba(ext, pal).copy(styxPreview, 256 * 150 * 4);
scale('graphics/styx_preview_3x.png', 256, 214, styxPreview, 3);
if (base) {
  const original = icp('fldchr120_0.dat'); assert.equal(fnv(original.palette), 0xf0b262c2);
  assert.ok(original.idx.subarray(150 * 256, 300 * 256).every(x => x === 0)); rows.copy(original.idx, 150 * 256);
  indexed('graphics/styx_full_patched.png', original.w, original.h, original.idx, pal);
  manifest.verified.push('Styx base palette hash matched; target rows 150-299 were blank before insertion.');
}

const recolors = [...source('recolor_tab.h').matchAll(/\{ (\d+), (\d+), (\d+), \/\*([^*]*)\*\/\s*"((?:\\x[0-9a-f]{2})+)" \}/g)].map(m =>
  ({ model: +m[1], chara: +m[2], donor: +m[3], source_label: m[4].trim(), pal: escapedBytes(m[5]) }));
assert.equal(recolors.length, 40);
for (const r of recolors) { assert.equal(r.pal.length, 1024); save(`raw/palettes/model${r.model}_chara${r.chara}.rgba.bin`, r.pal); }
const data = read('sprite_patch.bin'); assert.equal(data.toString('ascii', 0, 4), 'FSP1');
const count = data.readUInt32LE(4), patches = new Map();
for (let i = 0; i < count; i++) {
  const p = 8 + i * 20, model = data[p], chara = data[p + 1], sub = data[p + 2], added = data[p + 3];
  const w = data.readUInt16LE(p + 4), h = data.readUInt16LE(p + 6), expected = data.readUInt32LE(p + 8);
  const off = data.readUInt32LE(p + 12), len = data.readUInt32LE(p + 16); assert.ok(off >= 8 + count * 20 && off + len <= data.length);
  const colors = [], runs = []; let cursor = off;
  for (let k = 0; k < added; k++, cursor += 5) colors.push({ index: data[cursor], rgba: [...data.subarray(cursor + 1, cursor + 5)] });
  while (cursor < off + len) {
    assert.ok(cursor + 6 <= off + len); const start = data.readUInt32LE(cursor), n = data.readUInt16LE(cursor + 4); cursor += 6;
    assert.ok(start + n <= w * h && cursor + n <= off + len); runs.push({ start, indices: data.subarray(cursor, cursor + n) }); cursor += n;
  }
  assert.equal(cursor, off + len);
  const patch = { model, chara, sub, w, h, expected, colors, runs };
  patches.set(`${model}/${chara}/${sub}`, patch);
  const r = recolors.find(r => r.model === model && r.chara === chara); assert.ok(r);
  const palette = Buffer.from(r.pal); for (const c of colors) Buffer.from(c.rgba).copy(palette, c.index * 4);
  const pixels = Buffer.alloc(w * h * 4);
  for (const run of runs) rgba(run.indices, palette).copy(pixels, run.start * 4);
  const stem = `model${model}_chara${chara}_sub${sub}`;
  save(`graphics/deltas/${stem}.png`, png(w, h, pixels)); save(`raw/patches/${stem}.bin`, data.subarray(off, off + len));
  manifest.sprite_patches.push({ model, chara, sub, width: w, height: h, expected_index_fnv: hex(expected), bytes: len,
    file_offset: off, added_colors: colors, run_count: runs.length, changed_pixels: runs.reduce((n, r) => n + r.indices.length, 0) });
}
assert.equal(count, 74);
let payloadEnd = 8 + count * 20;
for (const p of [...manifest.sprite_patches].sort((a, b) => a.file_offset - b.file_offset)) {
  assert.equal(p.file_offset, payloadEnd, 'Unaccounted bytes in sprite_patch.bin'); payloadEnd += p.bytes;
}
assert.equal(payloadEnd, data.length, 'Trailing bytes in sprite_patch.bin');
const names = ['Sol', 'Aigis', 'Dusk', 'Sarah', 'Nacht', 'Alba', 'Diana', 'Glaive'];
const jobs = { 40: 'Dragoon', 41: 'Ranger', 42: 'Bard', 43: 'Dark Knight', 44: 'Memorist', 45: 'Dancer', 46: 'Paladin', 47: 'Ninja', 48: 'Seer', 49: 'Magus' };
const contact = Buffer.alloc(5 * 110 * 8 * 110 * 4);
let matched = 0;
for (const [index, r] of recolors.entries()) {
  const sheets = [];
  for (const sub of [0, 1]) {
    const stem = `model${r.model}_chara${r.chara}_sub${sub}`;
    if (!base) continue;
    const original = icp(`fldchr${r.model}_${r.donor}${sub ? '_1' : ''}.dat`), palette = Buffer.from(r.pal), idx = Buffer.from(original.idx);
    const patch = patches.get(`${r.model}/${r.chara}/${sub}`);
    if (patch) {
      assert.equal(original.w, patch.w); assert.equal(original.h, patch.h);
      assert.equal(fnv(original.idx), patch.expected, 'Sprite donor differs: ' + stem); matched++;
      for (const c of patch.colors) Buffer.from(c.rgba).copy(palette, c.index * 4);
      for (const run of patch.runs) run.indices.copy(idx, run.start);
    }
    indexed(`graphics/jobs/${stem}.png`, original.w, original.h, idx, palette);
    sheets.push({ sub, donor_file: original.source, donor_sha256: original.sha256, patched_pixels: !!patch, image: `graphics/jobs/${stem}.png` });
    if (sub === 0) {
      const pixels = rgba(idx, palette), dx = (index % 5) * 110 + 5, dy = Math.floor(index / 5) * 110 + 5;
      for (let y = 0; y < 100; y++) for (let x = 0; x < 100; x++) {
        const s = (Math.floor(y / 2) * original.w + Math.floor(x / 2)) * 4;
        pixels.copy(contact, ((dy + y) * 550 + dx + x) * 4, s, s + 4);
      }
    }
  }
  manifest.jobs.push({ model: r.model, chara: r.chara, character: names[r.chara], job: jobs[r.model], donor: names[r.donor], source_label: r.source_label, sheets });
}
if (base) { save('graphics/job_overview_2x.png', png(550, 880, contact)); assert.equal(matched, 74); manifest.verified.push('All 74 sprite patch donor index hashes matched; 80 job sheets reconstructed.'); }
const jump = array(source('jump_tab.h'), 'jump_tile_idx'); save('raw/jump_pad_tile.idx.bin', jump);
if (base) {
  const mc = icp('mc9_0.dat'); assert.equal(fnv(mc.palette), 0x801d2385);
  indexed('graphics/jump_pad_tile.png', 32, 32, jump, mc.palette); scale('graphics/jump_pad_preview_8x.png', 32, 32, rgba(jump, mc.palette), 8);
  manifest.verified.push('Jump-pad palette hash matched mc9_0.dat.');
}

const scriptSets = [];
const opNames = { 0: 'SetMessage', 3: 'SetReferenceVariable', 4: 'SetReferenceFlag', 42: 'SetFade', 50: 'WaitFrames', 54: 'PlaySE', 59: 'YesNoDialog', 61: 'ScriptIf', 63: 'Jump', 87: 'ScriptEnd', 102: 'CallEvent', 104: 'StartEntityAction', 105: 'WaitEntityAction', 107: 'BulkSetVars' };
for (const [header, wanted] of [['moogle_tab.h', ['moogle_script_7', 'moogle_script_8', 'moogle_script_9']], ['cd_tab.h', ['dawn_chest_script', 'dawn_chest_first']]]) {
  for (const name of wanted) {
    const bytes = array(source(header), name), commands = []; let p = 0;
    while (p < bytes.length) {
      const offset = p, n = bytes[p++]; assert.ok(p + n <= bytes.length); const c = bytes.subarray(p, p + n); p += n;
      const cmd = { block: commands.length, offset, length: n, opcode: hex(c[0]), name: opNames[c[0]] ?? 'Other', bytes: c.toString('hex') };
      if ([0, 0x3b].includes(c[0])) { cmd.message_id = c.readUInt16BE(1); manifest.message_references.push({ script: name, block: cmd.block, opcode: cmd.opcode, message_id: cmd.message_id }); }
      commands.push(cmd);
    }
    const expected = name.startsWith('moogle') ? 44 : name === 'dawn_chest_script' ? 75 : 1; assert.equal(commands.length, expected);
    save(`raw/scripts/${name}.bin`, bytes); json(`scripts/${name}.json`, commands); scriptSets.push({ name, bytes: bytes.length, blocks: commands.length });
  }
}
manifest.script_sets = scriptSets;
for (const name of ['bsc_469_orig', 'bsc_437_orig', 'bsc_80', 'bsc_81']) save(`raw/scripts/${name}.bin`, array(source('bsc_tab.h'), name));
manifest.verified.push('All five length-prefixed field script arrays were parsed to exact byte boundaries (208 blocks total). Their commands contain numeric operands/message references, not inline dialogue.');
json('scripts/message_references.json', manifest.message_references);

const dialogue = new Map();
if (base) for (const bank of [7, 8, 9]) {
  const file = `msg${bank}.dat`, b = fs.readFileSync(path.join(base, file)), count = b.readUInt16BE(0); let p = 2;
  const ids = new Set(manifest.message_references.filter(r => r.script === `moogle_script_${bank}`).map(r => r.message_id));
  for (let id = 0; id <= Math.max(...ids); id++) {
    const slots = [];
    for (let slot = 0; slot < 12; slot++) {
      assert.ok(p + 2 <= b.length); const len = b.readUInt16BE(p); p += 2; assert.ok(p + len < b.length);
      slots.push(new TextDecoder('utf-8', { fatal: true }).decode(b.subarray(p, p + len))); p += len; assert.equal(b[p++], 0);
    }
    if (!ids.has(id)) continue;
    assert.ok(id < count);
    const key = slots[0], entry = dialogue.get(key) ?? { id: `restored-dialogue-${id}`, japanese: slots[0], english_existing: slots[2], english_draft: '', references: [], speakers: [] };
    entry.references.push(`${file}:message ${id}`); entry.speakers.push({ file, japanese: slots[1], english: slots[3] }); dialogue.set(key, entry);
  }
}
const drafts = {
  33: '"I am a moogle who can guide you\nto even greater challenges, kupo!\nOnce you have made a little more\nprogress on your adventure,\nbe sure to come see me again!"',
  34: '"Kupopo! I have been waiting for you!\n\nI can guide you to even\ngreater challenges, kupo.',
  35: 'So, would you like to take on\nthe Challenge Dungeon, kupo?"',
  36: '"Understood, kupo!\nTake care, and good luck!"',
  37: '"Kupopo... What a shame.\nIf you change your mind,\ncome and talk to me again, kupo!"',
  38: '"I have been waiting for you, kupo!\nWould you like to take on\nthe Challenge Dungeon?"'
};
for (const entry of dialogue.values()) entry.english_draft = drafts[Number(entry.id.split('-').at(-1))] ?? '';
json('dialogue.json', [...dialogue.values()]);
const quote = s => '"' + String(s).replaceAll('"', '""') + '"';
const speakerRefs = [...dialogue.values()].flatMap(d => d.speakers.filter(s => s.japanese === 'モーグリ' && !s.english).map(s => `${s.file}:message ${d.id.split('-').at(-1)} speaker`));
const csvRows = [['id', 'category', 'japanese', 'english', 'references', 'existing_english', 'provenance'],
  ...[...dialogue.values()].map(d => [d.id, 'restored_dialogue', d.japanese, d.english_draft, d.references.join(' | '), d.english_existing, 'Existing base-game message activated by bundled mod scripts']),
  ...(speakerRefs.length ? [['restored-speaker-moogle', 'restored_speaker', 'モーグリ', 'Moogle', speakerRefs.join(' | '), '', 'Existing base-game speaker label; English slot empty']] : [])];
save('restored_dialogue.csv', '\ufeff' + csvRows.map(r => r.map(quote).join(',')).join('\r\n') + '\r\n');
manifest.dialogue = { unique_texts: dialogue.size, reference_count: manifest.message_references.length, english_missing: [...dialogue.values()].filter(d => !d.english_existing).length, speaker_labels: speakerRefs.length ? 1 : 0,
  provenance: 'Resolved against existing raw base-game data. These messages are not authored or stored in the mod payloads.' };
json('manifest.json', manifest);
save('README.md', `# FFLmod bundled asset and dialogue extraction

Inspected source: commit ${manifest.source_commit}, mod version 1.1.15.

## What changes the OBB?

The supplied installer does not rewrite the OBB. tools/fflmod_apk.py and the
manager's ApkBuilder.java add the mod DEX and native library to APKs and replace
the manifest. The native library hooks resource-loading functions and changes
decoded graphics, event scripts, and tables in memory.

The additions below therefore affect resources originally read from the OBB,
without becoming new files in the OBB itself. No game archive was modified.

## Extracted graphics

| Bundled source | Content | Output |
| --- | --- | --- |
| src/styx_tab.h | 256-color RGBA palette; 256x150 replacement battle rows; 256x64 extra sheet | graphics/styx_added_rows.png, styx_extra_sheet.png, enlarged preview |
| src/recolor_tab.h | 40 job/character recolor palettes | raw/palettes/ and reconstructed job sheets |
| src/sprite_patch.bin | 74 FSP1 patch records for 37 job/character pairs, two sheets each | raw/patches/, graphics/deltas/, manifest.json |
| src/jump_tab.h | 32x32 replacement jump-pad tile indices | raw/jump_pad_tile.idx.bin and colored PNG previews |

${base ? `80 complete job sheets were reconstructed (two sheets for each of 40 pairs),
including palette-only pairs without pixel patches. All 74 expected donor-index
FNV hashes matched the existing local raw base assets. The Styx palette and
jump-pad palette hashes also matched. Styx's target battle rows were verified
blank before insertion. These complete sheets combine base-game pixels with
the mod's changes; they are not 80 wholly new images authored by the mod.
` : 'Without base data, sparse pixel changes and standalone Styx graphics are available, but complete job sprites cannot be reconstructed.'}

The FSP1 table and every patch run were decoded with bounds checks. The table
and payloads account for every byte of sprite_patch.bin. Delta PNGs show only
the pixels written by each patch; transparent pixels mean unchanged pixels,
and are not a complete character image. No text was visible in the inspected
Styx, job-overview, or jump-pad previews.

Open gallery.html for labeled job sheets, Styx previews, and extracted dialogue.
PNG files preserve native pixels; enlarged previews use nearest-neighbor scaling.

## Dialogue discovered outside the direct-source catalog

All five length-prefixed field-script arrays in moogle_tab.h and cd_tab.h were
decoded: three 44-block moogle scripts, a 75-block chest script, and an 18-byte
one-block matching pattern (208 blocks total). They contain numeric instructions,
not embedded dialogue strings. Four battle-script arrays from bsc_tab.h were
also extracted as binary files; they are battle VM code, not field message banks.

The three replacement moogle scripts reference message IDs 33-38 (0x21-0x26)
in story banks 7, 8, and 9: 18 message-reference sites, six distinct Japanese
passages. This is pre-existing dormant base-game text that the mod activates.
In the inspected local base assets, all 18 English dialogue slots were empty.
An additional Japanese speaker label, モーグリ (Moogle), also lacked English
where present. The dialogue covers the dungeon invitation, eligibility notice,
yes/no prompt, departure, refusal, and repeat invitation.

restored_dialogue.csv contains six passages plus the speaker label, with
Japanese in column C and draft English in column D. Its English text is newly
drafted for review; it is not an official translation retrieved from the game.
The existing 110-entry source-string CSV has been left unchanged.

dialogue.json preserves per-bank speaker information. scripts/message_references.json
links each reference to its replacement script and block number. The full
decoded field command bytes are in scripts/*.json.

This confirms a translation gap missed by a source-literal scan. It does not
claim that every dormant challenge-dungeon message in the whole game has been
audited. Only the references in these bundled replacement script arrays were
resolved. Other restored map events may refer to additional base-game text.

## Reproduce

From the repository root:

    node tools/extract_bundled_assets.mjs --base-data "path/to/existing/raw_obb"

Omit --base-data to extract only self-contained mod data. The base folder is
read-only. Regeneration replaces extraction outputs, including the draft
dialogue CSV, so retain reviewed translations before rerunning it.

manifest.json records source payload hashes, per-patch sizes, donor hashes,
dimensions, message references, and checks. The mod source's MIT license
excludes the underlying game-data fragments; see the repository LICENSE.
`);
const escape = s => String(s).replaceAll('&', '&amp;').replaceAll('<', '&lt;').replaceAll('>', '&gt;').replaceAll('"', '&quot;');
save('gallery.html', `<!doctype html><html lang="en"><meta charset="utf-8"><title>FFLmod extracted assets</title>
<style>body{background:#15191e;color:#edf0f4;font:16px system-ui;margin:32px;max-width:1400px}h1,h2{color:#eab65f}img{image-rendering:pixelated;background:repeating-conic-gradient(#28323c 0% 25%,#34414f 0% 50%) 0/16px 16px;max-width:100%}.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(230px,1fr));gap:20px}figure{margin:0;padding:16px;background:#202831;border-radius:8px}figure img{width:100%}figcaption{margin-bottom:12px}pre{white-space:pre-wrap;padding:16px;background:#202831}</style>
<h1>FFLmod extracted assets</h1><p>Source commit ${manifest.source_commit}. The mod changes decoded resources in memory; it does not rewrite the OBB. Native-size PNGs, raw patch bytes, and provenance are included.</p>
<h2>Styx added battle poses</h2><img src="graphics/styx_preview_3x.png" alt="Styx added rows and extra sheet, enlarged three times">
${base ? '<h2>Restored jump-pad tile</h2><img src="graphics/jump_pad_preview_8x.png" alt="Jump-pad tile enlarged eight times">' : ''}
<h2>Job sprites</h2><p>${base ? 'Complete sheets reconstructed from hash-verified base data, recolor palettes, and applicable pixel patches.' : 'Only sparse delta graphics could be extracted without base data.'}</p><div class="grid">${manifest.jobs.map(j => `<figure><figcaption>${escape(j.character)} — ${escape(j.job)}</figcaption>${j.sheets.map(s => `<a href="${s.image}"><img src="${s.image}" alt="${escape(j.character)} ${escape(j.job)} sheet ${s.sub}"></a>`).join('')}</figure>`).join('')}</div>
<h2>Restored dialogue</h2><p>Existing base-game Japanese text referenced by the mod's replacement moogle scripts. The English slots in the inspected banks are empty. English below is a draft, not an existing official translation.</p>${[...dialogue.values()].map(d => `<h3>${escape(d.references.join('; '))}</h3><pre>${escape(d.japanese)}</pre><pre>${escape(d.english_draft)}</pre>`).join('')}
<p>See manifest.json for hashes and extraction checks. Modified game data is distinct from the MIT-licensed mod code, as noted in the repository LICENSE.</p></html>`);
console.log(JSON.stringify({ output: out, patches: count, reconstructed_sheets: manifest.jobs.reduce((n, j) => n + j.sheets.length, 0), scripts: scriptSets, dialogue: manifest.dialogue, verified: manifest.verified }, null, 2));
}
if (extended) await import('./extract_extended_assets.mjs').then(m => m.extractExtended({ root, out, base, save, json, source, read, sha, fnv, array, icp, rgba, png }));
