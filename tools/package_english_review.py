"""Package the integrated source and the locally built manager, excluding game data."""
from pathlib import Path
import hashlib
import json
import subprocess
import zipfile

ROOT = Path(__file__).resolve().parent.parent
DIST = ROOT / 'out' / 'dist'
VERSION = '1.1.15-en1'


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    DIST.mkdir(parents=True, exist_ok=True)
    tracked = subprocess.check_output(['git', 'ls-files', '-z'], cwd=ROOT).decode().split('\0')
    files = {p for p in tracked if p}
    files.add('ENGLISH_TRANSLATION.md')
    files.add('DISPLAY_4X3.md')
    for directory in ['src', 'tests', 'tools', 'manager/app/src/main']:
        for path in (ROOT / directory).rglob('*'):
            if path.is_file() and path.suffix in {'.h', '.c', '.kt', '.java', '.xml', '.smali', '.mjs', '.py', '.ps1'}:
                files.add(path.relative_to(ROOT).as_posix())
    for name in ['catalog.json', 'japanese_strings.csv', 'internal_references.csv', 'additional_english_drafts.json']:
        files.add('translations/' + name)
    for name in ['message_bank_metadata.json', 'field_overrides.json', 'map_name_overrides.json',
                 'completed_catalog.json', 'mod_related_translations.csv', 'all_completed_translations.csv',
                 'audit_summary.json', 'README.md', 'IMPLEMENTATION.md']:
        files.add('translations/completion_review/' + name)
    source = DIST / f'fflmod-english-{VERSION}-source.zip'
    with zipfile.ZipFile(source, 'w', zipfile.ZIP_DEFLATED) as z:
        for name in sorted(files):
            z.write(ROOT / name, 'ffl_mod/' + name)

    apk = DIST / f'fflmod-manager-{VERSION}-debug.apk'
    status = {
        'version': VERSION,
        'baseline_commit': subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=ROOT).decode().strip(),
        'source_integration': 'implemented; working-tree changes, not committed or published',
        'manager_apk': {'file': apk.name, 'bytes': apk.stat().st_size, 'sha256': sha(apk),
                        'signing': 'local development key; APK v2/v3 verified',
                        'toolchain': 'Kotlin 2.2.20; JDK 21; SDK/platform 36.1; apksig 9.2.1; min24 target34'},
        'host_overlay_tests': 'passed: 16 banks, 161 overrides, 23 native strings, restoration, guards, ELF imports',
        'arm64_c_analysis': 'passed: Clang 19.1.5 compile/null-dereference analysis; not linked',
        'display_tests': 'passed: 11 resolutions, Nova 4:3 viewport, uniform scale, touch/scissor, dialogue/HUD, render targets, aspect transitions and compatibility fallback',
        'save_dialog_static_checks': 'passed: 23 helper call sites and invoke register bounds; not assembled',
        'native_so': 'not built: complete Clang driver unavailable; downloads fail DNS resolution',
        'injected_dex': 'not built: apktool/smali assembler unavailable; downloads fail DNS resolution',
        'android_playtest': 'not performed: ADB server connection failed',
        'remote_ci': 'payload build job prepared; not pushed or run',
        'original_game_data': 'unchanged; no OBB rewrite',
        'source_zip': {'file': source.name, 'bytes': source.stat().st_size, 'sha256': sha(source)},
    }
    report = DIST / 'build_status.json'
    report.write_text(json.dumps(status, indent=2) + '\n', encoding='utf-8')
    review = DIST / f'fflmod-english-review-{VERSION}.zip'
    with zipfile.ZipFile(review, 'w', zipfile.ZIP_DEFLATED) as z:
        for file in [source, apk, report, ROOT / 'ENGLISH_TRANSLATION.md', ROOT / 'DISPLAY_4X3.md']:
            z.write(file, file.name)
    with zipfile.ZipFile(source) as z:
        assert z.testzip() is None
        assert 'ffl_mod/src/localization.h' in z.namelist()
        assert 'ffl_mod/src/english_text.h' in z.namelist()
        assert 'ffl_mod/src/smali/FFLModText.smali' in z.namelist()
        assert 'ffl_mod/translations/completion_review/message_bank_metadata.json' in z.namelist()
        assert all(not n.endswith(('.dat', '.obb', '.so', '.dex', '.jks', '.keystore')) for n in z.namelist())
    with zipfile.ZipFile(review) as z:
        assert z.testzip() is None
    print(json.dumps({'review_zip': str(review), 'bytes': review.stat().st_size,
                      'source_files': len(files), 'apk_sha256': sha(apk)}, indent=2))


if __name__ == '__main__':
    main()
