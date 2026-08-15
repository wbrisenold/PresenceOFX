#pragma once
#include <cstdint>

namespace presence {

struct Params {
  float amount = 1.0f;
  float depth = 0.35f;
  float micro = 0.20f;
  float atmosphere = -0.10f;
  float edgeSoft = 0.15f;
  float hiPresence = 0.20f;
  float shPresence = 0.20f;
  float texture = 0.12f;
  float bloom = 0.05f;
  float skinGuard = 0.70f;
  int view = 0;
};

void processRGBA(const float* src, float* dst, int width, int height, int srcStrideFloats, int dstStrideFloats, const Params& p);
float clamp01(float x);
float luma(float r, float g, float b);

} // namespace presence
