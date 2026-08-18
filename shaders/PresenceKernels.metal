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

static inline float sampleY(const device float* src, constant Params& p, int x, int y) {
  x = clampi(x, 0, p.width - 1);
  y = clampi(y, 0, p.height - 1);
  const device float* px = src + y * p.srcStrideFloats + x * 4;
  return luma3(float3(px[0], px[1], px[2]));
}

// Isotropic 17-tap multi-ring blur approximation.
// This replaces the old 9-tap "cross" sampler, which sampled only the centre
// and eight widely-separated points. At large radii that was not a blur and
// could alias codec blocks / fine texture into structured halos.
static inline float blur17(const device float* src, constant Params& p, int x, int y, int radius) {
  int r = max(1, radius);
  int h = max(1, r / 2);

  float acc = 4.0f * sampleY(src, p, x, y);

  // Inner cardinals: weight 2 each.
  acc += 2.0f * sampleY(src, p, x-h, y);
  acc += 2.0f * sampleY(src, p, x+h, y);
  acc += 2.0f * sampleY(src, p, x, y-h);
  acc += 2.0f * sampleY(src, p, x, y+h);

  // Inner diagonals: weight 1 each.
  acc += sampleY(src, p, x-h, y-h);
  acc += sampleY(src, p, x+h, y-h);
  acc += sampleY(src, p, x-h, y+h);
  acc += sampleY(src, p, x+h, y+h);

  // Outer cardinals: weight 1 each.
  acc += sampleY(src, p, x-r, y);
  acc += sampleY(src, p, x+r, y);
  acc += sampleY(src, p, x, y-r);
  acc += sampleY(src, p, x, y+r);

  // Outer diagonals: weight 0.5 each.
  acc += 0.5f * sampleY(src, p, x-r, y-r);
  acc += 0.5f * sampleY(src, p, x+r, y-r);
  acc += 0.5f * sampleY(src, p, x-r, y+r);
  acc += 0.5f * sampleY(src, p, x+r, y+r);

  return acc / 22.0f;
}

static inline float softDeadzone(float x, float threshold) {
  float a = abs(x);
  if (a <= threshold) return 0.0f;
  float d = a - threshold;
  // Smoothly re-enter rather than a hard threshold.
  float knee = smoothstep(0.0f, threshold * 2.0f + 1.0e-6f, d);
  return sgnf(x) * d * knee;
}

static inline float softLimit(float x, float limit) {
  float L = max(limit, 1.0e-6f);
  // Rational soft clip. Monotonic and finite, no transcendental dependency.
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

  float small = blur17(src, p, x, y, rS);
  float large = blur17(src, p, x, y, rL);
  float big = blur17(src, p, x, y, rB);

  float broadRaw = Y - large;
  float midRaw = small - large;
  float fineRaw = Y - small;

  // Automatic codec/noise floor. Tiny fluctuations in smooth surfaces should not
  // become "depth" or "texture" merely because spatial controls are enabled.
  float noiseFloor = 0.0015f + 0.0020f * clamp(abs(Y), 0.0f, 1.5f);
  float broad = softDeadzone(broadRaw, noiseFloor * 1.25f);
  float fine = softDeadzone(fineRaw, noiseFloor);
  float mid = softDeadzone(midRaw, noiseFloor * 0.65f);

  float edgeEnergy = abs(fine) + 0.45f * abs(mid);
  float edgeMask = smoothstep(0.004f, 0.045f, edgeEnergy);

  float skin = likelySkin(rgb, Y);
  float protect = 1.0f - clamp(p.skinGuard, 0.0f, 1.0f) * skin;

  float hi = smoothstep(0.55f, 1.15f, Y);
  float sh = 1.0f - smoothstep(0.05f, 0.35f, Y);

  float edgeSoft = clamp(p.edgeSoft, 0.0f, 1.0f);

  // Broad contrast/dehaze is automatically reduced near strong edges so large-radius
  // structure does not create bright/dark halos around silhouettes.
  float broadGuard = 1.0f - 0.72f * edgeMask;

  // Fine detail is attenuated, not blurred/desaturated. The old implementation mixed
  // RGB toward a grayscale luma sample for Edge Soft, which could create colour fringes.
  float detailGuard = 1.0f - edgeSoft * (0.55f + 0.35f * edgeMask);

  // Texture should not "discover" compression in flat areas.
  float textureGate = smoothstep(noiseFloor * 1.6f, noiseFloor * 6.0f + 1.0e-6f, abs(fineRaw));

  float presence = 0.0f;
  presence += clamp(p.depth, -1.0f, 1.0f) * broad * broadGuard * 1.05f;
  presence += clamp(p.micro, -1.0f, 1.0f) * fine * detailGuard * 0.68f;
  presence += clamp(p.hiPresence, 0.0f, 1.0f) * hi * mid * 0.62f;
  presence += clamp(p.shPresence, 0.0f, 1.0f) * sh * mid * 0.66f;
  presence += clamp(p.texture, 0.0f, 1.0f) * fine * textureGate * detailGuard * (1.0f - skin * 0.70f) * 0.24f;

  // Atmosphere/dehaze shares the broad structure and halo guard.
  presence += clamp(p.atmosphere, -1.0f, 1.0f) * broad * broadGuard * 0.45f;

  // Subtle highlight integration. Use blurred highlight energy; keep it deliberately low.
  float bloomEnergy = max(0.0f, big - 0.72f);
  float bloom = clamp(p.bloom, 0.0f, 1.0f) * bloomEnergy * 0.10f;

  // Apply Amount before the limiter so Amount > 1 cannot bypass safety.
  float amount = clamp(p.amount, 0.0f, 2.0f);
  float delta = (presence * protect + bloom) * amount;

  // Automatic perceptual delta limiter. This protects both shadows and bright smooth areas.
  float maxDelta = 0.045f + 0.11f * clamp(abs(Y), 0.0f, 1.5f);
  delta = softLimit(delta, maxDelta);

  // IMPORTANT: use an equal RGB offset in the incoming log/perceptual encoding.
  // The old newY/Y multiplication amplified R-G/B differences, including chroma noise
  // and H.264 colour errors. Equal offsets preserve encoded channel differences exactly.
  float3 out = rgb + float3(delta);

  if (p.view == 1) {
    out = float3(satf(0.5f + presence * 6.0f));
  } else if (p.view == 2) {
    out = float3(satf(edgeMask));
  } else if (p.view == 3) {
    out = float3(satf(abs(delta) * 8.0f));
  }

  dst[dstIdx+0] = out.x;
  dst[dstIdx+1] = out.y;
  dst[dstIdx+2] = out.z;
  dst[dstIdx+3] = in.a;
}
