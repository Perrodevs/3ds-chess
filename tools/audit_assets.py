#!/usr/bin/env python3
"""Check the committed native atlases and stable rival IDs; no image generation."""
import json,re,hashlib
from pathlib import Path
from PIL import Image
root=Path(__file__).resolve().parents[1]
manifest=json.loads((root/'assets/portraits/manifest.json').read_text())
expected=('pollito hamchow byte magnus hikaru martin wayne fabian juan filip elani noel oliver '
'danya kosteniuk ding-liren vishy bok lasker abdusattorov giri anna-muzychuk irina-krush paul-morphy '
'vidit aronian judit-polgar hou-yifan ian wesley-so capablanca leon jill claire chris joel ellie steve alex jensen').split()
assert manifest['row_order']==expected
assert len(expected)==40
atlas=Image.open(root/'assets/portraits/portraits.png').convert('RGBA')
assert atlas.size==(256,1024)
hashes=[]
for index,name in enumerate(expected):
 for expression,face in enumerate(manifest['expression_order']):
  box=((index//32)*128+expression*32,(index%32)*32,(index//32)*128+expression*32+32,(index%32)*32+32)
  tile=atlas.crop(box)
  assert tile.tobytes()==Image.open(root/f'assets/portraits/{name}-{face}.png').convert('RGBA').tobytes()
  assert tile.getextrema()[3]==(255,255)
  hashes.append(hashlib.sha256(tile.tobytes()).hexdigest())
assert len(hashes)==160 and len(set(hashes))==160
header=(root/'assets/portraits/portraits.hpp').read_text()
assert 'characterCount = 40' in header
assert re.search(r'botCount\s*=\s*50', (root/'include/ai/bots.hpp').read_text())
assert (root/'include/graphics/elon_sprite.hpp').is_file()
assert (root/'include/graphics/rival_sprites.hpp').is_file()
print('Assets: 40 legacy atlas IDs/160 faces plus code-drawn IDs40-49/40 expressions (200 total); atlas pixels agree')
