#pragma once
namespace presence {
struct Params {
  float amount=1.0f, depth=0.35f, micro=0.20f, atmosphere=-0.10f, edgeSoft=0.15f;
  float hiPresence=0.20f, shPresence=0.20f, texture=0.12f, bloom=0.05f, skinGuard=0.70f;
  int view=0;
};
void processRGBA(const float* src,float* dst,int width,int height,int srcStrideFloats,int dstStrideFloats,const Params& p);
}
