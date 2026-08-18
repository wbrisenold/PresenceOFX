#include "PresenceCPU.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>
#include <random>
#include <vector>

using namespace presence;

static bool finiteImage(const std::vector<float>& v) {
  for (float x : v) if (!std::isfinite(x)) return false;
  return true;
}

static float channelStd(const std::vector<float>& img, int w, int h, int channel) {
  const int stride = w * 4;
  double mean = 0.0;
  int n = w * h;
  for (int y = 0; y < h; ++y) for (int x = 0; x < w; ++x) mean += img[y*stride+x*4+channel];
  mean /= n;
  double var = 0.0;
  for (int y = 0; y < h; ++y) for (int x = 0; x < w; ++x) {
    double d = img[y*stride+x*4+channel] - mean;
    var += d*d;
  }
  return static_cast<float>(std::sqrt(var / n));
}

int main() {
  const int w = 96, h = 64, stride = w * 4;
  std::vector<float> src(stride * h), dst(stride * h), bypass(stride * h);

  // Random finite/extended-range stress.
  std::mt19937 rng(1234);
  std::uniform_real_distribution<float> dist(-0.25f, 2.5f);
  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) {
      int i = y * stride + x * 4;
      src[i+0] = dist(rng);
      src[i+1] = dist(rng);
      src[i+2] = dist(rng);
      src[i+3] = 1.0f;
    }
  }

  Params p;
  p.amount = 0.0f;
  processRGBA(src.data(), bypass.data(), w, h, stride, stride, p);
  for (size_t i = 0; i < src.size(); ++i) assert(std::fabs(src[i] - bypass[i]) < 1e-7f);

  p.amount = 1.0f;
  p.depth = 0.5f;
  p.micro = 0.35f;
  p.atmosphere = -0.4f;
  p.edgeSoft = 0.2f;
  p.hiPresence = 0.4f;
  p.shPresence = 0.4f;
  p.texture = 0.25f;
  p.bloom = 0.1f;
  p.view = 0;
  processRGBA(src.data(), dst.data(), w, h, stride, stride, p);
  assert(finiteImage(dst));

  // Normal mode must not amplify encoded chroma differences: every pixel receives
  // one shared RGB offset. This directly guards against the colour-noise breakup
  // seen in smooth sky/highlight surfaces.
  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) {
      int i = y * stride + x * 4;
      float inRG = src[i+0] - src[i+1];
      float inGB = src[i+1] - src[i+2];
      float outRG = dst[i+0] - dst[i+1];
      float outGB = dst[i+1] - dst[i+2];
      assert(std::fabs(inRG - outRG) < 2e-6f);
      assert(std::fabs(inGB - outGB) < 2e-6f);
    }
  }

  // Smooth "sky" with an 8x8 low-level codec/block pattern. At aggressive settings,
  // Presence must not substantially amplify the block variation.
  std::vector<float> sky(stride * h), skyOut(stride * h);
  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) {
      int i = y * stride + x * 4;
      float block = (((x / 8) + (y / 8)) & 1) ? 0.0015f : -0.0015f;
      sky[i+0] = 0.34f + block;
      sky[i+1] = 0.48f + block;
      sky[i+2] = 0.53f + block;
      sky[i+3] = 1.0f;
    }
  }
  Params aggressive;
  aggressive.amount = 2.0f;
  aggressive.depth = 0.75f;
  aggressive.micro = 0.70f;
  aggressive.atmosphere = -0.5f;
  aggressive.texture = 0.70f;
  aggressive.hiPresence = 0.5f;
  aggressive.shPresence = 0.5f;
  aggressive.bloom = 0.0f;
  aggressive.edgeSoft = 0.0f;
  aggressive.skinGuard = 0.0f;
  processRGBA(sky.data(), skyOut.data(), w, h, stride, stride, aggressive);
  float beforeStd = channelStd(sky, w, h, 1);
  float afterStd = channelStd(skyOut, w, h, 1);
  assert(afterStd <= beforeStd * 1.15f + 1e-7f);

  // A perfectly flat field with bloom disabled must remain perfectly flat.
  std::vector<float> flat(stride*h), flatOut(stride*h);
  for (int y=0;y<h;++y) for (int x=0;x<w;++x) {
    int i=y*stride+x*4;
    flat[i+0]=0.42f; flat[i+1]=0.47f; flat[i+2]=0.51f; flat[i+3]=1.0f;
  }
  Params flatP;
  flatP.amount=2.0f;
  flatP.depth=1.0f;
  flatP.micro=1.0f;
  flatP.atmosphere=-1.0f;
  flatP.texture=1.0f;
  flatP.hiPresence=1.0f;
  flatP.shPresence=1.0f;
  flatP.bloom=0.0f;
  flatP.skinGuard=0.0f;
  processRGBA(flat.data(),flatOut.data(),w,h,stride,stride,flatP);
  for (size_t i=0;i<flat.size();++i) assert(std::fabs(flat[i]-flatOut[i])<2e-6f);

  // Diagnostic views stay grayscale, finite, and in 0..1.
  for (int view = 1; view <= 3; ++view) {
    aggressive.view = view;
    processRGBA(src.data(), dst.data(), w, h, stride, stride, aggressive);
    assert(finiteImage(dst));
    for (int y = 0; y < h; ++y) {
      for (int x = 0; x < w; ++x) {
        int i = y * stride + x * 4;
        assert(dst[i] >= -1e-6f && dst[i] <= 1.0f + 1e-6f);
        assert(std::fabs(dst[i] - dst[i+1]) < 1e-6f);
        assert(std::fabs(dst[i+1] - dst[i+2]) < 1e-6f);
      }
    }
  }

  std::cout << "Presence v1.1 artifact-safety tests passed\n";
  std::cout << "codec-block std ratio: " << (afterStd / beforeStd) << "\n";
  return 0;
}
