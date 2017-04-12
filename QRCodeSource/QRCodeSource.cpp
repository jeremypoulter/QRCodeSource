#ifdef _WINDOWS
#include <windows.h>
#else
#define IN
#define OUT
typedef unsigned char BYTE;
#endif

#include <stdio.h>
#include <time.h>

#include "LibQREncode/qrencode.h"

#ifdef IS_VAPOURSYNTH

#include "VapourSynth.h"
#include "VSHelper.h"

#else

#include "avisynth.h"

#endif

//#define OUTBUG

#define OUT_FILE_PIXEL_PRESCALER	1											// Prescaler (number of pixels in bmp file for each QRCode pixel, on each dimension)
#define BOADER_SIZE               1

#define PIXEL_COLOR_R				0											// Color of bmp pixels
#define PIXEL_COLOR_G				0
#define PIXEL_COLOR_B				0

bool create_qr_instance(IN const char *string, IN int version, IN const char *error_correction, OUT QRcode **ppQRC)
{
  QRecLevel level = QR_ECLEVEL_H;

  if ('L' == error_correction[0]) {
    level = QR_ECLEVEL_L;
  }
  else if ('M' == error_correction[0]) {
    level = QR_ECLEVEL_M;
  }
  else if ('Q' == error_correction[0]) {
    level = QR_ECLEVEL_Q;
  }

  QRcode *pQRC = QRcode_encodeString(string, version, level, QR_MODE_8, 1);
  if (NULL != pQRC)
  {
    *ppQRC = pQRC;
    return true;
  }
  
  return false;
}

#ifdef IS_VAPOURSYNTH
#define COLOUR1 0x00
#define COLOUR2 0xff
#define Y_POS(h, y) (y)
typedef unsigned char PIXEL;
#else
#define COLOUR1 0x00000000
#define COLOUR2 0x00ffffff
#define Y_POS(h, y) (h - y - 1)
typedef unsigned int PIXEL;
#endif

void write_qr_code(BYTE* p, QRcode *pQRC, unsigned int pitch, unsigned int height)
{
  unsigned int unWidth, x, y, l, n;
  PIXEL uiPixelColour;
  unsigned char *pSourceData;
  PIXEL *pDestData;

  unWidth = pQRC->width;

  // Write the QrCode bits to the frame
  pSourceData = pQRC->data;
  for (y = 0; y < unWidth; y++)
  {
    for (x = 0; x < unWidth; x++)
    {
      uiPixelColour = (*pSourceData & 1) ? COLOUR1 : COLOUR2;

      for (l = 0; l < OUT_FILE_PIXEL_PRESCALER; l++)
      {
        for (n = 0; n < OUT_FILE_PIXEL_PRESCALER; n++)
        {
          pDestData = (PIXEL *)(p +
            ((BOADER_SIZE + (Y_POS(unWidth, y) * OUT_FILE_PIXEL_PRESCALER) + l) * pitch) +
            (BOADER_SIZE + (x * OUT_FILE_PIXEL_PRESCALER) + n) * sizeof(PIXEL));
          *pDestData = uiPixelColour;
        }
      }

      pSourceData++;
    }
  }

  // Write out the border
  for (y = 0; y < BOADER_SIZE; y++)
  {
    for (x = 0; x < pitch; x += sizeof(PIXEL))
    {
      pDestData = (PIXEL *)(p + y * pitch + x);
      *pDestData = COLOUR2;
    }
  }

  for (y = BOADER_SIZE; y < BOADER_SIZE + (unWidth * OUT_FILE_PIXEL_PRESCALER); y++)
  {
    for (x = 0; x < BOADER_SIZE; x++)
    {
      pDestData = (PIXEL *)(p + (y) * pitch + (x * sizeof(PIXEL)));
      *pDestData = COLOUR2;
      pDestData = (PIXEL *)(p + (y)* pitch + ((BOADER_SIZE + (unWidth * OUT_FILE_PIXEL_PRESCALER) + x) * sizeof(PIXEL)));
      *pDestData = COLOUR2;
    }
  }

  for (y = BOADER_SIZE + (unWidth * OUT_FILE_PIXEL_PRESCALER); y < height; y++)
  {
    for (x = 0; x < pitch; x += sizeof(PIXEL))
    {
      pDestData = (PIXEL *)(p + (y)* pitch + x);
      *pDestData = COLOUR2;
    }
  }
}

#ifdef IS_VAPOURSYNTH

#define FILTER_NAME "Code"

typedef struct {
  VSVideoInfo vi;
  QRcode *pQRC;
} FilterData;

static void VS_CC filterInit(VSMap *in, VSMap *out, void **instanceData, VSNode *node, VSCore *core, const VSAPI *vsapi) {
  FilterData *d = (FilterData *)* instanceData;
  vsapi->setVideoInfo(&d->vi, 1, node);
}

static const VSFrameRef *VS_CC filterGetFrame(int n, int activationReason, void **instanceData, void **frameData, VSFrameContext *frameCtx, VSCore *core, const VSAPI *vsapi) 
{
  FilterData *d = (FilterData *)* instanceData;

  if (activationReason != arInitial) {
    return NULL;
  }

  const VSFormat *fi = d->vi.format;

  VSFrameRef *dst = vsapi->newVideoFrame(fi, d->vi.width, d->vi.height, NULL, core);

  VSMap *props = vsapi->getFramePropsRW(dst);
  vsapi->propSetInt(props, "_DurationNum", d->vi.fpsDen, paReplace);
  vsapi->propSetInt(props, "_DurationDen", d->vi.fpsNum, paReplace);
  vsapi->propSetInt(props, "_SARNum", 1, paReplace);
  vsapi->propSetInt(props, "_SARDen", 1, paReplace);

  // It's processing loop time!
  // Loop over all the planes
  int plane;
  for (plane = 0; plane < fi->numPlanes; plane++) 
  {
    uint8_t *p = vsapi->getWritePtr(dst, plane);

    // note that if a frame has the same dimensions and format, the stride is guaranteed to be the same. int dst_stride = src_stride would be fine too in this filter.
    int dst_stride = vsapi->getStride(dst, plane); 
      
    // Since planes may be subsampled you have to query the height of them individually
    int h = vsapi->getFrameHeight(dst, plane);

    write_qr_code(p, d->pQRC, dst_stride, h);
  }

  // A reference is consumed when it is returned, so saving the dst reference somewhere
  // and reusing it is not allowed.
  return dst;
}

static void VS_CC filterFree(void *instanceData, VSCore *core, const VSAPI *vsapi) {
  FilterData *d = (FilterData *)instanceData;
  //vsapi->freeNode(d->node);
  QRcode_free(d->pQRC);
  free(d);
}

static void VS_CC filterCreate(const VSMap *in, VSMap *out, void *userData, VSCore *core, const VSAPI *vsapi) {
  FilterData *data;

  data = (FilterData *)calloc(1, sizeof(FilterData));
  if (NULL != data)
  {
    int err;
    const char * message = vsapi->propGetData(in, "message", 0, NULL);
    int64_t version = vsapi->propGetInt(in, "version", 0, &err);
    if(err) { version = 0; }
    const char * error_correction = vsapi->propGetData(in, "error_correction", 0, &err);
    if (err) { error_correction = "H"; }

    if(create_qr_instance(message, (int)version, error_correction, &data->pQRC))
    {
      data->vi.width = data->pQRC->width * OUT_FILE_PIXEL_PRESCALER + 2 * BOADER_SIZE;
      data->vi.height = data->pQRC->width * OUT_FILE_PIXEL_PRESCALER + 2 * BOADER_SIZE;
      data->vi.fpsNum = 24;
      data->vi.fpsDen = 1;
      data->vi.numFrames = 24 * 60 * 60;
      data->vi.format = vsapi->getFormatPreset(pfRGB24, core);

      vsapi->createFilter(in, out, FILTER_NAME, filterInit, filterGetFrame, filterFree, fmParallel, 0, data, core);
      return;
    }

    free(data);
  }
}

//////////////////////////////////////////
// Init

VS_EXTERNAL_API(void) VapourSynthPluginInit(VSConfigPlugin configFunc, VSRegisterFunction registerFunc, VSPlugin *plugin) {
  configFunc("net.bigjungle.qr", "qr", "QR code filter", VAPOURSYNTH_API_VERSION, 1, plugin);
  registerFunc(FILTER_NAME, "message:data;version:int:opt;error_correction:data:opt;", filterCreate, 0, plugin);
}

#else

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

class QRCodeSource 
  : public IClip 
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

    write_qr_code(p, pQRC, frame->GetPitch(), frame->GetHeight());

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

  QRcode *pQRC = NULL;
  if(create_qr_instance(string, version, error_correction, &pQRC))
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

#endif
