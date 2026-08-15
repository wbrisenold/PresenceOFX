#include "PresenceOFX_OFX.h"
#include "PresenceCPU.h"
#include <cstdarg>
#include <cstring>
#include <memory>

using namespace presence;

static OfxHost* gHost = nullptr;
static OfxPropertySuiteV1* gProp = nullptr;
static OfxImageEffectSuiteV1* gEffect = nullptr;
static OfxParameterSuiteV1* gParam = nullptr;

struct InstanceData {
  OfxImageClipHandle source = nullptr;
  OfxImageClipHandle output = nullptr;
  OfxParamHandle amount = nullptr;
  OfxParamHandle depth = nullptr;
  OfxParamHandle micro = nullptr;
  OfxParamHandle atmosphere = nullptr;
  OfxParamHandle edgeSoft = nullptr;
  OfxParamHandle hiPresence = nullptr;
  OfxParamHandle shPresence = nullptr;
  OfxParamHandle texture = nullptr;
  OfxParamHandle bloom = nullptr;
  OfxParamHandle skinGuard = nullptr;
  OfxParamHandle view = nullptr;
};

static void setHostFunc(void* host) {
  gHost = static_cast<OfxHost*>(host);
  if (!gHost || !gHost->fetchSuite) return;
  gProp = static_cast<OfxPropertySuiteV1*>(gHost->fetchSuite(gHost->host, kOfxPropertySuite, 1));
  gEffect = static_cast<OfxImageEffectSuiteV1*>(gHost->fetchSuite(gHost->host, kOfxImageEffectSuite, 1));
  gParam = static_cast<OfxParameterSuiteV1*>(gHost->fetchSuite(gHost->host, kOfxParameterSuite, 1));
}

static void setString(OfxPropertySetHandle h, const char* name, int idx, const char* value) { if (gProp) gProp->propSetString(h, name, idx, value); }
static void setInt(OfxPropertySetHandle h, const char* name, int idx, int value) { if (gProp) gProp->propSetInt(h, name, idx, value); }
static void setDouble(OfxPropertySetHandle h, const char* name, int idx, double value) { if (gProp) gProp->propSetDouble(h, name, idx, value); }
static void setPointer(OfxPropertySetHandle h, const char* name, int idx, void* value) { if (gProp) gProp->propSetPointer(h, name, idx, value); }

static void defineDouble(OfxParamSetHandle ps, const char* id, const char* label, double def, double mn, double mx, double step) {
  OfxPropertySetHandle p = nullptr;
  if (gParam->paramDefine(ps, kOfxParamTypeDouble, id, &p) != kOfxStatOK) return;
  setString(p, kOfxPropLabel, 0, label);
  setString(p, kOfxPropShortLabel, 0, label);
  setString(p, kOfxParamPropScriptName, 0, id);
  setDouble(p, kOfxParamPropDefault, 0, def);
  setDouble(p, kOfxParamPropMin, 0, mn);
  setDouble(p, kOfxParamPropMax, 0, mx);
  setDouble(p, kOfxParamPropDisplayMin, 0, mn);
  setDouble(p, kOfxParamPropDisplayMax, 0, mx);
  setDouble(p, kOfxParamPropIncrement, 0, step);
}

static void defineChoice(OfxParamSetHandle ps, const char* id, const char* label, int def) {
  OfxPropertySetHandle p = nullptr;
  if (gParam->paramDefine(ps, kOfxParamTypeChoice, id, &p) != kOfxStatOK) return;
  setString(p, kOfxPropLabel, 0, label);
  setString(p, kOfxPropShortLabel, 0, label);
  setString(p, kOfxParamPropScriptName, 0, id);
  setInt(p, kOfxParamPropDefault, 0, def);
  setString(p, kOfxParamPropChoiceOption, 0, "Normal");
  setString(p, kOfxParamPropChoiceOption, 1, "Presence Mask");
  setString(p, kOfxParamPropChoiceOption, 2, "Edge Mask");
  setString(p, kOfxParamPropChoiceOption, 3, "Difference");
}

static OfxStatus describe(OfxImageEffectHandle effect) {
  if (!gProp || !gEffect || !gParam) return kOfxStatErrMissingHostFeature;
  OfxPropertySetHandle props = nullptr;
  gEffect->getPropertySet(effect, &props);
  setString(props, kOfxPropLabel, 0, "PresenceOFX");
  setString(props, kOfxPropShortLabel, 0, "Presence");
  setString(props, kOfxPropLongLabel, 0, "PresenceOFX - Dimensionality and image character");
  setString(props, kOfxPropPluginDescription, 0, "Spatial presence, local depth, texture, dehaze/atmosphere, and optical integration.");
  setString(props, kOfxImageEffectPropSupportedContexts, 0, kOfxImageEffectContextFilter);
  setString(props, kOfxImageEffectPropSupportedContexts, 1, kOfxImageEffectContextGeneral);
  setString(props, kOfxImageEffectPropSupportedPixelDepths, 0, kOfxBitDepthFloat);
  setInt(props, kOfxImageEffectPropSupportsTiles, 0, 0);
  setInt(props, kOfxImageEffectPropSupportsMultiResolution, 0, 1);
  setInt(props, kOfxImageEffectPropSupportsMultipleClipDepths, 0, 0);
  setInt(props, kOfxImageEffectPropTemporalClipAccess, 0, 0);
  setInt(props, kOfxImageEffectPropRenderTwiceAlways, 0, 0);
  return kOfxStatOK;
}

static OfxStatus describeInContext(OfxImageEffectHandle effect) {
  OfxPropertySetHandle p = nullptr;
  gEffect->clipDefine(effect, kOfxImageEffectSimpleSourceClipName, &p);
  setString(p, kOfxImageEffectPropSupportedPixelDepths, 0, kOfxBitDepthFloat);
  setString(p, kOfxImageEffectPropComponents, 0, kOfxImageComponentRGBA);
  setString(p, kOfxImageClipPropFieldOrder, 0, kOfxImageFieldNone);

  gEffect->clipDefine(effect, kOfxImageEffectOutputClipName, &p);
  setString(p, kOfxImageEffectPropSupportedPixelDepths, 0, kOfxBitDepthFloat);
  setString(p, kOfxImageEffectPropComponents, 0, kOfxImageComponentRGBA);
  setString(p, kOfxImageClipPropFieldOrder, 0, kOfxImageFieldNone);

  OfxParamSetHandle ps = nullptr;
  gEffect->getParamSet(effect, &ps);
  defineDouble(ps, "amount", "Amount", 1.0, 0.0, 2.0, 0.01);
  defineDouble(ps, "depth", "Depth", 0.35, -1.0, 1.0, 0.01);
  defineDouble(ps, "micro", "Micro", 0.20, -1.0, 1.0, 0.01);
  defineDouble(ps, "atmosphere", "Atmosphere", -0.10, -1.0, 1.0, 0.01);
  defineDouble(ps, "edgeSoft", "Edge Soft", 0.15, 0.0, 1.0, 0.01);
  defineDouble(ps, "hiPresence", "Hi Presence", 0.20, 0.0, 1.0, 0.01);
  defineDouble(ps, "shPresence", "Sh Presence", 0.20, 0.0, 1.0, 0.01);
  defineDouble(ps, "texture", "Texture", 0.12, 0.0, 1.0, 0.01);
  defineDouble(ps, "bloom", "Bloom", 0.05, 0.0, 1.0, 0.01);
  defineDouble(ps, "skinGuard", "Skin Guard", 0.70, 0.0, 1.0, 0.01);
  defineChoice(ps, "view", "View", 0);
  return kOfxStatOK;
}

static InstanceData* getInstance(OfxImageEffectHandle effect) {
  OfxPropertySetHandle props = nullptr;
  if (!gEffect || !gProp || gEffect->getPropertySet(effect, &props) != kOfxStatOK) return nullptr;
  void* ptr = nullptr;
  if (gProp->propGetPointer(props, kOfxPropInstanceData, 0, &ptr) != kOfxStatOK) return nullptr;
  return static_cast<InstanceData*>(ptr);
}

static OfxStatus createInstance(OfxImageEffectHandle effect) {
  auto* data = new InstanceData();
  OfxPropertySetHandle props = nullptr;
  gEffect->getPropertySet(effect, &props);
  setPointer(props, kOfxPropInstanceData, 0, data);
  OfxPropertySetHandle dummy = nullptr;
  gEffect->clipGetHandle(effect, kOfxImageEffectSimpleSourceClipName, &data->source, &dummy);
  gEffect->clipGetHandle(effect, kOfxImageEffectOutputClipName, &data->output, &dummy);
  OfxParamSetHandle ps = nullptr;
  gEffect->getParamSet(effect, &ps);
  gParam->paramGetHandle(ps, "amount", &data->amount, &dummy);
  gParam->paramGetHandle(ps, "depth", &data->depth, &dummy);
  gParam->paramGetHandle(ps, "micro", &data->micro, &dummy);
  gParam->paramGetHandle(ps, "atmosphere", &data->atmosphere, &dummy);
  gParam->paramGetHandle(ps, "edgeSoft", &data->edgeSoft, &dummy);
  gParam->paramGetHandle(ps, "hiPresence", &data->hiPresence, &dummy);
  gParam->paramGetHandle(ps, "shPresence", &data->shPresence, &dummy);
  gParam->paramGetHandle(ps, "texture", &data->texture, &dummy);
  gParam->paramGetHandle(ps, "bloom", &data->bloom, &dummy);
  gParam->paramGetHandle(ps, "skinGuard", &data->skinGuard, &dummy);
  gParam->paramGetHandle(ps, "view", &data->view, &dummy);
  return kOfxStatOK;
}

static OfxStatus destroyInstance(OfxImageEffectHandle effect) {
  InstanceData* data = getInstance(effect);
  delete data;
  OfxPropertySetHandle props = nullptr;
  gEffect->getPropertySet(effect, &props);
  setPointer(props, kOfxPropInstanceData, 0, nullptr);
  return kOfxStatOK;
}

static double getDoubleAt(OfxParamHandle h, double time, double def) {
  if (!h || !gParam) return def;
  double v = def;
  if (gParam->paramGetValueAtTime(h, time, &v) != kOfxStatOK) return def;
  return v;
}

static int getChoiceAt(OfxParamHandle h, double time, int def) {
  if (!h || !gParam) return def;
  int v = def;
  if (gParam->paramGetValueAtTime(h, time, &v) != kOfxStatOK) return def;
  return v;
}

static Params fetchParams(InstanceData* d, double time) {
  Params p;
  p.amount = static_cast<float>(getDoubleAt(d->amount, time, p.amount));
  p.depth = static_cast<float>(getDoubleAt(d->depth, time, p.depth));
  p.micro = static_cast<float>(getDoubleAt(d->micro, time, p.micro));
  p.atmosphere = static_cast<float>(getDoubleAt(d->atmosphere, time, p.atmosphere));
  p.edgeSoft = static_cast<float>(getDoubleAt(d->edgeSoft, time, p.edgeSoft));
  p.hiPresence = static_cast<float>(getDoubleAt(d->hiPresence, time, p.hiPresence));
  p.shPresence = static_cast<float>(getDoubleAt(d->shPresence, time, p.shPresence));
  p.texture = static_cast<float>(getDoubleAt(d->texture, time, p.texture));
  p.bloom = static_cast<float>(getDoubleAt(d->bloom, time, p.bloom));
  p.skinGuard = static_cast<float>(getDoubleAt(d->skinGuard, time, p.skinGuard));
  p.view = getChoiceAt(d->view, time, p.view);
  return p;
}

static OfxStatus render(OfxImageEffectHandle effect, OfxPropertySetHandle inArgs) {
  InstanceData* d = getInstance(effect);
  if (!d || !d->source || !d->output) return kOfxStatFailed;
  double time = 0.0;
  gProp->propGetDouble(inArgs, kOfxPropTime, 0, &time);

  OfxPropertySetHandle srcImg = nullptr, dstImg = nullptr;
  OfxStatus s1 = gEffect->clipGetImage(d->source, time, nullptr, &srcImg);
  OfxStatus s2 = gEffect->clipGetImage(d->output, time, nullptr, &dstImg);
  if (s1 != kOfxStatOK || s2 != kOfxStatOK || !srcImg || !dstImg) {
    if (srcImg) gEffect->clipReleaseImage(srcImg);
    if (dstImg) gEffect->clipReleaseImage(dstImg);
    return kOfxStatFailed;
  }

  void* srcData = nullptr;
  void* dstData = nullptr;
  int srcRowBytes = 0;
  int dstRowBytes = 0;
  int bounds[4] = {0,0,0,0};
  gProp->propGetPointer(srcImg, kOfxImagePropData, 0, &srcData);
  gProp->propGetPointer(dstImg, kOfxImagePropData, 0, &dstData);
  gProp->propGetInt(srcImg, kOfxImagePropRowBytes, 0, &srcRowBytes);
  gProp->propGetInt(dstImg, kOfxImagePropRowBytes, 0, &dstRowBytes);
  gProp->propGetIntN(dstImg, kOfxImagePropBounds, 4, bounds);
  int width = bounds[2] - bounds[0];
  int height = bounds[3] - bounds[1];
  int srcStrideFloats = srcRowBytes / static_cast<int>(sizeof(float));
  int dstStrideFloats = dstRowBytes / static_cast<int>(sizeof(float));

  Params p = fetchParams(d, time);
  processRGBA(static_cast<const float*>(srcData),static_cast<float*>(dstData),width,height,srcStrideFloats,dstStrideFloats,p);
  gEffect->clipReleaseImage(srcImg); gEffect->clipReleaseImage(dstImg); return kOfxStatOK;
}

static OfxStatus getRegionOfDefinition(OfxImageEffectHandle effect, OfxPropertySetHandle inArgs, OfxPropertySetHandle outArgs) {
  (void)inArgs;
  InstanceData* d = getInstance(effect);
  if (!d || !d->source) return kOfxStatReplyDefault;
  double time = 0.0;
  gProp->propGetDouble(inArgs, kOfxPropTime, 0, &time);
  OfxRectD rod{0,0,0,0};
  if (gEffect->clipGetRegionOfDefinition(d->source, time, &rod) == kOfxStatOK) {
    double vals[4] = {rod.x1, rod.y1, rod.x2, rod.y2};
    gProp->propSetDoubleN(outArgs, kOfxImageEffectPropRegionOfDefinition, 4, vals);
    return kOfxStatOK;
  }
  return kOfxStatReplyDefault;
}

static OfxStatus mainEntry(const char* action, const void* handle, OfxPropertySetHandle inArgs, OfxPropertySetHandle outArgs) {
  OfxImageEffectHandle effect = (OfxImageEffectHandle)handle;
  if (!std::strcmp(action, kOfxActionDescribe)) return describe(effect);
  if (!std::strcmp(action, kOfxImageEffectActionDescribeInContext)) return describeInContext(effect);
  if (!std::strcmp(action, kOfxActionCreateInstance)) return createInstance(effect);
  if (!std::strcmp(action, kOfxActionDestroyInstance)) return destroyInstance(effect);
  if (!std::strcmp(action, kOfxImageEffectActionRender)) return render(effect, inArgs);
  if (!std::strcmp(action, kOfxImageEffectActionGetRegionOfDefinition)) return getRegionOfDefinition(effect, inArgs, outArgs);
  return kOfxStatReplyDefault;
}

static OfxPlugin gPlugin = {
  kOfxImageEffectPluginApi,
  1,
  "com.luma.presenceofx",
  1,
  0,
  setHostFunc,
  mainEntry
};

extern "C" {
OfxExport int OfxGetNumberOfPlugins(void) { return 1; }
OfxExport OfxPlugin* OfxGetPlugin(int nth) { return nth == 0 ? &gPlugin : nullptr; }
OfxExport void OfxSetHost(void* host) { setHostFunc(host); }
}
