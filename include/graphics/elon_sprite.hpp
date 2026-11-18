#pragma once
#include "graphics/renderer.hpp"
namespace graphics {
// Original code-drawn32x32 caricature: no photograph or external image resource.
inline void drawElonPortrait(Renderer&r,int expression,int x,int y,int scale){
 auto box=[&](int a,int b,int w,int h,Color c){r.rect(x+a*scale,y+b*scale,w*scale,h*scale,c);};
 const Color skin=rgb(224,171,143),shadow=rgb(173,117,102),hair=rgb(68,49,45),dark=rgb(24,28,37);
 box(0,0,32,32,rgb(24,32,46));box(5,25,22,7,dark);box(12,22,8,6,shadow);
 box(8,7,16,17,skin);box(6,12,2,7,shadow);box(24,12,2,7,shadow);
 box(7,4,18,7,hair);box(9,2,13,3,hair);box(6,8,3,5,hair);box(23,8,3,5,hair);
 box(10,12,4,1,hair);box(19,12,4,1,hair);
 box(11,14,2,expression==1?1:2,dark);box(20,14,2,expression==1?1:2,dark);
 box(16,16,2,3,shadow);box(12,21,8,1,dark);
 if(expression==2){box(12,20,1,2,dark);box(19,20,1,2,dark);box(14,21,4,1,rgb(245,230,212));}
 if(expression==3){box(11,11,3,1,dark);box(19,11,3,1,dark);box(13,20,6,1,dark);}
 if(expression==1){box(23,23,5,5,skin);box(21,23,5,2,skin);}
 box(13,27,6,1,rgb(200,205,212));box(15,26,2,5,rgb(200,205,212));
}
}
