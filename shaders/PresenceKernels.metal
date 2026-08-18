#include <metal_stdlib>
using namespace metal;

struct Params {
  float amount, depth, micro, atmosphere, edgeSoft, hiPresence, shPresence, texture, bloom, skinGuard;
  int view, width, height, srcStrideFloats, dstStrideFloats;
};

static inline float luma3(float3 c) { return dot(c, float3(0.2126f, 0.7152f, 0.0722f)); }
static inline float satf(float x) { return clamp(x, 0.0f, 1.0f); }
static inline int clampi(int x, int a, int b) { return max(a, min(b, x)); }
static inline float sgnf(float x) { return x < 0.0f ? -1.0f : 1.0f; }

static inline float3 sampleRGB(const device float* src, constant Params& p, int x, int y) {
  x = clampi(x, 0, p.width - 1);
  y = clampi(y, 0, p.height - 1);
  const device float* px = src + y * p.srcStrideFloats + x * 4;
  return float3(px[0], px[1], px[2]);
}

static inline float guideWeight(float3 center, float3 sample, float sigma) {
  float3 d = sample - center;
  float d2 = dot(d, d);
  float s2 = max(sigma * sigma, 1.0e-8f);
  // Smooth rational bilateral weight. Avoids exp() and remains well-behaved
  // for extended/log-domain values.
  return 1.0f / (1.0f + d2 / s2);
}

static inline void accumGuided(
  const device float* src, constant Params& p,
  int x, int y, int dx, int dy,
  float spatialW, float sigma, float3 center,
  thread float& sumY, thread float& sumW)
{
  float3 c = sampleRGB(src, p, x + dx, y + dy);
  float w = spatialW * guideWeight(center, c, sigma);
  sumY += w * luma3(c);
  sumW += w;
}

// 17-tap isotropic two-ring color-guided blur.
// Unlike the prior sparse cross/square sampler, the outer ring is rotated by 22.5 degrees.
// More importantly, samples unlike the center color are rejected, preventing a dark palm,
// subject silhouette, or building from bleeding broad contrast into adjacent blue sky.
static inline float guidedBlur17(
  const device float* src, constant Params& p,
  int x, int y, int radius, float sigma)
{
  int r = max(1, radius);
  int h = max(1, r / 2);
  int h7 = max(1, int(float(h) * 0.70710678f + 0.5f));
  int r92 = max(1, int(float(r) * 0.92387953f + 0.5f));
  int r38 = max(1, int(float(r) * 0.38268343f + 0.5f));

  float3 center = sampleRGB(src, p, x, y);
  float sumY = 4.0f * luma3(center);
  float sumW = 4.0f;

  // Inner ring, 8 samples.
  accumGuided(src,p,x,y, h, 0,1.50f,sigma,center,sumY,sumW);
  accumGuided(src,p,x,y,-h, 0,1.50f,sigma,center,sumY,sumW);
  accumGuided(src,p,x,y, 0, h,1.50f,sigma,center,sumY,sumW);
  accumGuided(src,p,x,y, 0,-h,1.50f,sigma,center,sumY,sumW);
  accumGuided(src,p,x,y, h7, h7,1.15f,sigma,center,sumY,sumW);
  accumGuided(src,p,x,y,-h7, h7,1.15f,sigma,center,sumY,sumW);
  accumGuided(src,p,x,y, h7,-h7,1.15f,sigma,center,sumY,sumW);
  accumGuided(src,p,x,y,-h7,-h7,1.15f,sigma,center,sumY,sumW);

  // Outer ring, 8 samples rotated 22.5 degrees.
  accumGuided(src,p,x,y, r92, r38,0.80f,sigma,center,sumY,sumW);
  accumGuided(src,p,x,y, r38, r92,0.80f,sigma,center,sumY,sumW);
  accumGuided(src,p,x,y,-r38, r92,0.80f,sigma,center,sumY,sumW);
  accumGuided(src,p,x,y,-r92, r38,0.80f,sigma,center,sumY,sumW);
  accumGuided(src,p,x,y,-r92,-r38,0.80f,sigma,center,sumY,sumW);
  accumGuided(src,p,x,y,-r38,-r92,0.80f,sigma,center,sumY,sumW);
  accumGuided(src,p,x,y, r38,-r92,0.80f,sigma,center,sumY,sumW);
  accumGuided(src,p,x,y, r92,-r38,0.80f,sigma,center,sumY,sumW);

  return sumY / max(sumW, 1.0e-8f);
}

static inline float softDeadzone(float x, float threshold) {
  float a = abs(x);
  if (a <= threshold) return 0.0f;
  float d = a - threshold;
  float knee = smoothstep(0.0f, threshold * 2.0f + 1.0e-6f, d);
  return sgnf(x) * d * knee;
}

static inline float softLimit(float x, float limit) {
  float L = max(limit, 1.0e-6f);
  return x / (1.0f + abs(x) / L);
}

static inline float likelySkin(float3 c, float y) {
  float s = c.x + c.y + c.z + 1e-6f;
  float nr = c.x / s;
  float ng = c.y / s;
  float maxc = max(c.x, max(c.y, c.z));
  float minc = min(c.x, min(c.y, c.z));
  float chroma = smoothstep(0.025f, 0.22f, maxc - minc);
  float rg = smoothstep(0.32f, 0.48f, nr) * (1.0f - smoothstep(0.50f, 0.62f, nr));
  float gg = smoothstep(0.24f, 0.38f, ng) * (1.0f - smoothstep(0.42f, 0.52f, ng));
  float lum = smoothstep(0.05f, 0.25f, y) * (1.0f - smoothstep(0.92f, 1.25f, y));
  return satf(rg * gg * lum * chroma);
}

kernel void presenceKernel(
  const device float* src [[buffer(0)]],
  device float* dst [[buffer(1)]],
  constant Params& p [[buffer(2)]],
  uint2 gid [[thread_position_in_grid]])
{
  if (gid.x >= (uint)p.width || gid.y >= (uint)p.height) return;

  int x = int(gid.x);
  int y = int(gid.y);
  int srcIdx = y * p.srcStrideFloats + x * 4;
  int dstIdx = y * p.dstStrideFloats + x * 4;

  float4 in = float4(src[srcIdx+0], src[srcIdx+1], src[srcIdx+2], src[srcIdx+3]);
  float3 rgb = in.rgb;
  float Y = luma3(rgb);

  int rS = max(1, p.width / 420);
  int rL = max(4, p.width / 85);
  int rB = max(6, p.width / 55);

  // Guide thresholds are intentionally scale-dependent:
  // small scale may cross subtle texture; broad scale must not cross object/color boundaries.
  float small = guidedBlur17(src,p,x,y,rS,0.095f);
  float large = guidedBlur17(src,p,x,y,rL,0.055f);
  float big   = guidedBlur17(src,p,x,y,rB,0.085f);

  float broadRaw = Y - large;
  float midRaw = small - large;
  float fineRaw = Y - small;

  float noiseFloor = 0.0018f + 0.0022f * clamp(abs(Y), 0.0f, 1.5f);
  float broad = softDeadzone(broadRaw, noiseFloor * 1.35f);
  float mid = softDeadzone(midRaw, noiseFloor * 0.85f);
  float fine = softDeadzone(fineRaw, noiseFloor * 1.10f);

  float edgeEnergy = abs(fine) + 0.45f * abs(mid);
  float edgeMask = smoothstep(0.005f, 0.050f, edgeEnergy);

  // Coherence gate: Micro/Texture only wake up on structure clearly above the codec/noise floor.
  float detailGate = smoothstep(noiseFloor * 2.5f, noiseFloor * 9.0f + 1.0e-6f, abs(fineRaw));

  float skin = likelySkin(rgb, Y);
  float protect = 1.0f - clamp(p.skinGuard, 0.0f, 1.0f) * skin;

  float hi = smoothstep(0.55f, 1.15f, Y);
  float sh = 1.0f - smoothstep(0.05f, 0.35f, Y);

  float edgeSoft = clamp(p.edgeSoft, 0.0f, 1.0f);
  float broadGuard = 1.0f - 0.80f * edgeMask;
  float hfGuard = (1.0f - edgeSoft * 0.75f) * detailGate;

  float presence = 0.0f;
  presence += clamp(p.depth, -1.0f, 1.0f) * broad * broadGuard * 0.95f;
  presence += clamp(p.micro, -1.0f, 1.0f) * fine * hfGuard * 0.58f;
  presence += clamp(p.hiPresence, 0.0f, 1.0f) * hi * mid * detailGate * 0.52f;
  presence += clamp(p.shPresence, 0.0f, 1.0f) * sh * mid * detailGate * 0.56f;
  presence += clamp(p.texture, 0.0f, 1.0f) * fine * detailGate * (1.0f - skin * 0.75f) * 0.18f;

  // Atmosphere shares the edge-aware broad signal. No separate cross-edge blur.
  presence += clamp(p.atmosphere, -1.0f, 1.0f) * broad * broadGuard * 0.35f;

  // Bloom is also guided; unrelated dark/colored structures do not contaminate a bright surface.
  float bloomEnergy = max(0.0f, big - max(Y, 0.72f));
  float bloom = clamp(p.bloom, 0.0f, 1.0f) * bloomEnergy * 0.07f;

  float amount = clamp(p.amount, 0.0f, 2.0f);
  float delta = (presence * protect + bloom) * amount;

  // Stronger automatic limiter than v1.1; local spatial tools should never create
  // a huge code-value step merely because Amount is pushed.
  float maxDelta = 0.032f + 0.085f * clamp(abs(Y), 0.0f, 1.5f);
  delta = softLimit(delta, maxDelta);

  // Shared offset remains critical: Presence does not multiply RGB differences.
  float3 out = rgb + float3(delta);

  if (p.view == 1) out = float3(satf(0.5f + presence * 7.0f));
  else if (p.view == 2) out = float3(satf(edgeMask));
  else if (p.view == 3) out = float3(satf(abs(delta) * 10.0f));

  dst[dstIdx+0] = out.x;
  dst[dstIdx+1] = out.y;
  dst[dstIdx+2] = out.z;
  dst[dstIdx+3] = in.a;
}
