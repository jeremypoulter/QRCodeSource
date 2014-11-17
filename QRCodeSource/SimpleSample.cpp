#include <windows.h>
#include "avisynth.h"

class SimpleSample : public GenericVideoFilter {
   int SquareSize;
public:
   SimpleSample(PClip _child, int _SquareSize, IScriptEnvironment* env);
   ~SimpleSample();
   PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
};

SimpleSample::SimpleSample(PClip _child, int _SquareSize, IScriptEnvironment* env) :
   GenericVideoFilter(_child), SquareSize(_SquareSize) {
   if (vi.width<SquareSize || vi.height<SquareSize) {
      env->ThrowError("SimpleSample: square doesn't fit into the clip!");
   }
}

SimpleSample::~SimpleSample() {}

PVideoFrame __stdcall SimpleSample::GetFrame(int n, IScriptEnvironment* env) {

   PVideoFrame src = child->GetFrame(n, env);
   env->MakeWritable(&src);

   unsigned char* srcp = src->GetWritePtr();
   int src_pitch = src->GetPitch();
   int src_width = src->GetRowSize();
   int src_height = src->GetHeight();

   int w, h, woffset;

   if (vi.IsRGB24()) {
      srcp = srcp + (src_height/2 - SquareSize/2) * src_pitch;

      woffset = src_width/2 - 3*SquareSize/2;

      for (h=0; h<SquareSize; h++) {
         for (w=0; w<3*SquareSize; w+=3) {
            *(srcp + woffset + w) = 255;
            *(srcp + woffset + w + 1) = 255;
            *(srcp + woffset + w + 2) = 255;
         }															
         srcp += src_pitch;
      }
   }
   if (vi.IsRGB32()) {
      srcp = srcp + (src_height/2 - SquareSize/2) * src_pitch;

      woffset = src_width/2 - 4*SquareSize/2;

      for (h=0; h<SquareSize; h++) {
         for (w=0; w<4*SquareSize; w+=4) {
            *(srcp + woffset + w) = 255;
            *(srcp + woffset + w + 1) = 0;
            *(srcp + woffset + w + 2) = 255;
         }
         srcp += src_pitch;
      }
   }
/*
   if (vi.IsRGB32()) { // variant 1 - processing a pixel at once
      srcp = srcp + (src_height/2 - SquareSize/2) * src_pitch;

      woffset = src_width/8 - SquareSize/2;

      for (h=0; h<SquareSize; h++) {
         for (w=0; w<SquareSize; w++) {
            *((unsigned int *)srcp + woffset + w) = 0x00FFFFFF;
         }	
         srcp += src_pitch;
      }
   }
*/
/*
   if (vi.IsRGB32()) { // variant 2 - processing a pixel at once
      unsigned int* srcp = (unsigned int*)src->GetWritePtr();
	
      srcp = srcp + (src_height/2 - SquareSize/2) * src_pitch/4;

      woffset = src_width/8 - SquareSize/2;

      for (h=0; h<SquareSize; h++) {
         for (w=0; w<SquareSize; w++) {
            srcp[woffset + w] = 0x00FFFFFF;
         }	
         srcp += src_pitch/4;
      }
   }
*/
   if (vi.IsYUY2()) {
      srcp = srcp + (src_height/2 - SquareSize/2) * src_pitch;

      woffset = src_width/8 - SquareSize/4;

      for (h=0; h<SquareSize; h++) {
         for (w=0; w<SquareSize/2; w++) {
            *((unsigned int *)srcp + woffset + w) = 0x80EB80EB;
         }
         srcp += src_pitch; 
      }
   }
   if (vi.IsPlanar() && vi.IsYUV()) {

      int planes[] = {PLANAR_Y, PLANAR_U, PLANAR_V};
      int square_value[] = {235, 128, 128};
      int p;
      int width_sub, height_sub;

      for (p=0; p<3; p++) {
         srcp = src->GetWritePtr(planes[p]);		
         src_pitch = src->GetPitch(planes[p]);
         src_width = src->GetRowSize(planes[p]);
         src_height = src->GetHeight(planes[p]);
         width_sub = vi.GetPlaneWidthSubsampling(planes[p]);
         height_sub = vi.GetPlaneHeightSubsampling(planes[p]);

         srcp = srcp + (src_height/2 - (SquareSize>>height_sub)/2) * src_pitch;

         woffset = src_width/2 - (SquareSize>>width_sub)/2;

         for (h=0; h<(SquareSize>>height_sub); h++) {
            for (w=0; w<(SquareSize>>width_sub); w++) {
               srcp[woffset + w] = square_value[p];
            }
            srcp += src_pitch;
         }
      }
   }

   return src;
}

AVSValue __cdecl Create_SimpleSample(AVSValue args, void* user_data, IScriptEnvironment* env) {
   return new SimpleSample(args[0].AsClip(),   
                           args[1].AsInt(100),
                           env); 
}

const AVS_Linkage *AVS_linkage = 0;

extern "C" __declspec(dllexport) const char* __stdcall AvisynthPluginInit3(IScriptEnvironment* env, const AVS_Linkage* const vectors) {
   AVS_linkage = vectors;
   env->AddFunction("SimpleSample", "c[size]i", Create_SimpleSample, 0);
   return "SimpleSample plugin";
}
