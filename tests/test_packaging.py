"""Regression: a private QA build must not silently become a public/source leak."""
from pathlib import Path
import importlib.util
import tempfile
import unittest
spec=importlib.util.spec_from_file_location('pack',Path(__file__).resolve().parents[1]/'tools/package.py')
pack=importlib.util.module_from_spec(spec);spec.loader.exec_module(pack)
class Packaging(unittest.TestCase):
 def test_source_filter(self):
  with tempfile.TemporaryDirectory() as d:
   root=Path(d)
   wanted={'README.md','include/app_version.hpp','tests/native_stubs/3ds.h','assets/source/README.md','docs/ASSET_RIGHTS.md'}
   forbidden={'private/personal.3dsx','build/profile.dat','dist/game.zip','.venv/token','assets/source/8 Bit Chess.zip','arbitrary.secret','source/example.dSYM/private'}
   for name in wanted|forbidden:
    p=root/name;p.parent.mkdir(parents=True,exist_ok=True);p.write_text('test')
   self.assertEqual(set(map(str,pack.source_members(root))),wanted)
 def test_explicit_scope_and_stale_build(self):
  with tempfile.TemporaryDirectory() as d:
   root=Path(d)
   with self.assertRaisesRegex(ValueError,'Rights review'):pack.package(root)
   for name,text in [('assets/generated/manifest.json','{"sprites":"Personal"}'),('include/app_version.hpp','version = "0.8.2 RC";')]:
    p=root/name;p.parent.mkdir(parents=True,exist_ok=True);p.write_text(text)
   with self.assertRaisesRegex(ValueError,'Personal pieces'):pack.package(root,True)
   (root/'assets/generated/manifest.json').write_text('{"sprites":"Original"}')
   (root/'dist').mkdir();(root/'dist/3ds-chess.3dsx').write_bytes(b'3DSXold');(root/'dist/3ds-chess.smdh').write_bytes(b'SMDHold')
   with self.assertRaisesRegex(ValueError,'Stale'):pack.package(root,True)
   self.assertFalse((root/'dist/3ds-chess-sd.zip').exists())
if __name__=='__main__':unittest.main()
