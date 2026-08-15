#include <metal_stdlib>
using namespace metal;

struct Params {
  float amount, depth, micro, atmosphere, edgeSoft, hiPresence, shPresence, texture, bloom, skinGuard;
  int view, width, height, srcStrideFloats, dstStrideFloats;
};

static inline float luma3(float3 c) { return dot(c, float3(0.2126, 0.7152, 0.0722)); }
static inline float satf(float x) { return clamp(x, 0.0f, 1.0f); }
static inline int clampi(int x, int a, int b) { return max(a, min(b, x)); }

static inline float sampleY(const device float* src, constant Params& p, int x, int y) {
  x = clampi(x, 0, p.width - 1);
  y = clampi(y, 0, p.height - 1);
  const device float* px = src + y * p.srcStrideFloats + x * 4;
  return luma3(float3(px[0], px[1], px[2]));
}

static inline float blurCross(const device float* src, constant Params& p, int x, int y, int r) {
  float acc = sampleY(src, p, x, y);
  acc += sampleY(src, p, x-r, y);
  acc += sampleY(src, p, x+r, y);
  acc += sampleY(src, p, x, y-r);
  acc += sampleY(src, p, x, y+r);
  acc += sampleY(src, p, x-r, y-r);
  acc += sampleY(src, p, x+r, y-r);
  acc += sampleY(src, p, x-r, y+r);
  acc += sampleY(src, p, x+r, y+r);
  return acc / 9.0f;
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

kernel void presenceKernel(const device float* src [[buffer(0)]], device float* dst [[buffer(1)]], constant Params& p [[buffer(2)]], uint2 gid [[thread_position_in_grid]]) {
  if (gid.x >= (uint)p.width || gid.y >= (uint)p.height) return;
  int x = int(gid.x), y = int(gid.y);
  int srcIdx = y * p.srcStrideFloats + x * 4;
  int dstIdx = y * p.dstStrideFloats + x * 4;
  float4 in = float4(src[srcIdx+0], src[srcIdx+1], src[srcIdx+2], src[srcIdx+3]);
  float3 rgb = in.rgb;
  float Y = luma3(rgb);
  int rS = max(1, p.width / 420);
  int rL = max(4, p.width / 85);
  int rB = max(6, p.width / 55);
  float small = blurCross(src, p, x, y, rS);
  float large = blurCross(src, p, x, y, rL);
  float big = blurCross(src, p, x, y, rB);
  float broad = Y - large;
  float mid = small - large;
  float fine = Y - small;
  float edge = abs(fine) + 0.45f * abs(mid);
  float edgeMask = smoothstep(0.006f, 0.055f, edge);
  float skin = likelySkin(rgb, Y);
  float protect = 1.0f - clamp(p.skinGuard,0.0f,1.0f) * skin;
  float hi = smoothstep(0.55f, 1.15f, Y);
  float sh = 1.0f - smoothstep(0.05f, 0.35f, Y);
  float presence = 0.0f;
  presence += clamp(p.depth,-1.0f,1.0f) * broad * 1.20f;
  presence += clamp(p.micro,-1.0f,1.0f) * fine * (0.80f - 0.45f * clamp(p.edgeSoft,0.0f,1.0f) * edgeMask);
  presence += clamp(p.hiPresence,0.0f,1.0f) * hi * mid * 0.75f;
  presence += clamp(p.shPresence,0.0f,1.0f) * sh * mid * 0.85f;
  presence += clamp(p.texture,0.0f,1.0f) * fine * (1.0f - skin * 0.65f) * 0.35f;
  presence += (-clamp(p.atmosphere,-1.0f,1.0f)) * (large - Y) * 0.55f;
  float bloomSrc = max(0.0f, Y - 0.70f);
  float bloom = clamp(p.bloom,0.0f,1.0f) * smoothstep(0.0f, 0.8f, big) * bloomSrc * 0.35f;
  float newY = Y + presence * protect + bloom;
  float scale = (abs(Y) > 1e-6f) ? newY / Y : 1.0f;
  scale = clamp(scale, 0.05f, 8.0f);
  float3 out = rgb * scale;
  float soft = clamp(p.edgeSoft,0.0f,1.0f) * edgeMask * 0.15f;
  out = mix(out, float3(small), soft);
  float amount = clamp(p.amount, 0.0f, 2.0f);
  if (p.view == 1) out = float3(satf(0.5f + presence * 6.0f));
  else if (p.view == 2) out = float3(satf(edgeMask));
  else if (p.view == 3) out = float3(satf(abs((out.x+out.y+out.z)-(rgb.x+rgb.y+rgb.z))*0.8f));
  else out = mix(rgb, out, amount);
  dst[dstIdx+0] = out.x;
  dst[dstIdx+1] = out.y;
  dst[dstIdx+2] = out.z;
  dst[dstIdx+3] = in.a;
}
