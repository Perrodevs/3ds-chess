"""Regression: target validation must not rebuild EN with the ES default."""
from pathlib import Path
import importlib.util
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
if __name__=='__main__':unittest.main()
