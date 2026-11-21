# Expanded translation review

The initial source-string inventory did not cover everything. This review adds
text reached indirectly through restored maps and numeric message references,
and searches all 16 story banks for Japanese entries with missing English.

| Completed category | Entries |
| --- | ---: |
| Original mod source strings: native 23, manager 68, save dialogs 19 | 110 |
| Missing-English base story passages: 158 official matches, 55 new drafts | 213 |
| Speaker entries, including preserved control tokens | 24 |
| Challenge Dungeon map name | 1 |
| English award messages corrected from 6 JP to the mod's 7 JP | 4 |
| Total completed catalog | 352 |

`mod_related_translations.csv` contains the focused 136-entry subset.
`all_completed_translations.csv` includes 216 supplemental base-data entries
whose connection to restored mod content was not established. They are marked
by `scope`; they should not be described as 216 additional mod features.
Both CSVs keep Japanese in C and English in D. Official matches carry provenance;
new English remains a draft for contextual review.

The static scan covered 18,384 story records, 1,679 indexed maps, 18,833 events,
and 166,863 commands. It found 11 mod-recognized moogle entrance sites and
243 text-reference sites in the identified mod-related scope. No missing English
remains among the identified mod-related entries, and the independent source
literal sweep found no uncovered Japanese literal lines. See `audit_summary.json`.

The 41 remaining missing-English passages are original staff-credit blocks
in story bank 14. They are separately inventoried in
`remaining_original_base_gaps.csv` and are not part of the completed catalog.
Names were not assigned speculative romanizations.

For implementation, start with `IMPLEMENTATION.md` and the current status in
`../../ENGLISH_TRANSLATION.md`. Runtime wiring now uses generated
`src/english_text.h`; the integration headers here remain reference exports.
The 14 optional patched DAT files preserve every other language and untouched
field. Their hashes and change counts are recorded in `patched_message_dat/index.json`.
The original data folder remains unchanged. A manager development APK has been
built; native/DEX linking, Android installation, and in-game layout/playthrough
verification remain blocked as detailed in the current integration status.

The earlier asset reviews still supply sprite sheets, map previews, tables,
script bytes, and extracted DAT files. The current CSV/JSON translation data
supersedes the drafts bundled in the earlier extraction ZIPs.

The following command reproduced this audit on the unlocalized baseline after
the two asset extraction passes:

```sh
node tools/complete_translation_review.mjs --base-data ../../Android/raw_obb
```

On that baseline it regenerated the completed catalogs, integration headers, review DATs,
and audit JSON. It also updates the older dialogue CSV/JSON/gallery views.
The original 110-entry source CSV and `additional_english_drafts.json` provide
editable inputs. Edits only to generated completion CSVs are not preserved
when regenerating; merge approved edits into those inputs first. On the
integrated fork, use `tools/generate_localization.mjs` as documented in the
current integration status; the baseline audit tools stop before overwriting
the catalog after resource migration.
