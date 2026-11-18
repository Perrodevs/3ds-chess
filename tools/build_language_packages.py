#!/usr/bin/env python3
"""Build ES/EN alternatives from one source tree, for private local QA only."""
from pathlib import Path
import hashlib,json,re,shutil,subprocess,sys,zipfile
ROOT=Path(__file__).resolve().parents[1]
LOG=ROOT/'build/space-language/packages'
def run(args,name):
    LOG.mkdir(parents=True,exist_ok=True)
    with (LOG/name).open('w') as log:
        result=subprocess.run(args,cwd=ROOT,stdout=log,stderr=subprocess.STDOUT)
    if result.returncode:raise RuntimeError(f'{name} failed; see {LOG/name}')
def build_language(lang):
    run(['make','-j3',f'DEFAULT_LANGUAGE={lang}','all'],f'{lang}-build.log')
    # Target verification is itself a Make invocation: preserve the same default.
    run(['make',f'DEFAULT_LANGUAGE={lang}','test-target'],f'{lang}-target.log')
    dest=ROOT/'dist/languages'/lang;dest.mkdir(parents=True,exist_ok=True)
    for ext in ('3dsx','smdh'):shutil.copy2(ROOT/f'dist/3ds-chess.{ext}',dest/f'3ds-chess.{ext}')
    meta=(dest/'3ds-chess.smdh').read_bytes()
    if f'/ {lang} /'.encode('utf-16le') not in meta:raise ValueError('Language metadata mismatch')
def main():
    if sys.argv[1:]!=['--local-testing']:raise ValueError('Use --local-testing; resource rights are not cleared for publication')
    python=str(ROOT/'.venv/bin/python')
    run([python,'tools/prepare_assets.py'],'original-assets.log')
    try:
        for lang in ('ES','EN'):build_language(lang)
    finally:
        run(['make','-j3','DEFAULT_LANGUAGE=ES','all'],'restore-ES.log')
        run(['make','test-target'],'restore-target.log')
    run([python,'tools/package.py','--local-testing'],'source-and-base-package.log')
    # Same install paths intentionally: alternatives, not two independent save trees.
    base=ROOT/'dist/3ds-chess-sd.zip'
    for lang in ('ES','EN'):
        output=ROOT/f'dist/3DS-Chess-{lang}.zip'
        with zipfile.ZipFile(base) as original,zipfile.ZipFile(output,'w',zipfile.ZIP_DEFLATED) as out:
            for member in original.infolist():
                if member.filename.endswith('/3ds-chess.3dsx'):data=(ROOT/f'dist/languages/{lang}/3ds-chess.3dsx').read_bytes()
                elif member.filename.endswith('/3ds-chess.smdh'):data=(ROOT/f'dist/languages/{lang}/3ds-chess.smdh').read_bytes()
                else:data=original.read(member)
                out.writestr(member.filename,data)
        with zipfile.ZipFile(output) as z:
            if z.testzip() is not None:raise ValueError('Corrupt language ZIP')
            if any(n.endswith(('.dat','.bak','.tmp')) for n in z.namelist()):raise ValueError('Personal save in package')
    files=[ROOT/f'dist/3DS-Chess-{lang}.zip' for lang in ('ES','EN')]
    files += [ROOT/f'dist/languages/{lang}/3ds-chess.{ext}' for lang in ('ES','EN') for ext in ('3dsx','smdh')]
    files += [ROOT/'dist/3ds-chess-source.zip']
    hashes={str(p.relative_to(ROOT/'dist')):hashlib.sha256(p.read_bytes()).hexdigest() for p in files}
    version=re.search(r'version = "([^"]+)"',(ROOT/'include/app_version.hpp').read_text())[1]
    (ROOT/'dist/SPACE_LANGUAGE_RELEASE.json').write_text(json.dumps({'version':version,'local_testing_only':True,'physical_tested':False,'install_policy':'Alternatives at same SD path; share compatible per-profile data; explicit preferences override defaults','sha256':hashes},indent=2)+'\n')
    (ROOT/'dist/SHA256SUMS-LANGUAGES').write_text(''.join(f'{digest}  {name}\n' for name,digest in hashes.items()))
    print('Built ES/EN alternatives and source ZIP. Main dist binary restored to ES. No saves packaged.')
if __name__=='__main__':main()
