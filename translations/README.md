# Japanese text supplied by FFLmod

This translation workspace is for the independent FFLmod project. The original
source inventory is below. The expanded review also resolves restored dialogue,
map labels, and missing-English base-data passages using local data read-only.
See `completion_review/README.md` for the completed catalogs and
`completion_review/IMPLEMENTATION.md` for insertion and build guidance.
Runtime integration has since been added to this fork; see
`../ENGLISH_TRANSLATION.md` for its build and test status. Original game data
and FFSeraph runtime sources remain untouched.

## Translate individual entries

Open `japanese_strings.csv` with a UTF-8-capable spreadsheet or text editor and
fill the `english` column. Keep IDs, Japanese originals, and the other columns
unchanged. Save as UTF-8 CSV. English cells now contain draft translations for
you to review and revise. Preserve placeholders such as `$name`, `${expression}`,
`%d`, and line breaks. A `${expression}` may contain quoted Kotlin code; leave
that expression intact.

| Category | Unique entries | Meaning |
| --- | ---: | --- |
| in_game | 23 | Native mod-added names, labels, ability descriptions, and help |
| manager | 68 | Manager UI, messages, errors, and application label |
| save_dialog | 19 | Injected save import/export UI and messages |

Equivalent UTF-8 and Shift-JIS spellings share one entry; all source locations
and encodings are retained. Entries are grouped by exact Japanese text within
each category. Repeated occurrences may need different wording after contextual
review. Locations refer to the source as inventoried, initially commit
`7fe105ff91aa028860876e3501b7325ede4567e8`.

`internal_references.csv` contains four Japanese matching/encoding checks and
one diagnostic message. These are separate from the translation queue:

- `単体の行動を遅くする`: original Slow description used for matching.
- `魔人`: job name used to recognize text encoding.
- `DEBUG用`: existing command name used to validate the slot being replaced.
- `メモリーズ`: existing ability name used to validate a cost modification.
- The remaining entry is a log message about that ability check.

Do not replace these checks with English display text. Their eventual handling
needs to preserve the matching logic.

## Coverage and limitations

The extractor scans native C/header string literals, Kotlin/Java string
literals, smali `const-string` instructions, and XML application labels under
`src/` and `manager/app/src/main/`. It excludes comments and documentation.
The native scanner joins adjacent C literals and decodes UTF-8 or Shift-JIS.

Generated numeric script/image arrays are not interpreted as text by this tool.
The catalog is an inventory of direct Japanese source strings. A separate
inspection decoded the bundled field-script arrays and resolved six Japanese-only
moogle passages plus one speaker label from existing base data; see
`asset_review/README.md` and `asset_review/restored_dialogue.csv` for the extracted
text and draft English. These entries are separate from the 110 source strings.
Text referenced by a message ID or copied from the base game is not newly
supplied literal text and is not extracted by the source-string tool.
For example, `jp_text_fix` changes a digit in an existing game message; it does
not supply a new full dialogue string.

## Refresh the inventory

From the repository root, with Node.js available:

```sh
node tools/extract_translation_catalog.mjs
```

The script preserves existing CSV English cells by stable ID and refreshes
`catalog.json`, a machine-readable companion containing individual occurrences.
Commit or back up your CSV before refreshing against upstream changes: changed
or removed Japanese entries receive new IDs or disappear from the new inventory.
Edit the CSV rather than the generated JSON. Extra CSV columns are not retained.

This extraction does not apply translations to the mod. Insertion and language
selection can follow once individual translations have been reviewed.
