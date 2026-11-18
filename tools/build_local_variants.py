#!/usr/bin/env python3
"""Build reproducible local QA variants, always restoring the original piece atlas."""
from pathlib import Path
import argparse
import hashlib
import shutil
import subprocess
root=Path(__file__).resolve().parents[1]
log_directory=root/'build/update-090'

def run(args, name):
    path=log_directory/name
    path.parent.mkdir(parents=True,exist_ok=True)
    with path.open('w') as log:
        result=subprocess.run(args,cwd=root,stdout=log,stderr=subprocess.STDOUT)
    if result.returncode:
        raise RuntimeError(f'{args[0]} failed ({result.returncode}); see {path}')

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--personal',action='store_true',help='Also build the existing private Cosunosuke variant')
    parser.add_argument('--local-testing',action='store_true',required=True,help='Required: artifacts are not cleared for public redistribution')
    parser.add_argument('--log-dir', default='build/update-090', help='Directory for build logs relative to the project')
    args=parser.parse_args()
    log_directory=root/args.log_dir
    python=str(root/'.venv/bin/python')
    if args.personal and not (root/'assets/source/8 Bit Chess.zip').is_file():
        parser.error('Personal source pack is unavailable; no substitute was generated')
    try:
        if args.personal:
            run([python,'tools/prepare_assets.py','--personal-pack'],'personal-assets.log')
            run(['make','-j3','all'],'personal-build.log')
            run(['make','test-target'],'personal-target.log')
            (root/'private').mkdir(exist_ok=True)
            for suffix in ('3dsx','smdh'):
                shutil.copy2(root/f'dist/3ds-chess.{suffix}',root/f'private/3ds-chess-cosunosuke.{suffix}')
    finally:
        run([python,'tools/prepare_assets.py'],'original-assets.log')
        run(['make','-j3','all'],'original-build.log')
        run(['make','test-target'],'original-target.log')
    if args.personal:
        files=[root/f'private/3ds-chess-cosunosuke.{suffix}' for suffix in ('3dsx','smdh')]
        (root/'private/SHA256SUMS').write_text(''.join(hashlib.sha256(p.read_bytes()).hexdigest()+'  '+p.name+'\n' for p in files))
    run([python,'tools/package.py','--local-testing'],'package.log')
    print('Original and requested personal variant built; original atlas restored; local packages generated.')
