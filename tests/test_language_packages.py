"""Regression: target validation must not rebuild EN with the ES default."""
from pathlib import Path
import importlib.util
import os
import subprocess
import tempfile
import unittest
from unittest.mock import patch
spec=importlib.util.spec_from_file_location('langpack',Path(__file__).resolve().parents[1]/'tools/build_language_packages.py')
module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
class LanguagePackages(unittest.TestCase):
 def test_same_language_for_build_and_target(self):
  with tempfile.TemporaryDirectory() as directory:
   root=Path(directory);(root/'dist').mkdir();calls=[]
   def fake_run(args,name):
    calls.append(args)
    lang=next((x.split('=')[1] for x in args if x.startswith('DEFAULT_LANGUAGE=')),'ES')
    (root/'dist/3ds-chess.3dsx').write_bytes(b'3DSX'+lang.encode())
    (root/'dist/3ds-chess.smdh').write_bytes(b'SMDH'+f'/ {lang} /'.encode('utf-16le'))
   with patch.object(module,'ROOT',root),patch.object(module,'run',fake_run):
    for lang in ('ES','EN'):module.build_language(lang)
   for lang in ('ES','EN'):
    self.assertIn(['make',f'DEFAULT_LANGUAGE={lang}','test-target'],calls)
    self.assertEqual((root/f'dist/languages/{lang}/3ds-chess.3dsx').read_bytes(),b'3DSX'+lang.encode())
 def test_reject_mismatched_metadata(self):
  with tempfile.TemporaryDirectory() as directory:
   root=Path(directory);(root/'dist').mkdir()
   (root/'dist/3ds-chess.3dsx').write_bytes(b'3DSX')
   (root/'dist/3ds-chess.smdh').write_bytes(b'SMDH'+('/ ES /'.encode('utf-16le')))
   with patch.object(module,'ROOT',root),patch.object(module,'run',lambda *args:None):
    with self.assertRaisesRegex(ValueError,'metadata mismatch'):module.build_language('EN')
 def test_restored_spanish_matches_both_files(self):
  with tempfile.TemporaryDirectory() as directory:
   root=Path(directory);(root/'dist/languages/ES').mkdir(parents=True)
   for ext in ('3dsx','smdh'):
    (root/f'dist/3ds-chess.{ext}').write_bytes(ext.encode())
    (root/f'dist/languages/ES/3ds-chess.{ext}').write_bytes(ext.encode())
   with patch.object(module,'ROOT',root):
    module.verify_restored_spanish()
    (root/'dist/3ds-chess.3dsx').write_bytes(b'wrong default language')
    with self.assertRaisesRegex(ValueError,'mixed-language'):module.verify_restored_spanish()
 def test_equal_second_timestamps_still_rebuild_after_language_switch(self):
  makefile=Path(__file__).resolve().parents[1]/'Makefile'
  with tempfile.TemporaryDirectory() as directory:
   root=Path(directory)
   for name in ('source/probe.cpp','include/app_version.hpp','assets/generated/atlas.hpp','assets/portraits/portraits.hpp','build/3ds/probe.o','build/language.stamp'):
    path=root/name;path.parent.mkdir(parents=True,exist_ok=True);path.write_text('EN' if name.endswith('stamp') else '')
    os.utime(path,(1000000000,1000000000))
   # Force cp to preserve an exactly tied second, independent of host Make's
   # timestamp precision. A tiny compiler stub records whether Make rebuilds.
   bindir=root/'bin';bindir.mkdir()
   scripts={
    'cp':"from pathlib import Path\nimport os,sys\np=Path(sys.argv[2]);p.write_bytes(Path(sys.argv[1]).read_bytes());os.utime(p,(1000000000,1000000000))\n",
    'compiler':"from pathlib import Path\nimport os,sys\np=Path(sys.argv[sys.argv.index('-o')+1]);p.write_bytes(b'object');os.utime(p,(1000000000,1000000000));Path('compiled').write_text('yes')\n"}
   for name,body in scripts.items():
    path=bindir/name;path.write_text('#!/usr/bin/env python3\n'+body);path.chmod(0o755)
   lib=root/'libctru/lib/libcitro2d.a';lib.parent.mkdir(parents=True);lib.touch()
   def build(language):
    env=dict(os.environ,PATH=str(bindir)+os.pathsep+os.environ['PATH'])
    result=subprocess.run(['make','-f',str(makefile),'APP=source/probe.cpp',f'DEFAULT_LANGUAGE={language}',f'ARM_CXX={bindir}/compiler',f'DEVKITPRO={root}','build/3ds/probe.o'],cwd=root,env=env,capture_output=True,text=True)
    self.assertEqual(result.returncode,0,result.stdout+result.stderr)
    return result
   build('EN');self.assertFalse((root/'compiled').exists())
   build('ES');self.assertTrue((root/'compiled').exists())
   self.assertEqual((root/'build/language.stamp').stat().st_mtime,(root/'build/3ds/probe.o').stat().st_mtime)
if __name__=='__main__':unittest.main()
