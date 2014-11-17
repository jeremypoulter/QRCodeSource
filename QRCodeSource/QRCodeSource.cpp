#include <windows.h>
#include <stdio.h>
#include <time.h>

#include "avisynth.h"
#include "LibQREncode/qrencode.h"

//#define OUTBUG

#define OUT_FILE_PIXEL_PRESCALER	1											// Prescaler (number of pixels in bmp file for each QRCode pixel, on each dimension)
#define BOADER_SIZE               1

#define PIXEL_COLOR_R				0											// Color of bmp pixels
#define PIXEL_COLOR_G				0
#define PIXEL_COLOR_B				0

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


class QRCodeSource : public IClip 
{
private:
  const   VideoInfo vi;
  bool    parity;
  int     last;                                              // remember last frame generated
  PVideoFrame frame;
  QRcode *pQRC;                                              // QR code 

public:
  QRCodeSource(const VideoInfo& _vi,PVideoFrame _frame, bool _parity, QRcode *_code)
    : vi(_vi), frame(_frame),parity(_parity),pQRC(_code), last(-1) {}
  ~QRCodeSource(){
    QRcode_free(pQRC);
  }
  void __stdcall GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env) {}
  const VideoInfo& __stdcall GetVideoInfo() { return vi; }
  bool __stdcall GetParity(int n) { return (vi.IsFieldBased() ? (n&1) : false) ^ parity; }
  int __stdcall SetCacheHints(int cachehints,int frame_range) { return 0; }
  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
};

PVideoFrame __stdcall QRCodeSource::GetFrame(int n, IScriptEnvironment* env) 
{
  // Range limit
  n = (n < 0) ? 0 : (n >= vi.num_frames) ? vi.num_frames - 1 : n;
  if(n != last) 
  {
    // Try to reuse old frame
    BYTE* p = frame->GetWritePtr();
    if  (!p) 
    {
      // Ditch old frame and create a new frame
      frame = env->NewVideoFrame(vi);
      p = frame->GetWritePtr();
    }

    unsigned int unWidth, x, y, l, uiPixelColour;
    unsigned char *pSourceData;
    unsigned int *pDestData;

    unWidth = pQRC->width;

    // Write the QrCode bits to the frame
    pSourceData = pQRC->data;
    for(y = 0; y < unWidth; y++)
    {
      for(x = 0; x < unWidth; x++)
      {
        uiPixelColour = (*pSourceData & 1) ? 0x00000000 : 0x00ffffff;

        for(l = 0; l < OUT_FILE_PIXEL_PRESCALER; l++)
        {
          for(n = 0; n < OUT_FILE_PIXEL_PRESCALER; n++)
          {
            pDestData = (unsigned int *)(p + 
              ((BOADER_SIZE + ((unWidth - y - 1) * OUT_FILE_PIXEL_PRESCALER) + l) * frame->GetPitch()) + 
              (BOADER_SIZE + (x * OUT_FILE_PIXEL_PRESCALER) + n) * 4);
            *pDestData = uiPixelColour;
          }
        }

        pSourceData++;
      }
    }

    // Write out the border
    for(y = 0; y < BOADER_SIZE; y++)
    {
      for(x = 0; x < frame->GetPitch(); x += 4)
      {
        pDestData = (unsigned int *)(p + y * frame->GetPitch() + x);
        *pDestData = 0x00FFFFFF;
      }
    }

    for(y = BOADER_SIZE; y < BOADER_SIZE + (unWidth * OUT_FILE_PIXEL_PRESCALER); y++)
    {
      for(x = 0; x < BOADER_SIZE; x++)
      {
        pDestData = (unsigned int *)(p + (y) * frame->GetPitch() + (x * 4));
        *pDestData = 0x00FFFFFF;
        pDestData = (unsigned int *)(p + (y) * frame->GetPitch() + ((BOADER_SIZE + (unWidth * OUT_FILE_PIXEL_PRESCALER) + x) * 4));
        *pDestData = 0x00FFFFFF;
      }
    }

    for(y = BOADER_SIZE + (unWidth * OUT_FILE_PIXEL_PRESCALER); y < frame->GetHeight(); y++)
    {
      for(x = 0; x < frame->GetPitch(); x += 4)
      {
        pDestData = (unsigned int *)(p + (y) * frame->GetPitch() + x);
        *pDestData = 0x00FFFFFF;
      }
    }

    // Remember last generated frame number
    last = n;
  }
  return frame;
}

static AVSValue __cdecl Create_QRCodeSource(AVSValue args, void*, IScriptEnvironment* env) {
  const char *string = args[0].AsString("");                        // text to encode, defaults to ""
  if(string == NULL) {
    string = "";
  }
  int version = args[1].AsInt(0);
  const char *error_correction = args[2].AsString("H");;
#ifdef OUTBUG
  dprintf("RandomSource: Seed = %d (0x%08X)\n",string,string);
#endif

  QRecLevel level = QR_ECLEVEL_H;

  if('L' == error_correction[0]) {
    level = QR_ECLEVEL_L;
  } else if('M' == error_correction[0]) {
    level = QR_ECLEVEL_M;
  } else if('Q' == error_correction[0]) {
    level = QR_ECLEVEL_Q;
  }

  QRcode *pQRC = QRcode_encodeString(string, version, level, QR_MODE_8, 1);
  if(NULL != pQRC)
  {
    VideoInfo vi;
    memset(&vi, 0, sizeof(VideoInfo));
    vi.width                    = pQRC->width * OUT_FILE_PIXEL_PRESCALER + 2 * BOADER_SIZE;
    vi.height                   = pQRC->width * OUT_FILE_PIXEL_PRESCALER + 2 * BOADER_SIZE;
    vi.fps_numerator            = 24;
    vi.fps_denominator          = 1;
    vi.num_frames               = 24*60*60;
    vi.pixel_type               = VideoInfo::CS_BGR32;
    vi.SetFieldBased(false);
    bool parity=false;
    PVideoFrame frame = env->NewVideoFrame(vi);         // Create initial frame
    return new QRCodeSource(vi, frame, parity, pQRC);
  }

  return NULL;
}

const AVS_Linkage *AVS_linkage = 0;

extern "C" __declspec(dllexport) const char* __stdcall AvisynthPluginInit3(IScriptEnvironment* env, const AVS_Linkage* const vectors) {
  AVS_linkage = vectors;
  env->AddFunction("QRCodeSource","s[version]i[error_correction]s",Create_QRCodeSource, 0);
  return "`QRCodeSource' QRCodeSource plugin";
}