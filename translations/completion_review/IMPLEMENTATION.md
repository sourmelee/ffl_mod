# Implementing the English translation

Prepared against independent FFLmod 1.1.15, commit
`7fe105ff91aa028860876e3501b7325ede4567e8`.
Source integration now exists in `src/localization.h`, `src/fflmod.c`, manager
resources, and the injected save-dialog helper. The manager development APK has
built successfully. See `../../ENGLISH_TRANSLATION.md` for current validation
and the remaining native/DEX build and Android testing blockers. The detailed
implementation notes below describe the translation data and integration design.

## Which files to use

| File | Purpose |
| --- | --- |
| `mod_related_translations.csv` | Focused review: 136 entries tied to mod source, restored content, or changed behavior |
| `all_completed_translations.csv` | All 352 completed entries, including 216 supplemental base-data entries |
| `field_overrides.json` | 161 concrete story text/speaker slots to override, including copies across banks |
| `map_name_overrides.json` | English name for map-name record 165: Challenge Dungeon |
| `integration/native_english.h` | Static English data for the 23 native display strings, keyed by catalog ID |
| `integration/story_english.h` | Static English strings indexed by bank, message ID, and text/speaker slot |
| `integration/manager_and_save_strings.json` | 68 manager and 19 injected save-dialog entries with source locations |
| `patched_message_dat/index.json` | Original/patched SHA-256 and changed-slot counts for 14 generated game-format files |

In both review CSVs, Japanese remains in column C and English in column D.
The 136 catalog entries and 161 field slots count different things: one passage
can appear in several banks, and the catalog also includes UI strings. The
headers are translation data, not an automatically active patch.

## Recommended route: extend the existing in-memory mod

The mod patches decoded game structures in memory. Its installer changes the
APK by adding native and DEX payloads; it does not need to rewrite the OBB.
Implement English selection in these existing hooks and overlay the restored
message pointers when their bank loads. Japanese and the other game languages
can remain available in the same universal installation.

### 1. Separate language selection from encoding detection

The inspected supported game stores language at `GameClass + 0x0c`:
Japanese is 0 and English is 1. `g_sjis` in `src/fflmod.c` describes text encoding,
not language. UTF-8 does not imply English.

`styx_encoding` currently identifies the Japanese job name at job slot 18 and
leaves `g_sjis == -1` for an unrecognized name. Several features then return
early. Add recognition of the supported English table, while retaining both
Japanese encodings and the surrounding structural checks. Reevaluate language
and any cached table state after a language change or table reload.

| Existing guard | Official English in the inspected data | Required treatment |
| --- | --- | --- |
| Job 18, 魔人 | Magus | Recognize English so Styx initialization can proceed |
| Ability 86, DEBUG用 | For debugging | Accept the English original before replacing this reserved slot |
| Ability 65, メモリーズ | Recollect | Permit the intended cost modification in English |
| Original Slow description | Slows one target. | Recognize the English original before setting the new description |

`internal_references.csv` contains reference translations for these guards.
Do not replace Japanese matching literals globally. Preserve Japanese/SJIS
recognition and add the corresponding English case. Also preserve slot IDs,
counts, donor checks, and other compatibility validation.

### 2. Select English in the native display hooks

Add an English branch to the current Japanese UTF-8/SJIS choices for the
23 `in_game` entries. Use the catalog's locations and symbols to connect the
keys in `native_english.h` to the appropriate assignments. This includes the
Witch job, Styx, Blue Magic, ability descriptions, Alarm, Dash labels, and
the additional configuration page's labels and help text.

Update both the assignments and the already-patched checks. In particular,
`styx_magic` compares description pointers to the expected static strings;
that expected pointer must be the selected English description in English mode.
Otherwise its periodic checks can keep treating translated data as unpatched.
The Slow description hook likewise needs a language-aware replacement.

Styx's initial name is also copied into a saved party-member buffer. Use the
English initial name for new members. For an existing save, only migrate a
recognized mod-default name; preserve player-entered nicknames and buffer bounds.

### 3. Overlay restored dialogue after its message bank is loaded

Use `story_english.h` or `field_overrides.json` after the supported game's
`ReadStoryMessageData` has loaded the active bank and bound its message tables
through `Field::SetMessageList`. Use the bank ID as well as message ID: IDs
alone are not globally unique.

The inspected layout has text and speaker pointer tables at `Field + 0x380`
and `Field + 0x388`. The loader creates separate pointer arrays at
`GameClass + 0x25370` and `+0x25378`, backed by an owning string collection at
`+0x25358`. Override the non-owning active pointer-array entries with the
generated static strings. Leave the owning collection's string pointers alone.
The header's `speaker` flag is 0 for text and 1 for speaker.

Apply only when the selected game language is English, the bank/ID is in range,
and the expected supported data is present. Use `expected_english` from the
JSON for validation: most target slots are originally empty, while four contain
an existing 6 JP award message changed to 7 JP to match the mod's reward.
Keep a compatible idempotent path for entries already pointing to the overlay.
Reapply after bank reloads and restore original pointers or reload the bank
when leaving English. Static strings must remain valid for the entire time
the game uses their pointers.

Do not overwrite a short decoded string buffer with a longer English sentence.
Do not free static replacement strings through the game's owning allocator.
Confirm the pointer-table lifecycle at the actual hook site before wiring it;
the documented offsets are for the inspected supported binary, not a portable ABI.

Apply map-name record 165 separately in the localized map-name table. This is
section 11 of `system_message.dat`, not ordinary system-message entry 519,
which the mod temporarily uses as a configuration-page title.

### 4. Translate the manager and injected save dialogs

For the manager, move the 68 entries to Android string resources with Japanese
defaults and English alternatives, then replace literals at the catalogued
Kotlin/XML locations. Convert Kotlin interpolations into equivalent formatted
arguments when moving them to resources; preserve their expressions and types.
Keep the intended app-language choice explicit: Android device locale and the
game's chosen language may differ.

The 19 save-dialog entries are in DEX/smali injected into the **game APK**.
They cannot reference resource IDs belonging only to the separate manager APK.
Use a bilingual helper in the injected payload, or add and reference resources
in the game APK. Pass or read the intended language consistently; a manager-only
resource change will not translate these dialogs.

### 5. Build and package the fork's payloads

After runtime wiring, the repository's source-only build route is:

```sh
tools/build.sh - out
python3 tools/mkpatch.py out/work out/patch 1.1.15-en1
```

This example requires setting `FFLMOD_VERSION` to the same version first and
having the build script's clang/lld, Java, Python, and apktool dependencies.
The `-` route builds `fflmod.dex` and `libfflmod.so` without taking an APK/OBB
as input. Build the manager separately using its Gradle project and Android SDK.
The example commands were inspected, not executed in this review.

`manager/.../Release.kt` currently requests releases from
`fflmod/ffl_mod`. Point a translated distribution at the fork's release endpoint
(`sourmelee/ffl_mod`) and publish matching payload metadata there when ready.
Regenerate the payload hashes with `mkpatch.py`; retain the installer's original
game compatibility and integrity checks. Preserve upstream attribution.

## Alternative: generated message DAT files

`patched_message_dat` contains 13 story banks plus `system_message.dat`.
They change only the identified mod-related English fields and the four JP
award corrections, not every supplemental translation in the larger catalog.
Other locales and original trailing bytes are preserved. The system-message
section offsets are rebuilt when the English map name grows.

These are useful for inspecting the game format or implementing a file-loader
replacement. A loader must return the replacement buffer and its correct length
with the ownership rules expected by the game. Repacking a local OBB is another
possible integration route, but this review supplies neither an OBB repacker nor
an installed replacement archive. Neither route translates native or Android
UI literals by itself. Prefer the small source-level overlay for distribution;
the full generated DAT files contain original game data.

## Verification before release

The data generator checks translation placeholders/control tags, all 16 story
banks, and event records in 1,679 indexed maps. Generated DATs are reparsed and
all untouched fields and trailing metadata are compared against the originals.
No original data files or tracked runtime sources were modified.

An Android build and visual playtest remain necessary. Check English and
Japanese, language changes, existing saves, Styx/Blue Magic/Alarm, Recollect's
cost, Slow's help text, configuration page 4, all restored moogle entrances,
challenge and ghost-ship dialogue, coordinate/Karma captions, and save dialogs.
Coordinate drafts use short `Lon`/`Lat` labels for 64-pixel captions; verify
actual font fit, wrapping, control tags, and page breaks in-game.

Static coverage is not a full playthrough or proof that every possible runtime
branch has been displayed. The separately inventoried 41 remaining base-data
gaps are original staff-credit blocks; they are outside the identified mod
translation scope and have not been assigned speculative name romanizations.
