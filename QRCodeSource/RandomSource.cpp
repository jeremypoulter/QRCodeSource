#include <windows.h>
#include <stdio.h>
#include <time.h>
#include "avisynth.h"

//#define OUTBUG

#ifdef OUTBUG
int dprintf(char* fmt, ...) {
  char printString[2048];
  char *p=printString;
  va_list argp;
  va_start(argp, fmt);
  vsprintf(p, fmt, argp);
  va_end(argp);
  for(;*p++;);
  --p;                                                            // @ null term
  if(printString == p || p[-1] != '\n') {
    p[0]='\n';                                                  // append n/l if not there already
    p[1]='\0';
    OutputDebugString(printString);
    p[0]='\0';                                                  // remove appended n/l
  } else {
    OutputDebugString(printString);
  }
  return p-printString;                                           // strlen printString
}
#endif


class RandomSource : public IClip {
private:
  const   VideoInfo vi;
  bool    parity;
  int     last;                                                       // remember last frame generated
  PVideoFrame frame;
  //
  //
  const unsigned int UserSeed;                                        // User supplied or random (time(0))
  unsigned __int64 SetupSeed;                                         // Used to setup WELLRNG512 random bits
  //
  unsigned int state[16];                                             // need initialize state to random bits
  unsigned int index;                                                 // init should also reset this to 0
  //
  unsigned int GetSetupRand(void);
  void InitWELLRNG512(unsigned int n);    
  unsigned int WELLRNG512(void);  
  //
public:
  RandomSource(const VideoInfo& _vi,PVideoFrame _frame, bool _parity,unsigned int _userseed)
    : vi(_vi), frame(_frame),parity(_parity),UserSeed(_userseed), last(-1) {}
  ~RandomSource(){}
  void __stdcall GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env) {}
  const VideoInfo& __stdcall GetVideoInfo() { return vi; }
  bool __stdcall GetParity(int n) { return (vi.IsFieldBased() ? (n&1) : false) ^ parity; }
  int __stdcall SetCacheHints(int cachehints,int frame_range) { return 0; }
  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
};

PVideoFrame __stdcall RandomSource::GetFrame(int n, IScriptEnvironment* env) {
  n = (n < 0) ? 0 : (n >= vi.num_frames) ? vi.num_frames - 1 : n;     // Range limit
  if(n != last) {
    BYTE* p = frame->GetWritePtr();                                 // Try to reuse old frame
    if  (!p) {
      frame = env->NewVideoFrame(vi);                             // Ditch old frame and create a new frame
      p = frame->GetWritePtr();
    }
    InitWELLRNG512(n);                                              // init
    int i,size = frame->GetPitch() * frame->GetHeight();
    for (i=0; i<size; i+=4) {
      unsigned int rgb = WELLRNG512() & 0x00FFFFFF;
      *((unsigned int*)(p+i)) = rgb;
    }
    last = n;                                                       // Remember last generated frame number
  }
  return frame;
}


unsigned int RandomSource::GetSetupRand(void) {
  //  http://en.wikipedia.org/wiki/Linear_congruential_generator
  //  Numbers by Donald Knuth, MMIX.
  //
  //  X_n+1 equiv ( a * X_n + c ) % m
  //
  unsigned __int64 a = 6364136223846793005;
  unsigned __int64 c = 1442695040888963407;
  // m = 2 ^ 64
  SetupSeed = (a * SetupSeed + c);                        // % m (2 ^ 64)
  return (unsigned int)(SetupSeed >> 32);                 // Hi 32 bits, (most randomized bits)
}


void RandomSource::InitWELLRNG512(unsigned int n) {
  SetupSeed = (n + 1) * 4562693 + UserSeed;               // init the setup seed (4562693 prime picked out of the hat)
  int i;
  for(i=0;i<13;++i)                                       // Kick it through a few iterations
    GetSetupRand();
  for(i=0;i<16;++i) {
    state[i]= GetSetupRand();                           // Initialise WELLRNG512
  }
  index = 0;                                              // All systems GO!
  for(i=0;i<7;++i)                                        // Kick main rand generator through a few iterations Too
    WELLRNG512();
}


unsigned int RandomSource::WELLRNG512(void) {               // return 32 bit random number
  // Public Domain, Chriss Lomont @ http://www.lomont.org : Feb 8 2008.
  unsigned long a, b, c, d;
  a = state[index];
  c = state[(index+13) & 15];
  b = a^c^(a<<16)^(c<<15);
  c = state[(index+9) & 15];
  c ^= (c>>11);
  a = state[index] = b^c;
  d = a^((a<<5) & 0xDA442D24UL);
  index = (index + 15) & 15;
  a = state[index];
  state[index] = a^b^d^(a<<2)^(b<<18)^(c<<28);
  return state[index];
}


static AVSValue __cdecl Create_RandomSource(AVSValue args, void*, IScriptEnvironment* env) {
  int userseed = args[0].AsInt(0);                        // userseed defaults to 0 (ALWAYS SAME)
  if(userseed == -1)
    userseed = (int)(time(NULL));
#ifdef OUTBUG
  dprintf("RandomSource: Seed = %d (0x%08X)\n",userseed,userseed);
#endif
  VideoInfo vi;
  memset(&vi, 0, sizeof(VideoInfo));
  vi.width                    =640;
  vi.height                   =480;
  vi.fps_numerator            =24;
  vi.fps_denominator          =1;
  vi.num_frames               =24*60*60;
  vi.pixel_type               =VideoInfo::CS_BGR32;
  vi.SetFieldBased(false);
  bool parity=false;
  PVideoFrame frame = env->NewVideoFrame(vi);         // Create initial frame
  return new RandomSource(vi,frame, parity,(unsigned int)userseed);
}

const AVS_Linkage *AVS_linkage = 0;

extern "C" __declspec(dllexport) const char* __stdcall AvisynthPluginInit3(IScriptEnvironment* env, const AVS_Linkage* const vectors) {
  AVS_linkage = vectors;
  env->AddFunction("RandomSource","[seed]i",Create_RandomSource, 0);
  return "`RandomSource' RandomSource plugin";
}