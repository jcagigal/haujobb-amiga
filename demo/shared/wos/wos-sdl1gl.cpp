/*
    Wicked OS SDL frontend by Crumb, 95% based on original wos.cpp/wos.h by haujobb, thx guys!

    INSTRUCTIONS:
    Replace wos.cpp/wos.h by wos-sdl1gl.cpp/wos-sdl1gl.h in your makefiles/favourite IDE.

    You should add the required link libraries depending on your OS, in the case of windows "This works here(tm)":
    -mwindows -lws2_32 -lbass
    In case you use OpenGL you should also add:
    -lopengl32 -lglu32

    DEPENDENCIES:
    +SDL 1.2 dev files & dynamic libraries
    -You no longer need QT libraries so remove them from your makefiles/build configuration

    TO DO:
    -file reloading using "simplefilewatcher" or pressing "r" to force it manually
*/

#ifndef AMIGA
#include <SDL/SDL.h>

#ifndef NO_OPENGL_THANKS
#include <GL/gl.h>
#include <GL/glu.h>
#endif
#endif


#include <string.h>

#include "wos-sdl1gl.h"

// #define EXPORT_BMP
// #define BLEND_FRAMES
// #define FAST_FORWARD 1

#define GL_BGRA                           0x80E1

#pragma comment(lib, "winmm")

int refresh= 50;
int g_vbitimer= 0;
int g_renderedFrames= 0;
int musicStart= 0;
int sample1= 0;
int audioRate= 0;
short* audioLeft= 0;
short* audioRight= 0;
int audioLength= 0;

static WosSdl1Gl* instance= 0;
static void* wosBuffer= 0;
static unsigned int wosPal[256];

#ifdef __cplusplus
extern "C" {
#endif

extern void drawDemo(int time);
extern void updateDemo(int time);
extern void mainDemo();

int totalChipMem= 0;
int oddeven= 0;
static int mouseX= 0;
static int mouseY= 0;
static int mouseZ= 0;

int wosMouseX()
{
   return mouseX;
}

int wosMouseY()
{
   return mouseY;
}

int wosMouseZ()
{
   return mouseZ;
}

unsigned char* wosAllocChipMem(int bytes)
{
   totalChipMem+=bytes;
   unsigned char* buffer= (unsigned char*)malloc(bytes);
   return buffer;
}

void wosSetMusicStart(int miliseconds)
{
   musicStart= miliseconds;
}

void wosFreeChipMem(unsigned char* buffer, int /*size*/)
{
   // size only for Amiga FreeMem
   if (buffer) free(buffer);
}

void wosSetupMusic(int sampleRate, short* left, short* right, int length)
{
   audioRate= sampleRate;
   audioLeft= left;
   audioRight= right;
   audioLength= length;
}

void wosCreate()
{
    int argc= 0;
    char **argv= 0;

/*
     Uint32 format= SDL_HWSURFACE|SDL_DOUBLEBUF|SDL_RESIZABLE;

    #ifndef DONTUSEOPENGL
        format |= SDL_OPENGL;
    #endif
*/
//    format.setSampleBuffers(false);
//    format.setSwapInterval(1);

    instance= new WosSdl1Gl();
}

void wosInit()
{
   if (!instance)
      wosCreate();
//   instance->move( QPoint(-1920,0) );

   //TODO: free memory and resources
}

void wosSetMode(int mode, void* buffer, void* palette, int brightness)
{
   int i;
   wosBuffer= (void*)buffer;
   unsigned int* pal= (unsigned int*)palette;

   if (pal)
   {
      unsigned int* temp= (unsigned int*)pal;
      for (i=0; i<256; i++)
      {
         int b= pal[i]&255;
         int g= pal[i]>>8&255;
         int r= pal[i]>>16&255;

         b= b*brightness>>8;
         g= g*brightness>>8;
         r= r*brightness>>8;

         if (r>255) r=255;
         if (g>255) g=255;
         if (b>255) b=255;

         wosPal[i]= (r<<16)|(g<<8)|b;
      }
   }

   instance->setMode(mode);
}

void wosInitFileWatcher(void* filenames, void(*callback)())
{
   if (!instance)
      wosCreate();
   instance->initFileWatcher((char**)filenames, callback);
}

void wosSetCols(void* palette, int brightness)
{
   int i;
   unsigned int* pal= (unsigned int*)palette;
   if (pal==0) return;
   for (i=0; i<256; i++)
   {
      int b= pal[i]&255;
      int g= pal[i]>>8&255;
      int r= pal[i]>>16&255;

      b= b*brightness>>8;
      g= g*brightness>>8;
      r= r*brightness>>8;

      if (r>255) r=255;
      if (g>255) g=255;
      if (b>255) b=255;

      wosPal[i]= (r<<16)|(g<<8)|b;
   }
}

void wosDisplay(int mode)
{
   instance->updateBuffer((unsigned char*)wosBuffer);
}

void wosSetupBitplane(
  int bitPlaneId,         // um welche Bitplane geht es
  unsigned char* buffer,  // da stehen die bits (8 "Pixel" -> 1 Byte)
  int pitch,              // Breite der Bitplane (in Byte?) -> definiert wo die naechste Zeile anfaengt
  int starty,             // erste Zeile ab der die Bitplane sichtbar sein soll
  int height )            // anzahl der Zeilen die die Bitplane sichtbar sein soll
{
   instance->setBitplane(bitPlaneId, buffer, pitch, starty, height);
}

void wosSetupBitplaneLines(
   int bitPlaneId,        // welche Bitplane
   unsigned char** linePointer,   // pro scanline: bitplane pointer & shift register
   int starty,            // erste scanline
   int height )           // anzahl der scanlines (eintraege in "lines")
{
   instance->setBitplaneLines(bitPlaneId, linePointer, starty, height);
}

void wosSetupBitplaneLine(
   int bitPlaneId,        // welche Bitplane
   int scanline,          // welche scanline
   unsigned char* buff )  // shift register
{
   instance->setBitplaneLine(bitPlaneId, scanline, buff);
}

void wosSetupCopper(int index, void* gradient)
{
   instance->setCopper(index, (unsigned int*)gradient);
}

void wosSetPlayfieldShift(int id, unsigned char shift)
{
   instance->setOddEvenShift(id, shift);
}

int wosCheckExit()
{
   return 0;
}

void wosClearPlanes()
{
   // ?
}

void wosClearExit()
{
   //instance->deleteLater();
   // ?
}

#ifdef __cplusplus
};
#endif


#ifdef WIN32
#include <windows.h>
#include <mmsystem.h>

WAVEHDR  mWave;
HWAVEOUT mHandle= 0;
HANDLE   mThread;
short*   mAudioBuffer= 0;
WAVEFORMATEX	pcmwf;

void appquit(void) {
    //TODO: cleanup!
}

int mTimer_elapsed(Uint32 startTicks) {
 return SDL_GetTicks() - (startTicks);
}

void mTimer_setInterval(Uint32 *timer, int interval) {
    *timer +=interval;
}


void restartTiming()
{
   UINT			   hr;

   waveOutUnprepareHeader(mHandle, &mWave, sizeof(WAVEHDR));

   waveOutReset(mHandle);
   waveOutClose(mHandle);

   hr = waveOutOpen(&mHandle, WAVE_MAPPER, &pcmwf, 0, 0, 0);
   if (hr)
      return;


   hr = waveOutPrepareHeader(mHandle, &mWave, sizeof(WAVEHDR));
   if (hr)
      return;

   waveOutWrite(mHandle, &mWave, sizeof(WAVEHDR));

   waveOutPrepareHeader(mHandle, &mWave, sizeof(WAVEHDR));
   waveOutWrite(mHandle, &mWave, sizeof(WAVEHDR));
}

void initPlayer()
{
   if (audioLength>0)
   {
//      int channels= 1; if (audioLeft && audioRight) channels= 2;

      mAudioBuffer= new short[audioLength*2];

      sample1= (int) (musicStart * (audioRate / 1000.0));
      if (sample1 < audioLength)
      {
         for (int i=0; i<sample1; i++)
         {
            short l= 0;
            short r= 0;
            if (audioLeft) l= audioLeft[i];
            if (audioRight) r= audioRight[i]; else r=l;
            mAudioBuffer[(audioLength-sample1+i)*2+0]= l;
            mAudioBuffer[(audioLength-sample1+i)*2+1]= r;
         }

         for (int i=0; i<audioLength-sample1; i++)
         {
            short l= 0;
            short r= 0;
            if (audioLeft) l= audioLeft[sample1+i];
            if (audioRight) r= audioRight[sample1+i]; else r=l;
            mAudioBuffer[i*2+0]= l;
            mAudioBuffer[i*2+1]= r;
         }
      }
      else
      {
         for (int i=0; i<audioLength*2; i++)
            mAudioBuffer[i]= 0;
      }
   }
   else
   {
      audioRate= 22050;
      audioLength= 10000;
      mAudioBuffer= new short[audioLength*2];
      memset(mAudioBuffer, 0, audioLength*4);
   }

   mWave.lpData			= (LPSTR)mAudioBuffer;
   mWave.dwBufferLength	= audioLength<<2; // stereo + 16bit per sample
   mWave.dwBytesRecorded= 0;
   mWave.dwUser			= 0;
   mWave.dwFlags			= WHDR_BEGINLOOP | WHDR_ENDLOOP;
   mWave.dwLoops			= -1;

   int bits= 16;
   pcmwf.wFormatTag		= WAVE_FORMAT_PCM;
   pcmwf.nChannels		= 2;
   pcmwf.nSamplesPerSec	= audioRate;
   pcmwf.nAvgBytesPerSec= pcmwf.nSamplesPerSec * pcmwf.nChannels * bits >> 3;
   pcmwf.nBlockAlign		= pcmwf.nChannels * bits >> 3;
   pcmwf.wBitsPerSample	= bits;
   pcmwf.cbSize			= 0;

   restartTiming();
}
#else
void restartTiming() //for windows audio
{

}

void initPlayer() //for windows audio
{
}
#endif

WosSdl1Gl::WosSdl1Gl(/*Uint32& format*/)
{
    mLastFpsTime = 0;
    mLastFpsFrame = 0;
    mMode = 0;
    mLastRenderedFrame = -1;
    mMaxNumColors = 0;
    mC2PFirstBit = 0;
    mC2PLastBit = 0;
    mC2PTarget = 0;
    mHamChannels = 0;
//    mWatcher = 0;
//    mReloadDelay = 0;
    mScaleY = 1;
    mCaptureBuffer = 0;
    mBlendBuffer = 0;
    mSaturate = 256;

    instance= this;

   mOddEvenShift[0]= 0;
   mOddEvenShift[1]= 0;

   //TODO instalar gestor de eventos...
   /*
   mWindow= new QMainWindow();
   mWindow->installEventFilter(this);
   QWidget* mainWidget= new QWidget();
   mWindow->setCentralWidget(mainWidget);
   QVBoxLayout* layout= new QVBoxLayout(mainWidget);
   layout->setMargin(1);
   layout->addWidget(this);
   QHBoxLayout* tools= new QHBoxLayout();
   layout->addLayout(tools);
   setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

   // redraw at ~50hz
   mUpdateTimer= new QTimer(this);
   mUpdateTimer->setInterval(1);
   mUpdateTimer->setSingleShot(true);
   connect(mUpdateTimer, SIGNAL(timeout()), this, SLOT(update()));
 */

   g_renderedFrames= 0;
   g_vbitimer= 0;

   for (int id=0; id<8; id++)
   {
      Bitplane* bpl= &mBitplane[id];
      for (int i=0; i<512; i++)
      {
         bpl->line[i]= 0;
      }
   }

   for (int i=0; i<8; i++)
   {
      mC2PPlanes[i]= wosAllocChipMem(DEFAULT_WINDOW_WIDTH*DEFAULT_WINDOW_HEIGHT>>3);
      memset(mC2PPlanes[i], 0, DEFAULT_WINDOW_WIDTH*DEFAULT_WINDOW_HEIGHT>>3);
   }
   for (int i=0; i<512; i++) mCopperColorEnabled[i]= 0;

   mCopperColorData= new unsigned int[65536];
   for (int i=0; i<65536;i++) mCopperColorData[i]= 0;

   // the file system watcher trigger when a file was opened for writing.
   // delay reload by 100ms because the write process may not be complete yet

/*   mReloadDelay= new QTimer(this);
   mReloadDelay->setSingleShot(true);
   mReloadDelay->setInterval(100);
   connect(mReloadDelay, SIGNAL(timeout()), this, SLOT(triggerReload()));
*/

   initPlayer(); //inits music player

    // initialize SDL video
    if ( SDL_Init( SDL_INIT_VIDEO ) < 0 )
    {
        printf( "Unable to init SDL: %s\n", SDL_GetError() );
//todo: exit        return 1;
    }

   Uint32 format= SDL_HWSURFACE|SDL_DOUBLEBUF;

  #ifndef NO_OPENGL_THANKS
      format |= SDL_OPENGL|SDL_RESIZABLE;
      screen = SDL_SetVideoMode(DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT, 16, format);
  #else
      screen = SDL_SetVideoMode(DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT, 32, format);
  #endif

   if ( !screen )
   {
       printf("Unable to open window: %s\n", SDL_GetError());
//todo: exit        return 1;
   }


    // make sure SDL cleans up before exit
    atexit(SDL_Quit);

   initializeGL();
   mainLoop();
// ////////////////////   mWindow->resize(320*2+2, 180*2+2);
// //////////////   mWindow->show();

}


WosSdl1Gl::~WosSdl1Gl()
{
    if (mBuffer) free(mBuffer);

    if (screen)
    {
        SDL_FreeSurface(screen);
    }
#ifdef WIN32
   waveOutUnprepareHeader(mHandle, &mWave, sizeof(WAVEHDR));
   mWave.dwFlags &= ~WHDR_PREPARED;

   waveOutReset(mHandle);
   waveOutClose(mHandle);
#endif

   delete[] mAudioBuffer;
}

void SetupHam15Bit(unsigned int* bp0, unsigned int* bp1, int rows)
{
   while (rows--)
   {
      for (int x=0; x<40; x++)
      {
//                                bgrbgrbg rbgrbgrb grbgrbgr bgrbgrbg
//         *bp0++= 0xdb6db6db; // 11011011.01101101.10110110.11011011
//         *bp1++= 0x6db6db6d; // 01101101.10110110.11011011.01101101

//                              grbgrbgr.bgrbgrbg.rbgrbgrb.grbgrbgr  <- bottom bit first! (rgb, rgb, ...)
         *bp0++= 0xb6db6db6; // 10110110.11011011.01101101.10110110
         *bp1++= 0xdb6db6db; // 11011011.01101101.10110110.11011011
      }

//    rgbrgbrg.brgbrgbr.gbrgbrgb.rgbrgbrg
//    01101101.10110110.11011011.01101101
//    11011011.01101101.10110110.11011011
   }
}

void SetupHam18(unsigned int* bp0, unsigned int* bp1, int rows)
{
   while (rows--)
   {
      for (int x=0; x<40; x++)
      {
//         *bp0++= 0xdddddddd; // bgrb 1101
//         *bp1++= 0x66666666; // bgrb 0110
         *bp0++= 0xbbbbbbbb; // bgrb 1101 = 0xb (bottom bit first!)
         *bp1++= 0x66666666; // bgrb 0110 = 0x6
      }
   }
}


void WosSdl1Gl::setMode(int mode)
{
   int i,y;
   mMode= mode;
   switch (mode)
   {
      // copies bits 12345678
      // 256 colors 8bit
      case -1:
         for (i=0;i<8;i++) setBitplane(i,0,0,0,180);
         mC2PFirstBit= -1;
         mC2PLastBit= -1;
         mScaleX= 1;
         mScaleY= 1;
      break;

      // copies bits 12345678
      // 320x180 256 colors 8bit
      case 8:
         for (i=0;i<8;i++) setBitplane(i,mC2PPlanes[i],320/8,0,180);
         mC2PFirstBit= 0;
         mC2PLastBit= 8;
         mScaleX= 1;
         mScaleY= 1;

         mC2PTarget= 0;
         mHamChannels = 0;
         mSaturate=256;
      break;

      // copies bits 12345678
      // 320x90 256 colors 8bit
      case 9:
         for (i=0;i<8;i++) setBitplane(i,mC2PPlanes[i],320/8,0,180);
         mC2PFirstBit= 0;
         mC2PLastBit= 8;
         mScaleX= 1;
         mScaleY= 2;

         mC2PTarget= 0;
         mHamChannels = 0;
         mSaturate=256;
      break;

      // copies bits 12345678
      // 160x90 256 colors 8bit
      case 10:
         for (i=0;i<8;i++) setBitplane(i,mC2PPlanes[i],320/8,0,180);
         mC2PFirstBit= 0;
         mC2PLastBit= 8;
         mScaleX= 2;
         mScaleY= 2;

         mC2PTarget= 0;
         mHamChannels = 0;
         mSaturate=256;
      break;

      // copies bits ss12345x
      // 32 colors 5bit
      case 24:
         for (i=0;i<5;i++) setBitplane(i,mC2PPlanes[i],320/8,0,180);
         setBitplane(5,0,0,0,0);
         setBitplane(6,0,0,0,0);
         setBitplane(7,0,0,0,0);
         mC2PTarget= 0;
         mC2PFirstBit= 0;
         mC2PLastBit=  5;
         mHamChannels = 0;
         mScaleX= 1;
         mScaleY= 1;
         mSaturate= 64;
      break;


      // copies bits oo123456
      // 640x180 ham8 brgb (truecolor 18bit 2x2 -> 160x90)
      case 11:
      {
         mScaleX= 1;
         mScaleY= 2;
         mSaturate= 64;
         for (i=0;i<8;i++) setBitplane(i,mC2PPlanes[i],640/8,0,180);

         SetupHam18((unsigned int*)mC2PPlanes[0], (unsigned int*)mC2PPlanes[1], 180);

         mC2PFirstBit= 0;
         mC2PLastBit= 6;
         mC2PTarget= 2;  // untere 2bits sind ham control
         mHamChannels= 4;
         break;
      }

      // copies bits 12345678
      // 640x180 256 colors 8bit
      case 12:
         for (i=0;i<8;i++) setBitplane(i,mC2PPlanes[i],640/8,0,DEFAULT_WINDOW_HEIGHT);
         mC2PFirstBit= 0;
         mC2PLastBit= 8;
         mC2PTarget= 0;
         mSaturate= 256;
         mHamChannels = 0;
         mScaleX= 0;
         mScaleY= 1;
      break;

      // 640x360
      case 13:
         for (i=0;i<8;i++) setBitplane(i,mC2PPlanes[i],640/8,0,DEFAULT_WINDOW_HEIGHT);
         mC2PFirstBit= 0;
         mC2PLastBit= 8;
         mC2PTarget= 0;
         mSaturate= 256;
         mHamChannels = 0;
         mScaleX= 0;
         mScaleY= 0;
      break;

      // copies bits ss123456
      // 64 colors 6bit
      case 14:
         for (i=0;i<6;i++) setBitplane(i,mC2PPlanes[i],320/8,0,180);
         setBitplane(6,0,0,0,180);
         setBitplane(7,0,0,0,180);
         mC2PFirstBit= 0;
         mC2PLastBit=  6;
         mC2PTarget= 0;
         mSaturate= 64;
         mHamChannels = 0;
         mScaleX= 1;
         mScaleY= 1;
      break;


      // copies bits 12345xxx
      // 32 colors 5bit
      case 15:
         for (i=0;i<5;i++) setBitplane(i,mC2PPlanes[i],320/8,0,180);
         for (i=5;i<8;i++) setBitplane(i,0, 0,0,0);
         mC2PFirstBit= 0;
         mC2PLastBit=  5;
         mC2PTarget= 0;
         mHamChannels = 0;
         mSaturate= 256;
         mScaleX= 1;
         mScaleY= 1;
      break;

      // 640x180 8bit 256 colors
      case 16:
         for (i=0;i<8;i++) setBitplane(i,mC2PPlanes[i],640/8,0,180);
         mC2PFirstBit= 0;
         mC2PLastBit= 8;
         mC2PTarget= 0;
         mHamChannels = 0;
         mSaturate= 256;
         mScaleX= 1;
         mScaleY= 1;
      break;

      // 320x180 5bit, 25..31 copper
      case 25:
         for (i=0;i<5;i++) setBitplane(i,mC2PPlanes[i],DEFAULT_WINDOW_WIDTH/8,0,180);
         mC2PFirstBit= 0;
         mC2PLastBit= 5;
         mC2PTarget= 0;
         mHamChannels = 0;
         mSaturate= 256;
         mScaleX= 1;
         mScaleY= 1;
      break;


      // copies bits 12345xxx
      // 640x180 ham8 rgb (truecolor 18bit 3x1 -> 220x180)
      case 17:
      {
         mScaleX= 1;
         mScaleY= 1;
         mSaturate= 256;

         for (i=0;i<8;i++) setBitplane(i, mC2PPlanes[i], 640/8, 0, 180);

         SetupHam15Bit((unsigned int*)mC2PPlanes[0], (unsigned int*)mC2PPlanes[1], 180);
         mC2PFirstBit= 3; //
         mC2PLastBit= 8;
         mC2PTarget= 3;  // untere 2bits sind ham control, untere 6 farbbits in 2..7
         mHamChannels= 3;
         break;
      }
/*
*/

      // copies bits oo12345x
      // 640x180 ham8 rgb (truecolor 18bit 3x1 -> 220x180)
      case 18:
      {
         mScaleX= 1;
         mScaleY= 1;
         mSaturate= 256;

         for (i=0;i<8;i++) setBitplane(i, mC2PPlanes[i], 640/8, 0, 180);

         SetupHam15Bit((unsigned int*)mC2PPlanes[0], (unsigned int*)mC2PPlanes[1], 180);
         mC2PFirstBit= 1; // unterstes farbbit weglassen
         mC2PLastBit= 6;
         mC2PTarget= 3;  // untere 2bits sind ham control, unterstes farbbit wird nicht verwendet (5 anstatt 6bit)
         mHamChannels= 3;
         break;
      }

      // copies bits oo12345x
      // 640x90 ham8 rgb (truecolor 18bit 3x1 -> 216x90)
      case 19:
      {
         mScaleX= 1;
         mScaleY= 2;
         mSaturate= 64;

         for (i=0;i<8;i++) setBitplane(i, mC2PPlanes[i], 640/8, 0, 180);

         SetupHam15Bit((unsigned int*)mC2PPlanes[0], (unsigned int*)mC2PPlanes[1], 180);
         mC2PFirstBit= 1; // unterstes farbbit weglassen
         mC2PLastBit= 6;
         mC2PTarget= 3;  // untere 2bits (0..1) sind ham control, obere 5 farbbits in 3..7 (bitplane 2 bleibt leer)
         mHamChannels= 3;
         break;
      }

      // copies bits oo345678
      // 640x180 ham8 rgb (truecolor 18bit 3x1 -> 220x180)
      case 20:
      {
         mScaleX= 1;
         mScaleY= 1;
         mSaturate= 256;

         for (i=0;i<8;i++) setBitplane(i, mC2PPlanes[i], 640/8, 0, 180);

         SetupHam15Bit((unsigned int*)mC2PPlanes[0], (unsigned int*)mC2PPlanes[1], 180);
         mC2PFirstBit= 2; // 2 unterste farbbits weglassen
         mC2PLastBit= 8;
         mC2PTarget= 2;  // untere 2bits (0..1) sind ham control, obere 6 farbbits in 2..7
         mHamChannels= 3;
         break;
      }

      // copies bits oo123456
      // 640x180 ham8 rgb (truecolor 18bit 3x1 -> 220x180)
      case 21:
      {
         mScaleX= 1;
         mScaleY= 1;
         mSaturate= 64;

         for (i=0;i<8;i++) setBitplane(i, mC2PPlanes[i], 640/8, 0, 180);

         SetupHam15Bit((unsigned int*)mC2PPlanes[0], (unsigned int*)mC2PPlanes[1], 180);
         mC2PFirstBit= 0; //
         mC2PLastBit= 6;
         mC2PTarget= 2;  // untere 2bits sind ham control, untere 6 farbbits in 2..7
         mHamChannels= 3;
         break;
      }

      // copies bits oo123456
      // 640x90 ham8 rgb (truecolor 18bit 3x1 -> 216x90)
      case 22:
      {
         mScaleX= 1;
         mScaleY= 2;
         mSaturate= 64;

         for (i=0;i<8;i++) setBitplane(i, mC2PPlanes[i], 640/8, 0, 180);

         SetupHam15Bit((unsigned int*)mC2PPlanes[0], (unsigned int*)mC2PPlanes[1], 180);
         mC2PFirstBit= 0; // copy bits [0..6[
         mC2PLastBit= 6;
         mC2PTarget= 2;  // bits 0..1 sind ham control
         mHamChannels= 3;
         break;
      }

      // copies bits oo123456
      // 640x180 ham8 rgb (truecolor 18bit 3x1 -> 216x180)
      case 23:
      {
         mScaleX= 1;
         mScaleY= 1;
         mSaturate= 64;

         for (i=0;i<8;i++) setBitplane(i, mC2PPlanes[i], 640/8, 0, 180);

         SetupHam15Bit((unsigned int*)mC2PPlanes[0], (unsigned int*)mC2PPlanes[1], 180);
         mC2PFirstBit= 2; // unterstes farbbit weglassen
         mC2PLastBit= 6;
         mC2PTarget= 4;  // bits 0..1 sind ham control, unterste 6 farbbits in 2..7
         mHamChannels= 3;
         break;
      }
   };

   if (mode!=16 && mode!=11 && mode!=17 && mode!=18 && mode!=19 && mode!=20 && mode!=21 && mode!=22 && mode!=23)
   {
      for (int i=0; i<512; i++)
         mCopperColorEnabled[i]= 0;
   }
}

void WosSdl1Gl::initializeGL()
{
#ifndef NO_OPENGL_THANKS
   glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
   glColor4f(1,1,1,1);

   mBuffer=  (unsigned int*)malloc(DEFAULT_WINDOW_WIDTH*DEFAULT_WINDOW_HEIGHT*4);

//   glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);;

   glEnable(GL_DEPTH_TEST);
   glDepthFunc(GL_LEQUAL);                   // The Type Of Depth Test To Do
   glClearDepth(1.0);                      // Enables Clearing Of The Depth Buffer
   glClearStencil(0x0);

   glEnable(GL_CULL_FACE);

   glDisable(GL_BLEND);
   glDisable(GL_DEPTH_TEST);
   glColor4f(1,1,1,1);

   glDepthFunc(GL_ALWAYS);
   glEnable(GL_TEXTURE_2D);

   glDisable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
   glDisable(GL_ALPHA_TEST);
   glAlphaFunc(GL_GREATER, 0.001f);
   glLineWidth(1.0);

   glGenTextures(1, &mOffscreen);
   glBindTexture(GL_TEXTURE_2D, mOffscreen);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,GL_NEAREST);
   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT, 0, GL_BGRA, GL_UNSIGNED_BYTE, 0);
   glBindTexture(GL_TEXTURE_2D, 0);
#else
//pure 2d software functions
//TODO: adapt to window size
    mBuffer=  (unsigned int*)malloc(DEFAULT_WINDOW_WIDTH*DEFAULT_WINDOW_HEIGHT*4);

#endif
//   glEnable(GL_POLYGON_OFFSET_LINE);
//   glPolygonOffset(1.0, -10.0);

   mainDemo();

   mTimer = SDL_GetTicks();
   mUpdateTimer = SDL_GetTicks();
   mTimer_isActive = TRUE;
   mUpdateTimer_isActive = TRUE;
}

void WosSdl1Gl::resizeGL(int width, int height)
{
   int w,h;

   w= (height * DEFAULT_WINDOW_WIDTH / DEFAULT_WINDOW_HEIGHT );

   if (width >= w)
   {
      // pillar box
      int left= (width-w) >> 1;
#ifndef NO_OPENGL_THANKS
      glViewport(left,0,w,height);
#else
    //TODO: scale to window bitmap keeping aspect ratio
    //we should calculate aspect ratio
#endif
   }
   else
   {
      // letter box
      h= (width * DEFAULT_WINDOW_HEIGHT / DEFAULT_WINDOW_WIDTH);
      int top= (height - h) >> 1;
#ifndef NO_OPENGL_THANKS
      glViewport(0,top,width,h);
#else
    //TODO: scale to window bitmap keeping aspect ratio
    //we should calculate aspect ratio
#endif
   }
}

#ifdef WIN32
unsigned char* align(unsigned char* ptr, int bytes)
{
   unsigned int address= (unsigned int)ptr;
   address&=~((1<<(bytes-1))-1);
   return (unsigned char*)address;
}
#endif

void WosSdl1Gl::setBitplane(int id, unsigned char* buffer, int pitch, int starty, int height)
{
   int i;

   if (id<0 || id>7)
      return; // wtf!!

   if (!buffer)
      memset(mC2PPlanes[id], 0, DEFAULT_WINDOW_WIDTH*180/8);

   Bitplane* bpl= &mBitplane[id];

   // make sure buffer and pitch are 16bit aligned
#ifdef WIN32
   buffer= align(buffer, 2);
#endif

//   pitch &= ~1;

   for (i=0; i<starty; i++)
   {
      bpl->line[i]= 0;
   }
   for (i=0; i<height; i++)
   {
      bpl->line[starty+i]= buffer;
      if (buffer)
         buffer+=pitch;
   }
   for (i=starty+height; i<512; i++)
   {
      bpl->line[i]= 0;
   }
}

void WosSdl1Gl::setBitplaneLine(int id, int y, unsigned char* buff)
{
   Bitplane* bpl= &mBitplane[id];
   bpl->line[y]= buff;
}

void WosSdl1Gl::setOddEvenShift(int id, unsigned char shift)
{
   if (id>=0 && id<2)
      mOddEvenShift[id]= shift;
}

void WosSdl1Gl::setBitplaneLines(int id, unsigned char** linePointer, int starty, int height)
{
   int i;

   if (id<0 || id>7)
      return; // wtf!!

   Bitplane* bpl= &mBitplane[id];

   for (i=0;i<starty;i++)
   {
      bpl->line[i]= 0;
   }

   for (i=0; i<height; i++)
   {
      bpl->line[starty+i]= linePointer[i];
   }

   for (i=starty+height; i<512; i++)
   {
      bpl->line[i]= 0;
   }
}


// copy one bitplane of an 8bit image
// also works for brgb ham8
void WosSdl1Gl::copyBitplane(
      unsigned char* dst,
      int dstPitch,
      unsigned char* src,
      int srcPitch,
      int srcWidth,
      int srcHeight,
      int bitid)
{
   int x,y,i;

//   dstPitch -= srcWidth>>3;
   for (y=0; y<srcHeight; y++)
   {
      if (!src)
         memset(dst, 0, srcWidth);
      else
      {
         int outpos= 0;
         for (x=0; x<srcWidth; x+=8)
         {
            unsigned char bits= 0;

            for (i=0; i<8; i++)
            {
               unsigned char v= src[x+i];
               if (v>=mSaturate) v=mSaturate-1;

               unsigned char bit= v >> bitid & 1;
               bits |= (bit<<i);
            }
            dst[outpos++]= bits;
         }
         src+=srcPitch;
      }
      dst+=dstPitch;
   }
}

// copy one bitplane of an 8:8:8:8 argb image
void WosSdl1Gl::copyBitplaneRGB3(
      unsigned char* dst8,
      int dstPitch,
      unsigned char* src,
      int srcPitch,
      int srcWidth,
      int srcHeight,
      int bitid)
{
   int x,y,i;

//   dstPitch -= srcWidth>>3;
   for (y=0; y<srcHeight; y++)
   {
      unsigned int* dst32= (unsigned int*)dst8;
      int pos= 0;
      for (x=0; x<srcWidth; x+=32)
      {
         unsigned int bits= 0;

         for (i=0; i<33; i++)
         {
            unsigned char v= src[(pos&~3)+2-(pos&3)];
            if (v>=mSaturate) v=mSaturate-1;

            unsigned int bit= v >> bitid & 1;
            if (i!=32)
               bits |= (bit<<i);
            pos++;
            if ((pos&3)==3) pos++;
         }

         *dst32++= bits;
      }
      dst8+=dstPitch;
      src+=srcPitch;
   }
}

// copy one bitplane of an 8:8:8:8 argb image
void WosSdl1Gl::copyBitplaneRGB4(
      unsigned char* dst8,
      int dstPitch,
      unsigned char* src,
      int srcPitch,
      int srcWidth,
      int srcHeight,
      int bitid)
{
   int x,y,i;

//   dstPitch -= srcWidth>>3;
   for (y=0; y<srcHeight; y++)
   {
      unsigned char bits= 0;
      for (x=0; x<srcWidth; x++)
      {
         i= x & 7;

         unsigned char v= src[x];
         if (v>=mSaturate) v=mSaturate-1;

         unsigned char bit= v >> bitid & 1;
         bits |= (bit<<i);

         if (i==7)
         {
            *dst8++= bits;
            bits= 0;
         }
      }
      dst8+=dstPitch - (srcWidth>>3);
      src+=srcPitch;
   }
}


void WosSdl1Gl::updateBuffer(unsigned char* src)
{
   if (mMode==-1)
   {
      int x,y;
      unsigned int* src32= (unsigned int*)src;
      unsigned int* dst= mBuffer;
      for (y=0;y<180;y++)
      {
         for (x=0;x<320;x++)
         {
            unsigned int col= *src32++;
            *dst++= col;
            *dst++= col;
         }
      }
      return;
   }

   for (int i=0; i<8; i++)
   {
      if (mHamChannels == 3)
      {
         if (i>=mC2PFirstBit && i<mC2PLastBit)
            copyBitplaneRGB3(
               mC2PPlanes[i+mC2PTarget-mC2PFirstBit],
               DEFAULT_WINDOW_WIDTH/8, // 640 / 8bit
               src,
               220*4, //
               DEFAULT_WINDOW_WIDTH,
               180,
               i);
      }
      else
      if (mHamChannels == 4)
      {
         if (i>=mC2PFirstBit && i<mC2PLastBit)
            copyBitplaneRGB4(
               mC2PPlanes[i+mC2PTarget],
               DEFAULT_WINDOW_WIDTH/8, // 640 / 8bit
               src,
               160*4, //
               DEFAULT_WINDOW_WIDTH,
               180,
               i+mC2PFirstBit);
      }
      else
      {
         if (i>=mC2PFirstBit && i<mC2PLastBit)
         copyBitplane(
            mC2PPlanes[i+mC2PTarget-mC2PFirstBit],
            (320)>>2,
            src,
            DEFAULT_WINDOW_WIDTH >> mScaleX,
            DEFAULT_WINDOW_WIDTH >> mScaleX,
            DEFAULT_WINDOW_HEIGHT >> mScaleY,
            i);
      }
   }
}


void WosSdl1Gl::display8()
{
   int x,y;
   unsigned int* dst= mBuffer;
   unsigned char used[256];
   memset(used, 0, 256);
   const int height= DEFAULT_WINDOW_HEIGHT >> mScaleY;

   for (y=0;y<height;y++)
   {
      unsigned int* copper= &mCopperColorData[y<<8];
      for (int sx=0; sx<DEFAULT_WINDOW_WIDTH; sx++)
      {
         unsigned int col32= 0;
         int x= sx>>mScaleX;
         unsigned char merge= 0;
         int pos= (x>>3)+y*80;
         for (int i=0; i<8; i++)
         {
            int bit= mC2PPlanes[i][pos] >> (x & 7) & 1;
            merge |= bit << i;
         }

         unsigned char col8= merge;
         if (mCopperColorEnabled[col8])
            col32= copper[col8 ];
         else
            col32= wosPal[col8];

         used[col8]++;

         *dst++= col32;
      }
   }

   mMaxNumColors= 0;
   for (int i=0;i<256;i++)
      if (used[i]) mMaxNumColors++;

#ifndef NO_OPENGL_THANKS
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glOrtho(0, DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT, 0, -1, +1);

   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();

   // upload image to texture
   glBindTexture(GL_TEXTURE_2D, mOffscreen);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT, 0, GL_BGRA, GL_UNSIGNED_BYTE, mBuffer);

   float v;
   if (mScaleY>0)
      v= 0.5f / mScaleY;
   else
      v= 1.0f;
   // draw fullscreen quad with texture-coordinates to use 160x120 of 256x128 (texture size)
   glBegin(GL_QUADS);
   glTexCoord2f(1,0); glVertex2i( DEFAULT_WINDOW_WIDTH, 0 );
   glTexCoord2f(0,0); glVertex2i( 0, 0 );
   glTexCoord2f(0,v); glVertex2i( 0, DEFAULT_WINDOW_HEIGHT );
   glTexCoord2f(1,v); glVertex2i( DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT  );
   glEnd();

   glBindTexture(GL_TEXTURE_2D, 0);
#else

#endif
}


void updateRGB(unsigned char v, unsigned char* r, unsigned char* g, unsigned char* b)
{
   unsigned char o= v & 3;
   v= v & 0xfc;
   switch(o)
   {
   case 0: // 64 colors base palette
      *r= wosPal[v>>2]>>16&255;
      *g= wosPal[v>>2]>>8&255;
      *b= wosPal[v>>2]&255;
      break;
   case 1: // modify blue
         *b= v;
      break;
   case 2: // modify red
         *r= v;
      break;
   case 3: // modify green
      *g= v;
      break;
   }
}

void WosSdl1Gl::display18()
{
   int x,y;

   if (mMode>=0)
   {
      unsigned int* dst= mBuffer;
      for (y=0;y<180;y++)
      {
         unsigned char r= 0;
         unsigned char g= 0;
         unsigned char b= 0;

         for (x=0;x<DEFAULT_WINDOW_WIDTH;x++)
         {
            unsigned char merge= 0;
            int pos= (x>>3)+y*80;
            for (int i=0; i<8; i++)
            {
               int bit= mC2PPlanes[i][pos] >> (x & 7) & 1;
               merge |= bit << i;
            }
            updateRGB(merge, &r, &g, &b);
            *dst++= (r<<16)|(g<<8)|b;
         }
      }
   }

#ifndef NO_OPENGL_THANKS

   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glOrtho(0, DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT, 0, -1, +1);

   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();

   // upload image to texture
   glBindTexture(GL_TEXTURE_2D, mOffscreen);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT, 0, GL_BGRA, GL_UNSIGNED_BYTE, mBuffer);

   float v= 0.5f / mScaleY;
   // draw fullscreen quad with texture-coordinates to use 160x120 of 256x128 (texture size)
   glBegin(GL_QUADS);
   glTexCoord2f(1,0); glVertex2i( DEFAULT_WINDOW_WIDTH, 0 );
   glTexCoord2f(0,0); glVertex2i( 0, 0 );
   glTexCoord2f(0,v); glVertex2i( 0, DEFAULT_WINDOW_HEIGHT );
   glTexCoord2f(1,v); glVertex2i( DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT  );
   glEnd();

   glBindTexture(GL_TEXTURE_2D, 0);
#else

#endif
}

void WosSdl1Gl::mergeBitplanes(unsigned char* dst)
{
/*
   unsigned char bitcache[8];

   for (int y=0; y<yres*mScaleY; y++)
   {
      // copy scanline, merge bits
      for (int x=0; x<DEFAULT_WINDOW_WIDTH; x+=8)
      {
         for (int i=0;i<8;i++)
         {
            // get bitplane pointers for current scanline
            Bitplane* bpl= &mBitplane[(i) & 7];
            if (bpl->line[y]==0)
               bitcache[i]= 0;
            else
            {
               unsigned char* data= bpl->line[y] + (x>>3);
               unsigned char shift= mOddEvenShift[i&1];
               // get 24 bit in advance, so we can shift 16 bits and still got 8 left.
               unsigned int bits= (data[0] << 16) | (data[1] << 8) | (data[2]);
               bitcache[i]= (bits << shift) >> 16;
            }
         }

         for (int i=0; i<8; i++)
         {
            unsigned char color= 0;
            for (int j=0; j<8; j++)
            {
               unsigned char bits= (bitcache[j]>>(7-i)&1)<<j;
               color |= bits;
            }
            dst[x+i]= color;
         }
      }

      dst+=xres*mScaleX;
   }
*/
}

void WosSdl1Gl::paintGL()
{
   if (mUpdateTimer_isActive) {
      // stop mUpdateTimer

      mUpdateTimer_isActive = FALSE;
   }

   /*qint64*/long long int elapsed;

#ifndef FAST_FORWARD

#ifdef WIN32
   // wait for next frame!
   int frame= 0;
   //   do {
    MMTIME	mmt;
    mmt.wType = TIME_SAMPLES;
    waveOutGetPosition(mHandle, &mmt, sizeof(MMTIME));
    frame= (mmt.u.sample + sample1) * 50 / audioRate;
   //   } while (frame < g_vbitimer);
   g_vbitimer= frame;
#else
      g_vbitimer= mTimer_elapsed(mTimer)*refresh/1000;
#endif

#else
   // for video capture
   g_vbitimer++;
#endif

   elapsed= g_vbitimer*1000/refresh;


   if (g_renderedFrames > mLastRenderedFrame)
   {
      drawDemo( g_vbitimer );
      mLastRenderedFrame= g_renderedFrames;
   }
   else
   {
        // Sleep the remaining frame time
        // if (elapsed < (1000 / refresh)
        // SDL_Delay( ( 1000 / refresh ) - elapsed );
   }

   // update palettes and do bitplane magic
   oddeven^=1;
   updateDemo( g_vbitimer );

//   mergeBitplanes(mBuffer8);

   if (mMode == -1)
      display18();
   else if (mMode == 11 || mMode==17 || mMode==18 || mMode==19 || mMode==20 || mMode==21 || mMode==22 || mMode==23)
      display18();
   else
      display8();

   {
       //uncomment to record a video
       char temp[256];
       sprintf(temp, "f:\\encode2\\anim%05d.bmp", g_vbitimer);
       //   saveBmp(temp, mBuffer, xres*4, yres);
   }


   char title[80];
   // update fps
   g_renderedFrames++;
   float fps = 0.0;
   if (elapsed - mLastFpsTime >= 100 || mLastFpsTime > elapsed)
   {
      fps = (g_renderedFrames - mLastFpsFrame) * 1000.0f / (elapsed - mLastFpsTime);
      mLastFpsFrame = g_renderedFrames;
      mLastFpsTime = elapsed;

      sprintf(title,"frame: %d [%.2f fps] [%d colors] [mouseXY: %d,%d]", g_renderedFrames, fps, mMaxNumColors, mouseX, mouseY);
      //mWindow->setWindowTitle( QString("frame: %1 [%2 fps] [%3 colors]").arg().arg().arg() );
      SDL_WM_SetCaption(title, 0);
      mMaxNumColors= 0;
   }

   int interval = 0;
#ifndef FAST_FORWARD
   // adaptively adjust to 50Hz
   interval= (g_renderedFrames*1000/refresh) - elapsed - 10;
   if (interval <= 0)
   {
      // skip frames
      g_renderedFrames += (-interval*refresh/1000);
      interval= 1;
   }
   if (interval > 15)
      interval= 15;
#endif

   // schedule update of next frame
   mTimer_setInterval(&mUpdateTimer, interval);
   mUpdateTimer = SDL_GetTicks();
//   qDebug("%d: %d", (int)elapsed, (int)interval);
}


bool WosSdl1Gl::eventFilter(void* watched, SDL_Event* event)
{
   /*if (event.type == SDL_KEYDOWN)
   {
      keyPressEvent(event);
      return true;
   } else if (event.type == SDL_VIDEORESIZE) {
       //ResizeWindow(&event);
       //resizeGL(event.resize.w, event.resize.h);
   } else if (event.type == SDL_QUIT) {
       //qApp->quit();
   }*/
   return false;
}


void WosSdl1Gl::keyPressEvent(SDL_Event* event)
{
    // exit if ESCAPE is pressed
    /*if (event.key.keysym.sym == SDLK_ESCAPE) {
        //qApp->quit();
    } else if (event.key.keysym.sym == SDLK_SPACE) {
        restartTiming();
        mTimer = SDL_GetTicks();
        g_renderedFrames= 0;
        mLastRenderedFrame= 0;
    } else if (event.key.keysym.sym == SDLK_RIGHT) {
        restartTiming();
    }*/
}


void WosSdl1Gl::wheelEvent(SDL_Event* me)
{
//   mouseZ += me->delta();
}

void WosSdl1Gl::mouseMoveEvent(SDL_Event* me)
{
/*   if (me->buttons() & Qt::LeftButton)
   {
      QPoint delta= me->pos() - mStartPos;
      mouseX += delta.x();
      mouseY += delta.y();
      mStartPos= me->pos();
   }*/
}

void WosSdl1Gl::mousePressEvent(SDL_Event* me)
{
/*    SDL_MOUSEMOTION,			// Mouse moved
    SDL_MOUSEBUTTONDOWN,		// Mouse button pressed
    SDL_MOUSEBUTTONUP,		// Mouse button released

   if (me->button() == Qt::LeftButton)
   {
      mStartPos= me->pos();
   }*/
}

void WosSdl1Gl::initFileWatcher(char** filenames, void(*callback)())
{
/*   mWatchList.clear();
   mCallback= callback;
   while (*filenames)
   {
      const char* filename= *filenames++;
      mWatchList.append( QString(filename) );
   }
*/
   initFileWatcher();

}


void WosSdl1Gl::initFileWatcher()
{
    //TODO: use inotify on posix and
/*   if (mWatcher)
      delete mWatcher;
   mWatcher= new QFileSystemWatcher(this);
   connect(mWatcher, SIGNAL(fileChanged(QString)), mReloadDelay, SLOT(start()));
   foreach (const QString& filename, mWatchList)
      mWatcher->addPath(filename);*/
}


void WosSdl1Gl::triggerReload()
{
   mCallback();
   initFileWatcher();
}

void WosSdl1Gl::setCopper(unsigned char index, unsigned int* gradient)
{
   if (gradient==0)
      mCopperColorEnabled[index]= 0;
   else
   {
      mCopperColorEnabled[index]= 1;
      for (int i=0;i<180;i++)
         mCopperColorData[(i<<8)|index]= gradient[i];
   }
}

void WosSdl1Gl::mainLoop()
{
    bool done = false;
    resizeGL(DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT);

        // load an image
/*    SDL_Surface* bmp = SDL_LoadBMP("C:/Jaime/haujobb-amiga-master/demo/hellodemo/cb.bmp");
    if (!bmp)
    {
        printf("Unable to load bitmap: %s\n", SDL_GetError());
    }*/

    while (!done)
    {

        // message processing loop
        while (SDL_PollEvent(&event))
        {
            // check for messages
            switch (event.type)
            {
                case SDL_VIDEORESIZE:
                    //! resize gl overwritten
                    resizeGL(event.resize.w, event.resize.h);
                    break; // resizing called here
                case SDL_QUIT:
                    // exit if the window is closed
                    done = true;
                    break;
                    // check for keypresses
                case SDL_KEYDOWN:
                        // exit if ESCAPE is pressed
                    if (event.key.keysym.sym == SDLK_ESCAPE) {
//                        qApp->quit();
                        done = true;
                    } else  if (event.key.keysym.sym == SDLK_SPACE) {
                       restartTiming();
                       mTimer = SDL_GetTicks(); //mTimer_restart();
                       mTimer_isActive = TRUE;
                       g_renderedFrames= 0;
                       mLastRenderedFrame= 0;
                    } else  if (event.key.keysym.sym == SDLK_RIGHT) {
                       restartTiming();
                    } else if (event.key.keysym.sym == SDLK_r) {
                        //force reload of list of files
                        triggerReload();
                    }
                    break;
                case SDL_MOUSEBUTTONDOWN:
                    if(event.button.button == SDL_BUTTON_LEFT){
                        mouseX = event.button.x;
                        mouseY = event.button.y;
                        mStartPos_x= event.button.x;
                        mStartPos_y= event.button.y;
                    } else if (event.button.button == SDL_BUTTON_WHEELUP) {
                        mouseX = event.button.x;
                        mouseY = event.button.y;
                        mouseZ += 1;
                    } else if (event.button.button == SDL_BUTTON_WHEELDOWN) {
                        mouseX = event.button.x;
                        mouseY = event.button.y;
                        mouseZ -= 1;
                    }
                    break;
                case SDL_MOUSEMOTION:
                    mouseX = event.motion.x;
                    mouseY = event.motion.y;
                    break;

                //TODO: add all the events calls, timer increasing, etc...


                // square.Handle_Input(down); break;
                // case SDL_KEYUP: square.Handle_Input(up); break;
            } // end switch
        } // end of message processing

        // DRAWING STARTS HERE

   //! paint gl overwritten
        paintGL();


#ifndef NO_OPENGL_THANKS
        SDL_GL_SwapBuffers();
#else
/*
    int bpp = surface->format->BytesPerPixel;
    Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;
*/

/*        SDL_Surface *tmpSurface = SDL_CreateRGBSurfaceFrom(mBuffer,
                        DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT, 32, DEFAULT_WINDOW_WIDTH,
                        0xFF0000, 0x00FF00, 0x0000FF, 0);*/
        sdlSetPalette(screen);

//        SDL_FillRect(screen, 0, SDL_MapRGB(screen->format, 0, 0, 0));

        SDL_LockSurface(screen);
        unsigned long int *pixels=(unsigned long int *)screen->pixels;
        unsigned long int *mBufferPtr=(unsigned long int *)mBuffer;

        if (screen->w != DEFAULT_WINDOW_WIDTH || screen->h != DEFAULT_WINDOW_HEIGHT) {
           int w,h;
           float xStep=1.0;
           float yStep=1.0;

/*           SDL_Rect dstrect;
           dstrect.x = 0; // (screen->w - 640) /2;
           dstrect.y = 0; // (screen->h - 360) /2;
           dstrect.w = DEFAULT_WINDOW_WIDTH;
           dstrect.h = DEFAULT_WINDOW_HEIGHT;*/

           w= (screen->h * DEFAULT_WINDOW_WIDTH / DEFAULT_WINDOW_HEIGHT );

           if (screen->w >= w)
           {
              // pillar box
              int left= (screen->w - w) >> 1;
//              glViewport(left,0,w,height);
              //TODO: scale to window bitmap keeping aspect ratio


           }
           else
           {
              // letter box
              h= (screen->w * DEFAULT_WINDOW_HEIGHT / DEFAULT_WINDOW_WIDTH);
              int top= (screen->h - h) >> 1;
//              glViewport(0,top,width,h);
            //TODO: scale to window bitmap keeping aspect ratio
           }
        } else {
            int widthInBytes = DEFAULT_WINDOW_WIDTH*4;
            for (int ydst=0,ysrc=0;ydst<DEFAULT_WINDOW_HEIGHT;ydst+=2, ysrc++) {
                memcpy(pixels+(DEFAULT_WINDOW_WIDTH*ydst),mBuffer+(DEFAULT_WINDOW_WIDTH*ysrc), widthInBytes);
                memcpy(pixels+(DEFAULT_WINDOW_WIDTH*(ydst+1)),mBuffer+(DEFAULT_WINDOW_WIDTH*ysrc), widthInBytes);   //double lines
            }
        }

        SDL_UnlockSurface(screen);
        SDL_Flip(screen);

#endif
    } // end main loop

    //TODO typical SDL loop
}


#ifdef NO_OPENGL_THANKS
void sdlSetPalette(SDL_Surface *screen) {
    SDL_Color colors[256];
    int i=0;
    for(i=0;i<256;i++){
      colors[i].r=(wosPal[i]&0xFF0000)>>16;
      colors[i].g=(wosPal[i]&0x00FF00)>>8;
      colors[i].b=(wosPal[i]&0x0000FF);
    }
    SDL_SetPalette(screen, SDL_LOGPAL/*|SDL_PHYSPAL*/, colors, 0, 256);
}
#endif
