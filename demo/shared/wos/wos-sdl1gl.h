#ifndef WOSSDL1GL_H
#define WOSSDL1GL_H 1

#ifndef AMIGA
#include <SDL/SDL.h>

#define DEFAULT_WINDOW_WIDTH 640
#define DEFAULT_WINDOW_HEIGHT 360

#ifndef NO_OPENGL_THANKS
#include <GL/gl.h>
#include <GL/glu.h>
#else
void sdlSetPalette(SDL_Surface *screen);
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
   unsigned char* mPointer;  // 16bit aligned
//   int            mShift;    // 0..15
} BitplaneLine;

extern int xres,yres;
extern int ocs;
extern int oddeven;
extern int movex, movey, movez;
extern unsigned char* screenBuffer;
extern int g_vbitimer;
extern int g_renderedFrames;

extern void wosInit();
extern void wosSetMode(int mode, void* buffer, void* pal, int);
extern void wosDisplay(int mode);
extern int wosCheckExit();
extern void wosClearPlanes();
extern void wosClearExit();
extern void wosSetCols(void* palette, int brightness);
extern void wosSetupBitplane(int bpl, unsigned char* buffer, int pitch, int starty, int height);
extern void wosSetupBitplaneLines(int bpl, unsigned char** linePointer, int starty, int height);
extern void wosSetupBitplaneLine(int bpl, int line, unsigned char* buff);
extern void wosSetupCopper(int index, void* gradient);
extern void wosSetMusicStart(int miliseconds);
extern void wosSetupMusic(int samplerate, short* left, short* right, int length);
extern void wosSetPlayfieldShift(int playfieldId, unsigned char shift);
extern int wosMouseX();
extern int wosMouseY();
extern int wosMouseZ();

unsigned char* wosAllocChipMem(int bytes);
void wosFreeChipMem(unsigned char* buffer, int size);

extern void wosInitFileWatcher(void* filenames, void(*callback)());

#ifdef __cplusplus
};
#endif

#ifdef __cplusplus

class WosSdl1Gl
{
//   Q_OBJECT

public:
   struct Bitplane
   {
      unsigned char* line[512];
   };

   //! constructor
   WosSdl1Gl(void /*const Uint32& format*/);

   //! destructor
   ~WosSdl1Gl();

   void setMode(int mode);
   void setBitplane(int id, unsigned char* buffer, int pitch, int starty, int height);
   void setBitplaneLine(int id, int line, unsigned char* buff);
   void setBitplaneLines(int id, unsigned char** linePointer, int starty, int height);
   void setOddEvenShift(int id, unsigned char shift);
   void setCopper(unsigned char index, unsigned int* gradient);
   void display8();
   void display18();
   void updateBuffer(unsigned char* src);

   void copyBitplaneRGB3(unsigned char* dst, int dstPitch, unsigned char* src, int srcPitch, int srcWidth, int srcHeight, int bitid);
   void copyBitplaneRGB4(unsigned char* dst, int dstPitch, unsigned char* src, int srcPitch, int srcWidth, int srcHeight, int bitid);
   void copyBitplane(unsigned char* dst, int dstPitch, unsigned char* src, int srcPitch, int srcWidth, int srcHeight, int bitid);

   void initFileWatcher(char** filenames, void(*callback)());
   void initFileWatcher();
   bool eventFilter(void* watched, SDL_Event* event);

public:
   void triggerReload();
   void mainLoop();
protected:
   //! inialize gl context
   void initializeGL();

   //! paint gl overwritten
   void paintGL();

   //! resize gl overwritten
   void resizeGL(int width, int height);

   void keyPressEvent(SDL_Event* event);
   void mouseMoveEvent(SDL_Event* event);
   void mousePressEvent(SDL_Event* event);
   void wheelEvent(SDL_Event* event);

private:
   void mergeBitplanes(unsigned char* dst);

   int                     mLastFpsTime;  //!< frame time
   int                     mLastFpsFrame;
   Uint32                  mTimer;
   bool                    mTimer_isActive;
   int                     mMode;
   SDL_Surface*            screen;
   SDL_Event               event;
   unsigned int*           mBuffer;
   unsigned int            mOffscreen;
   /* is this used at all? QElapsedTimer           mTime;*/
   Uint32                  mUpdateTimer;
   bool                    mUpdateTimer_isActive;
   int                     mLastRenderedFrame;
   int                     mMaxNumColors;
   int                     mC2PFirstBit;
   int                     mC2PLastBit;
   int                     mC2PTarget;
   int                     mHamChannels;
   unsigned char*          mC2PPlanes[8];
   Bitplane                mBitplane[8];

   /* file reloader...
   QStringList             mWatchList;
   QFileSystemWatcher*     mWatcher; */
   Uint32*                 mReloadDelay; // timer for reloading files

   char                    mCopperColorEnabled[512];
   unsigned int*           mCopperColorData;
   int                     mScaleX;
   int                     mScaleY;
   void(*mCallback)();
   unsigned int*           mCaptureBuffer;
   unsigned int*           mBlendBuffer;
   unsigned char           mOddEvenShift[2];
   int                     mSaturate;
/* QPoint                  mStartPos; */
   int                     mStartPos_x;
   int                     mStartPos_y;

};

#endif

#endif
