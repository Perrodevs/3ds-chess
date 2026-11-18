#!/usr/bin/env python3
"""Build local QA artifacts; complete portraits still need rights clearance."""
from pathlib import Path
import argparse
import hashlib
import json
import os
import re
import zipfile

EXCLUDED = {'.venv', 'build', 'dist', 'private', '.git', '__pycache__'}
ROOT_FILES = {'README.md', 'PROGRESS.md', 'LICENSE', 'Makefile', '.gitignore', '.clang-format'}
ROOT_DIRS = {'source', 'include', 'assets', 'tests', 'tools', 'docs'}


def source_members(root):
    for directory, dirs, files in os.walk(root):
        base = Path(directory).relative_to(root)
        dirs[:] = sorted(d for d in dirs if d not in EXCLUDED and not d.endswith('.dSYM')
                         and (base.parts or d in ROOT_DIRS))
        for name in sorted(files):
            relative = base / name
            if name == '.DS_Store' or (not base.parts and name not in ROOT_FILES):
                continue
            if relative.parts[:2] == ('assets', 'source') and name != 'README.md':
                continue
            yield relative


def create_zip(path, entries):
    temporary = path.with_suffix(path.suffix + '.tmp')
    with zipfile.ZipFile(temporary, 'w', zipfile.ZIP_DEFLATED) as archive:
        for source, name in entries:
            archive.write(source, name)
    with zipfile.ZipFile(temporary) as archive:
        if archive.testzip() is not None:
            raise ValueError('ZIP verification failed')
    temporary.replace(path)


def package(root, local_testing=False):
    if not local_testing:
        raise ValueError('Rights review pending: full artifacts are local QA only. Use --local-testing; do not redistribute.')
    manifest = json.loads((root / 'assets/generated/manifest.json').read_text())
    if not manifest['sprites'].startswith('Original'):
        raise ValueError('Restore original assets first. Personal pieces must stay in private/.')
    match = re.search(r'version = "([^"]+)"', (root / 'include/app_version.hpp').read_text())
    if not match:
        raise ValueError('Missing canonical version')
    version = match[1]
    dist = root / 'dist'
    binary = (dist / '3ds-chess.3dsx').read_bytes()
    smdh = (dist / '3ds-chess.smdh').read_bytes()
    if binary[:4] != b'3DSX' or smdh[:4] != b'SMDH':
        raise ValueError('Invalid binary headers')
    if version.encode() not in binary or version.encode('utf-16le') not in smdh:
        raise ValueError('Stale binary/SMDH: rebuild the canonical version before packaging')
    for required in ('AUDIT_082.md', 'CHANGELOG_082.md', 'LOCAL_TESTING_ONLY.md', 'ASSET_RIGHTS.md'):
        if not (root / 'docs' / required).is_file():
            raise ValueError('Missing required release document: ' + required)
    if version.startswith('0.9.'):
        for required in ('UPDATE_090.md', 'CHANGELOG_090.md', 'PHYSICAL_TESTS_090.md'):
            if not (root / 'docs' / required).is_file():
                raise ValueError('Missing required update document: ' + required)
    if version.startswith('1.1.'):
        for required in ('LEGACY_COMPLETE.md', 'LEGACY_TEST_RESULTS.json', 'LEGACY_COMPOSER.md', 'LEGACY_VARIANTS.md', 'LEGACY_BATTERY.md'):
            if not (root / 'docs' / required).is_file():
                raise ValueError('Missing required Legacy document: ' + required)
    docs = ['README.md', 'LICENSE'] + [str(p.relative_to(root)) for p in sorted((root / 'docs').rglob('*'))
                                     if p.is_file() and p.suffix in {'.md', '.json'}]
    sd = [(dist / f'3ds-chess.{extension}', f'3ds/3ds-chess/3ds-chess.{extension}')
          for extension in ('3dsx', 'smdh')]
    sd += [(root / name, name) for name in docs]
    sd += [(p, str(p.relative_to(root))) for p in sorted((root / 'docs/licenses').iterdir()) if p.is_file()]
    sd += [(root / 'assets/portraits/LICENSE-CC-BY-4.0.txt', 'assets/portraits/LICENSE-CC-BY-4.0.txt')]
    create_zip(dist / '3ds-chess-sd.zip', sd)
    create_zip(dist / '3ds-chess-source.zip', [(root / p, '3ds-chess/' + str(p)) for p in source_members(root)])
    files = [dist / name for name in ('3ds-chess.3dsx', '3ds-chess.smdh', '3ds-chess-sd.zip', '3ds-chess-source.zip')]
    metadata = {'version': version, 'distribution': 'LOCAL TESTING ONLY - NOT CLEARED FOR REDISTRIBUTION',
                'rights_inventory': 'docs/ASSET_RIGHTS.md',
                'physical_hardware_tested': False,
                'sha256': {p.name: hashlib.sha256(p.read_bytes()).hexdigest() for p in files}}
    (dist / 'RELEASE.json').write_text(json.dumps(metadata, indent=2) + '\n')
    files.append(dist / 'RELEASE.json')
    (dist / 'SHA256SUMS').write_text(''.join(hashlib.sha256(p.read_bytes()).hexdigest() + '  ' + p.name + '\n' for p in files))
    for p in files:
        print(p.name, p.stat().st_size, 'bytes')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--local-testing', action='store_true', help='Build private local QA artifacts with rights-review notices')
    args = parser.parse_args()
    try:
        package(Path(__file__).resolve().parents[1], args.local_testing)
    except (ValueError, OSError) as error:
        parser.exit(1, str(error) + '\n')
