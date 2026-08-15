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

static void boxBlurLuma(const std::vector<float>& in, std::vector<float>& out, int w, int h, int radius) {
  if (radius <= 0) { out = in; return; }
  std::vector<float> tmp(w * h, 0.0f);
  const int win = radius * 2 + 1;
  for (int y = 0; y < h; ++y) {
    float acc = 0.0f;
    for (int x = -radius; x <= radius; ++x) acc += in[y * w + clampi(x, 0, w - 1)];
    for (int x = 0; x < w; ++x) {
      tmp[y * w + x] = acc / static_cast<float>(win);
      int x0 = clampi(x - radius, 0, w - 1);
      int x1 = clampi(x + radius + 1, 0, w - 1);
      acc += in[y * w + x1] - in[y * w + x0];
    }
  }
  for (int x = 0; x < w; ++x) {
    float acc = 0.0f;
    for (int y = -radius; y <= radius; ++y) acc += tmp[clampi(y, 0, h - 1) * w + x];
    for (int y = 0; y < h; ++y) {
      out[y * w + x] = acc / static_cast<float>(win);
      int y0 = clampi(y - radius, 0, h - 1);
      int y1 = clampi(y + radius + 1, 0, h - 1);
      acc += tmp[y1 * w + x] - tmp[y0 * w + x];
    }
  }
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
  std::vector<float> Y(n), blurSmall(n), blurLarge(n), blurBloom(n);
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      const float* px = src + y * srcStrideFloats + x * 4;
      Y[y * width + x] = luma(px[0], px[1], px[2]);
    }
  }

  int rSmall = std::max(1, width / 420);
  int rLarge = std::max(4, width / 85);
  int rBloom = std::max(6, width / 55);
  boxBlurLuma(Y, blurSmall, width, height, rSmall);
  boxBlurLuma(Y, blurLarge, width, height, rLarge);
  boxBlurLuma(Y, blurBloom, width, height, rBloom);

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      int i = y * width + x;
      const float* s = src + y * srcStrideFloats + x * 4;
      float* d = dst + y * dstStrideFloats + x * 4;
      float r = s[0], g = s[1], b = s[2], a = s[3];
      float yy = Y[i];

      float broad = yy - blurLarge[i];
      float mid = blurSmall[i] - blurLarge[i];
      float fine = yy - blurSmall[i];
      float edge = std::fabs(fine) + 0.45f * std::fabs(mid);
      float edgeMask = smoothstep(0.006f, 0.055f, edge);
      float skin = likelySkinMask(r, g, b, yy);
      float protect = 1.0f - p.skinGuard * skin;

      float hi = smoothstep(0.55f, 1.15f, yy);
      float sh = 1.0f - smoothstep(0.05f, 0.35f, yy);
      float presence = 0.0f;
      presence += p.depth * broad * 1.20f;
      presence += p.micro * fine * (0.80f - 0.45f * p.edgeSoft * edgeMask);
      presence += p.hiPresence * hi * mid * 0.75f;
      presence += p.shPresence * sh * mid * 0.85f;
      presence += p.texture * fine * (1.0f - skin * 0.65f) * 0.35f;

      float haze = blurLarge[i] - yy;
      presence += (-p.atmosphere) * haze * 0.55f;

      float bloomSrc = std::max(0.0f, yy - 0.70f);
      float bloom = p.bloom * smoothstep(0.0f, 0.8f, blurBloom[i]) * bloomSrc * 0.35f;

      float newY = yy + presence * protect + bloom;
      float scale = (std::fabs(yy) > 1e-6f) ? (newY / yy) : 1.0f;
      scale = clampf(scale, 0.05f, 8.0f);
      float rr = r * scale;
      float gg = g * scale;
      float bb = b * scale;

      if (p.edgeSoft > 0.0f) {
        float soft = p.edgeSoft * edgeMask * 0.15f;
        rr = rr * (1.0f - soft) + blurSmall[i] * soft;
        gg = gg * (1.0f - soft) + blurSmall[i] * soft;
        bb = bb * (1.0f - soft) + blurSmall[i] * soft;
      }

      float amount = p.amount;
      if (p.view == 1) {
        float m = clamp01(0.5f + presence * 6.0f);
        d[0] = d[1] = d[2] = m; d[3] = a;
      } else if (p.view == 2) {
        float m = clamp01(edgeMask);
        d[0] = d[1] = d[2] = m; d[3] = a;
      } else if (p.view == 3) {
        float m = clamp01(std::fabs((rr + gg + bb) - (r + g + b)) * 0.8f);
        d[0] = d[1] = d[2] = m; d[3] = a;
      } else {
        d[0] = r + (rr - r) * amount;
        d[1] = g + (gg - g) * amount;
        d[2] = b + (bb - b) * amount;
        d[3] = a;
      }
    }
  }
}

} // namespace presence
