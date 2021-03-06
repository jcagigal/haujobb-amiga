// movetable example, 25.11.18

#include "movetableeffect.h"

#include "wos/wos.h"
#include "tools/mem32.h"

#include "image/gif.h"
#include "image/tga.h"
#include "math/imath.h"
#include "math.h"

#define pi   3.14159265358979323846

// table size
#define tabx 640
#define taby 360

char *xtab, *ytab;  // pointer to tabels with x- / y-values

unsigned char* texture= 0;
unsigned int texturepal[256];
extern unsigned int* g_currentPal;


unsigned int* movetablePalette()
{
   return texturepal;
}

void buildSphere(char *xbuf, char *ybuf)
{
  int   x,y,w1,z1,p;
  float xmid,ymid;
  float tx,ty,d;
  float fak1,fak2;

  #define CAMDIST 100000

  xmid=tabx / 2.0 - 0.5;
  ymid=taby / 2.0 - 0.5;

  fak1= 128.0 / pi;
  fak2= pi / 32.0;

  for (p=0,y=0;y<taby;y++)    // alle pixel durchlaufen
  {
    for (x=0;x<tabx;x++,p++)
    {
      tx = x - xmid;
      ty = y - ymid;
      d  = CAMDIST / (tx*tx + ty*ty + CAMDIST*1.2);

      z1 = tx * d;
      w1 = ty * d;

      xbuf[p]= w1;
      ybuf[p]= z1;
    }

  }
}


void buildTunnel(char *xbuf, char *ybuf)
{
  int   x,y,w1,z1,p;
  float xmid,ymid;
  float tx,ty;
  float fak1,fak2;

  xmid=tabx / 2.0 - 0.5;
  ymid=taby / 2.0 - 0.5;

  fak1= 128.0 / pi;
  fak2= pi / 32.0;

  for (p=0,y=0;y<taby;y++)    // alle pixel durchlaufen
  {
    for (x=0;x<tabx;x++,p++)
    {
      tx = x - xmid;
      ty = y - ymid;

      w1= 128.0 + atan2(ty,tx)*fak1;  // winkel von akt. pixel zum mittelpunkt
      z1= sqrt (tx*tx + ty*ty);       // entfernung vom mittelpunkt
      z1 = 8192/(z1+1);               // perspektivische umwandlung

      xbuf[p]= w1;
      ybuf[p]= z1;
    }

  }
}


void buildWater(char *xbuf, char *ybuf)
{
  int   x,y,w1,z1,p;
  float xmid,ymid;
  float tx,ty;
  float fak1,fak2;

  #define CAMDIST 100000

  xmid=tabx / 2.0 - 0.5;
  ymid=taby / 2.0 - 0.5;

  fak1= 128.0 / pi;
  fak2= pi / 32.0;

  for (p=0,y=0;y<taby;y++)    // alle pixel durchlaufen
  {
    for (x=0;x<tabx;x++,p++)
    {
      tx = x - xmid;
      ty = y - ymid;

      w1= 128.0 + atan2(ty,tx)*fak1;  // winkel von akt. pixel zum mittelpunkt
      z1= sqrt (tx*tx + ty*ty);       // entfernung vom mittelpunkt

      z1+= sin(z1/32.0*pi)*32.0;

      xbuf[p]= w1;
      ybuf[p]= z1;
    }

  }
}

// draw to "dst" with table "tab", using texture "tex"
void tableDraw(char *dst, char *tab1, char *tab2, char *tex, int move, int xp, int yp)
{
  int  p,t,x,y,p2;

  if (xp<0) xp=0; if (xp>tabx/2) xp=tabx/2;
  if (yp<0) yp=0; if (yp>taby/2) yp=taby/2;

  for (y=0;y<yres;y++)
  {
    p= (y+yp)*tabx+xp;
    //p2= (tabx*taby)-p;
    for (x=0;x<xres;x++,dst++,p++)
    {
       t= ((tab2[p]<<8)+tab1[p]+move) & 0xffff;
       //t= ( ((tab2[p]<<8)+tab1[p]+move) + ((tab2[p2]<<8)+tab1[p2]+move) )   & 0xffff;
      *dst= tex[t];
    }
  }
}

void movetableRelease()
{
   free(xtab);
   free(ytab);
   free(texture);
}

void movetableInit()
{
   int tabsize;
   int w,h;

   gifLoad("data/texture.gif", (void**)&texture, &w,&h, (unsigned int*)&texturepal);
   //tgaLoad8("data/texture.tga", (void**)&texture, &w,&h, (unsigned int*)&texturepal); // provide 256x256x8 tga-file and test this
   g_currentPal = texturepal;

  
   // allocate memory
   tabsize= tabx*taby;  // table size
   xtab=   (char*)malloc(tabsize);
   ytab=   (char*)malloc(tabsize);

   // build tables   
   //buildSphere(xtab, ytab);
   //buildWater(xtab, ytab);
   buildTunnel(xtab, ytab);
}

// draw all stars to screenbuffer, movement based on time
void movetableDraw(int time, unsigned char* screenBuffer)
{
   int  xp,yp,move;
      
   move=time*0x01ff;
  
   xp= ( ((float)tabx/4.0) + fsin(time/48.0)*(tabx>>2) );
   yp= ( ((float)taby/4.0) + fcos(time/64.0)*(taby>>2) );
   
   tableDraw(screenBuffer, xtab, ytab, texture, move, xp, yp);

}

// render method called from main.c
void movetableRender(int time)
{     
   // clear screen
   memset32(screenBuffer, 0, xres*yres);
   
   movetableDraw(time, screenBuffer);

}

