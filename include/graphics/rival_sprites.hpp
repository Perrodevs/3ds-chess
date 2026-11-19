#pragma once
#include "graphics/renderer.hpp"
#include <algorithm>
namespace graphics {
// Original 32x32 code art. No downloaded art, atlas growth or owned resources.
inline void drawNewRival(Renderer &r,int id,int expression,int x,int y,int scale) {
 const int e=std::clamp(expression,0,3),n=std::clamp(id-41,0,8);
 auto b=[&](int a,int c,int w,int h,Color color){r.rect(x+a*scale,y+c*scale,w*scale,h*scale,color);};
 const Color bg=rgb(15,21,32),ink=rgb(22,23,30),purple=rgb(181,101,238),skin=rgb(224,176,147);
 b(0,0,32,32,bg);
 if(id==49){
  b(7,5,18,5,ink);b(4,10,24,19,rgb(22,21,40));b(9,2,14,6,rgb(30,27,49));
  b(8,11,16,12,rgb(6,8,17));b(2,29,28,3,ink);
  Color eyes=e==2?rgb(227,163,255):e==1?rgb(132,94,245):purple;
  b(10,14,4,e==1?1:2,eyes);b(19,14,4,e==1?1:2,eyes);
  if(e==2)b(13,21,7,1,purple);
  if(e==3){b(8,13,7,1,purple);b(20,16,5,1,purple);b(3,24,11,1,purple);b(23,9,7,1,purple);}
  return;
 }
 if(id==46){
  b(6,4,20,21,rgb(20,17,30));b(9,25,4,7,ink);b(20,25,4,7,ink);
  b(8,13,6,e==1?1:2,purple);b(19,13,6,e==1?1:2,purple);
  if(e==2){b(11,20,10,2,purple);b(3,5,2,2,purple);}
  if(e==3){b(11,21,10,4,ink);b(5,17,3,1,purple);b(25,8,3,1,purple);}
  return;
 }
 if(id==47){
  b(15,0,3,8,rgb(97,103,111));b(6,7,21,15,rgb(197,204,200));b(3,10,5,10,rgb(119,130,137));
  b(10,10,14,8,ink);b(14,12,e==1?3:7,e==2?5:3,rgb(246,204,e==3?119:66));
  b(12,23,12,4,rgb(119,130,137));b(20,27,8,3,rgb(197,204,200));
  if(e==2)b(9,20,12,1,rgb(246,204,66));if(e==3)b(5,6,9,2,rgb(193,96,60));
  return;
 }
 const Color coats[]={rgb(56,70,101),rgb(53,60,70),rgb(61,87,145),rgb(166,37,60),rgb(30,32,40),bg,bg,rgb(62,104,136)};
 const Color hair[]={rgb(50,39,36),rgb(34,32,35),rgb(133,99,62),rgb(25,25,32),rgb(205,177,105),ink,ink,rgb(117,88,61)};
 b(4,26,24,6,coats[n]);b(12,21,8,6,skin);b(8,7,16,17,skin);b(6,12,2,7,skin);b(24,12,2,7,skin);
 b(7,4,18,6,hair[n]);b(8,2,15,3,hair[n]);b(6,8,3,6,hair[n]);
 if(id==41){for(int i=0;i<4;++i)b(7+i*5,3+i%2,3,3,ink);b(14,27,4,5,rgb(155,64,58));}
 if(id==42){b(7,5,6,9,hair[n]);b(21,25,7,4,ink);}
 if(id==43){b(19,5,6,8,hair[n]);b(12,27,8,2,rgb(207,215,221));}
 if(id==44){b(21,6,6,20,hair[n]);b(6,7,5,17,hair[n]);b(26,18,2,3,rgb(223,186,96));}
 if(id==45){b(9,11,15,5,ink);b(10,12,4,1,rgb(107,128,138));b(18,12,4,1,rgb(107,128,138));}
 else {b(11,13,2,e==1?1:2,ink);b(20,13,2,e==1?1:2,ink);}
 if(id==48){b(9,11,6,6,ink);b(18,11,6,6,ink);b(15,13,3,1,ink);b(10,12,4,4,skin);b(19,12,4,4,skin);}
 b(16,16,2,3,rgb(177,121,105));b(12,21,8,1,ink);
 if(e==1){b(22,24,5,5,skin);b(20,23,5,2,skin);}
 if(e==2){b(12,20,1,2,ink);b(19,20,1,2,ink);b(14,21,4,1,rgb(246,232,216));}
 if(e==3){b(10,10,5,1,ink);b(19,10,4,1,ink);b(13,20,6,1,ink);}
}
}
