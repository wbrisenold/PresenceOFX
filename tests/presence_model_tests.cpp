#include "PresenceCPU.h"
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

int main() {
  const int w = 64, h = 48, stride = w * 4;
  std::vector<float> src(stride * h), dst(stride * h), bypass(stride * h);
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
  for (size_t i = 0; i < src.size(); ++i) assert(std::fabs(src[i] - bypass[i]) < 1e-6f);

  p.amount = 1.0f;
  p.depth = 0.5f;
  p.micro = 0.35f;
  p.atmosphere = -0.4f;
  p.edgeSoft = 0.2f;
  p.hiPresence = 0.4f;
  p.shPresence = 0.4f;
  p.texture = 0.25f;
  p.bloom = 0.1f;
  processRGBA(src.data(), dst.data(), w, h, stride, stride, p);
  assert(finiteImage(dst));

  for (int view = 1; view <= 3; ++view) {
    p.view = view;
    processRGBA(src.data(), dst.data(), w, h, stride, stride, p);
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

  std::cout << "Presence model tests passed\n";
  return 0;
}
