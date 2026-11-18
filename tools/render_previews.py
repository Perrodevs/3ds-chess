#!/usr/bin/env python3
"""Rasterize the rectangular pixel output of the shared host renderer."""
from pathlib import Path
from PIL import Image,ImageDraw
import xml.etree.ElementTree as ET
import re
root=Path(__file__).resolve().parents[1]
for p in (root/'build').glob('preview-*.svg'):
 im=Image.new('RGBA',(400,496),(0,0,0,255));draw=ImageDraw.Draw(im,'RGBA')
 def render(element,ox=0,oy=0):
  transform=element.get('transform','')
  if transform:
   tx,ty=map(int,re.findall(r'-?\d+',transform));ox+=tx;oy+=ty
  if element.tag.endswith('rect'):
   x=ox+int(element.get('x'));y=oy+int(element.get('y'));w=int(element.get('width'));h=int(element.get('height'));rgb=element.get('fill');color=tuple(int(rgb[i:i+2],16) for i in (1,3,5));alpha=round(float(element.get('fill-opacity','1'))*255)
   if alpha==255:draw.rectangle((x,y,x+w-1,y+h-1),fill=(*color,255))
   else:
    for yy in range(max(0,y),min(496,y+h)):
     for xx in range(max(0,x),min(400,x+w)):
      old=im.getpixel((xx,yy));im.putpixel((xx,yy),tuple((c*alpha+old[i]*(255-alpha))//255 for i,c in enumerate(color))+(255,))
  for child in element:render(child,ox,oy)
 render(ET.parse(p).getroot());im.resize((800,992),Image.Resampling.NEAREST).save(p.with_suffix('.png'))
 print(p.with_suffix('.png').name)
