#include "PresenceCPU.h"
#include <algorithm>
#include <cmath>
#include <vector>

namespace presence {

static inline float clampf(float x, float a, float b) { return std::max(a, std::min(b, x)); }
float clamp01(float x) { return clampf(x, 0.0f, 1.0f); }
float luma(float r, float g, float b) { return 0.2126f * r + 0.7152f * g + 0.0722f * b; }

static inline float smoothstep(float e0, float e1, float x) {
  if (e1 <= e0) return x >= e1 ? 1.0f : 0.0f;
  x = clampf((x - e0) / (e1 - e0), 0.0f, 1.0f);
  return x * x * (3.0f - 2.0f * x);
}

static inline int clampi(int x, int a, int b) { return std::max(a, std::min(b, x)); }

static inline float sampleY(const std::vector<float>& Y, int w, int h, int x, int y) {
  x = clampi(x, 0, w - 1);
  y = clampi(y, 0, h - 1);
  return Y[y * w + x];
}

// CPU parity for the Metal 17-tap multi-ring blur.
static inline float blur17(const std::vector<float>& Y, int w, int h, int x, int y, int radius) {
  int r = std::max(1, radius);
  int half = std::max(1, r / 2);

  float acc = 4.0f * sampleY(Y, w, h, x, y);

  acc += 2.0f * sampleY(Y, w, h, x-half, y);
  acc += 2.0f * sampleY(Y, w, h, x+half, y);
  acc += 2.0f * sampleY(Y, w, h, x, y-half);
  acc += 2.0f * sampleY(Y, w, h, x, y+half);

  acc += sampleY(Y, w, h, x-half, y-half);
  acc += sampleY(Y, w, h, x+half, y-half);
  acc += sampleY(Y, w, h, x-half, y+half);
  acc += sampleY(Y, w, h, x+half, y+half);

  acc += sampleY(Y, w, h, x-r, y);
  acc += sampleY(Y, w, h, x+r, y);
  acc += sampleY(Y, w, h, x, y-r);
  acc += sampleY(Y, w, h, x, y+r);

  acc += 0.5f * sampleY(Y, w, h, x-r, y-r);
  acc += 0.5f * sampleY(Y, w, h, x+r, y-r);
  acc += 0.5f * sampleY(Y, w, h, x-r, y+r);
  acc += 0.5f * sampleY(Y, w, h, x+r, y+r);

  return acc / 22.0f;
}

static inline float softDeadzone(float x, float threshold) {
  float a = std::fabs(x);
  if (a <= threshold) return 0.0f;
  float d = a - threshold;
  float knee = smoothstep(0.0f, threshold * 2.0f + 1.0e-6f, d);
  return (x < 0.0f ? -1.0f : 1.0f) * d * knee;
}

static inline float softLimit(float x, float limit) {
  float L = std::max(limit, 1.0e-6f);
  return x / (1.0f + std::fabs(x) / L);
}

static inline float likelySkinMask(float r, float g, float b, float Y) {
  float maxc = std::max(r, std::max(g, b));
  float minc = std::min(r, std::min(g, b));
  float c = maxc - minc;
  if (c <= 1e-6f || Y <= 1e-6f) return 0.0f;
  float nr = r / (r + g + b + 1e-6f);
  float ng = g / (r + g + b + 1e-6f);
  float rg = smoothstep(0.32f, 0.48f, nr) * (1.0f - smoothstep(0.50f, 0.62f, nr));
  float gg = smoothstep(0.24f, 0.38f, ng) * (1.0f - smoothstep(0.42f, 0.52f, ng));
  float lum = smoothstep(0.05f, 0.25f, Y) * (1.0f - smoothstep(0.92f, 1.25f, Y));
  float chroma = smoothstep(0.025f, 0.22f, c);
  return clampf(rg * gg * lum * chroma, 0.0f, 1.0f);
}

void processRGBA(const float* src, float* dst, int width, int height, int srcStrideFloats, int dstStrideFloats, const Params& pIn) {
  if (!src || !dst || width <= 0 || height <= 0) return;

  Params p = pIn;
  p.amount = clampf(p.amount, 0.0f, 2.0f);
  p.depth = clampf(p.depth, -1.0f, 1.0f);
  p.micro = clampf(p.micro, -1.0f, 1.0f);
  p.atmosphere = clampf(p.atmosphere, -1.0f, 1.0f);
  p.edgeSoft = clampf(p.edgeSoft, 0.0f, 1.0f);
  p.hiPresence = clampf(p.hiPresence, 0.0f, 1.0f);
  p.shPresence = clampf(p.shPresence, 0.0f, 1.0f);
  p.texture = clampf(p.texture, 0.0f, 1.0f);
  p.bloom = clampf(p.bloom, 0.0f, 1.0f);
  p.skinGuard = clampf(p.skinGuard, 0.0f, 1.0f);

  const int n = width * height;
  std::vector<float> Y(n);
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      const float* px = src + y * srcStrideFloats + x * 4;
      Y[y * width + x] = luma(px[0], px[1], px[2]);
    }
  }

  int rSmall = std::max(1, width / 420);
  int rLarge = std::max(4, width / 85);
  int rBloom = std::max(6, width / 55);

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      int i = y * width + x;
      const float* s = src + y * srcStrideFloats + x * 4;
      float* d = dst + y * dstStrideFloats + x * 4;

      float r = s[0], g = s[1], b = s[2], a = s[3];
      float yy = Y[i];

      float small = blur17(Y, width, height, x, y, rSmall);
      float large = blur17(Y, width, height, x, y, rLarge);
      float big = blur17(Y, width, height, x, y, rBloom);

      float broadRaw = yy - large;
      float midRaw = small - large;
      float fineRaw = yy - small;

      float noiseFloor = 0.0015f + 0.0020f * clampf(std::fabs(yy), 0.0f, 1.5f);
      float broad = softDeadzone(broadRaw, noiseFloor * 1.25f);
      float fine = softDeadzone(fineRaw, noiseFloor);
      float mid = softDeadzone(midRaw, noiseFloor * 0.65f);

      float edgeEnergy = std::fabs(fine) + 0.45f * std::fabs(mid);
      float edgeMask = smoothstep(0.004f, 0.045f, edgeEnergy);

      float skin = likelySkinMask(r, g, b, yy);
      float protect = 1.0f - p.skinGuard * skin;

      float hi = smoothstep(0.55f, 1.15f, yy);
      float sh = 1.0f - smoothstep(0.05f, 0.35f, yy);

      float broadGuard = 1.0f - 0.72f * edgeMask;
      float detailGuard = 1.0f - p.edgeSoft * (0.55f + 0.35f * edgeMask);
      float textureGate = smoothstep(noiseFloor * 1.6f, noiseFloor * 6.0f + 1.0e-6f, std::fabs(fineRaw));

      float presence = 0.0f;
      presence += p.depth * broad * broadGuard * 1.05f;
      presence += p.micro * fine * detailGuard * 0.68f;
      presence += p.hiPresence * hi * mid * 0.62f;
      presence += p.shPresence * sh * mid * 0.66f;
      presence += p.texture * fine * textureGate * detailGuard * (1.0f - skin * 0.70f) * 0.24f;
      presence += p.atmosphere * broad * broadGuard * 0.45f;

      float bloomEnergy = std::max(0.0f, big - 0.72f);
      float bloom = p.bloom * bloomEnergy * 0.10f;

      float delta = (presence * protect + bloom) * p.amount;
      float maxDelta = 0.045f + 0.11f * clampf(std::fabs(yy), 0.0f, 1.5f);
      delta = softLimit(delta, maxDelta);

      // Equal encoded RGB offset: preserves R-G, G-B and R-B differences exactly.
      float rr = r + delta;
      float gg = g + delta;
      float bb = b + delta;

      if (p.view == 1) {
        float m = clamp01(0.5f + presence * 6.0f);
        d[0] = d[1] = d[2] = m;
      } else if (p.view == 2) {
        float m = clamp01(edgeMask);
        d[0] = d[1] = d[2] = m;
      } else if (p.view == 3) {
        float m = clamp01(std::fabs(delta) * 8.0f);
        d[0] = d[1] = d[2] = m;
      } else {
        d[0] = rr;
        d[1] = gg;
        d[2] = bb;
      }
      d[3] = a;
    }
  }
}

} // namespace presence
