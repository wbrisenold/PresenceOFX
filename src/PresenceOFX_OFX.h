#pragma once
#include <cstdint>
#include <cstddef>

#if defined(_WIN32)
#define OfxExport __declspec(dllexport)
#else
#define OfxExport __attribute__((visibility("default")))
#endif


typedef int OfxStatus;
typedef void* OfxPropertySetHandle;
typedef void* OfxImageEffectHandle;
typedef void* OfxParamSetHandle;
typedef void* OfxParamHandle;
typedef void* OfxImageClipHandle;
typedef void* OfxImageMemoryHandle;

typedef struct OfxRectI { int x1, y1, x2, y2; } OfxRectI;
typedef struct OfxRectD { double x1, y1, x2, y2; } OfxRectD;
typedef struct OfxPointD { double x, y; } OfxPointD;

constexpr OfxStatus kOfxStatOK = 0;
constexpr OfxStatus kOfxStatFailed = 1;
constexpr OfxStatus kOfxStatErrFatal = 2;
constexpr OfxStatus kOfxStatErrUnknown = 3;
constexpr OfxStatus kOfxStatErrMissingHostFeature = 4;
constexpr OfxStatus kOfxStatErrUnsupported = 5;
constexpr OfxStatus kOfxStatReplyDefault = 14;

#define kOfxActionLoad "OfxActionLoad"
#define kOfxActionUnload "OfxActionUnload"
#define kOfxActionDescribe "OfxActionDescribe"
#define kOfxActionCreateInstance "OfxActionCreateInstance"
#define kOfxActionDestroyInstance "OfxActionDestroyInstance"
#define kOfxImageEffectActionDescribeInContext "OfxImageEffectActionDescribeInContext"
#define kOfxImageEffectActionGetRegionOfDefinition "OfxImageEffectActionGetRegionOfDefinition"
#define kOfxImageEffectActionGetRegionsOfInterest "OfxImageEffectActionGetRegionsOfInterest"
#define kOfxImageEffectActionRender "OfxImageEffectActionRender"

#define kOfxPropType "OfxPropType"
#define kOfxPropName "OfxPropName"
#define kOfxPropLabel "OfxPropLabel"
#define kOfxPropShortLabel "OfxPropShortLabel"
#define kOfxPropLongLabel "OfxPropLongLabel"
#define kOfxPropVersion "OfxPropVersion"
#define kOfxPropVersionLabel "OfxPropVersionLabel"
#define kOfxPropInstanceData "OfxPropInstanceData"
#define kOfxPropTime "OfxPropTime"
#define kOfxPropPluginDescription "OfxPropPluginDescription"
#define kOfxPropHostOSHandle "OfxPropHostOSHandle"

#define kOfxImageEffectPluginApi "OfxImageEffectPluginAPI"
#define kOfxImageEffectPropContext "OfxImageEffectPropContext"
#define kOfxImageEffectPropSupportedContexts "OfxImageEffectPropSupportedContexts"
#define kOfxImageEffectPropSupportedPixelDepths "OfxImageEffectPropSupportedPixelDepths"
#define kOfxImageEffectPropSupportsTiles "OfxImageEffectPropSupportsTiles"
#define kOfxImageEffectPropSupportsMultiResolution "OfxImageEffectPropSupportsMultiResolution"
#define kOfxImageEffectPropTemporalClipAccess "OfxImageEffectPropTemporalClipAccess"
#define kOfxImageEffectPropRenderTwiceAlways "OfxImageEffectPropRenderTwiceAlways"
#define kOfxImageEffectPropFieldRenderTwiceAlways "OfxImageEffectPropFieldRenderTwiceAlways"
#define kOfxImageEffectPropSupportsMultipleClipDepths "OfxImageEffectPropSupportsMultipleClipDepths"
#define kOfxImageEffectPropRenderQualityDraft "OfxImageEffectPropRenderQualityDraft"
#define kOfxImageEffectPropPixelDepth "OfxImageEffectPropPixelDepth"
#define kOfxImageEffectPropComponents "OfxImageEffectPropComponents"
#define kOfxImageEffectPropPreMultiplication "OfxImageEffectPropPreMultiplication"
#define kOfxImageEffectPropRenderWindow "OfxImageEffectPropRenderWindow"
#define kOfxImageEffectPropRegionOfDefinition "OfxImageEffectPropRegionOfDefinition"
#define kOfxImageEffectPropProjectSize "OfxImageEffectPropProjectSize"
#define kOfxImageEffectPropProjectOffset "OfxImageEffectPropProjectOffset"
#define kOfxImageEffectPropRenderScale "OfxImageEffectPropRenderScale"
#define kOfxImageEffectPropMetalRenderSupported "OfxImageEffectPropMetalRenderSupported"
#define kOfxImageEffectPropMetalEnabled "OfxImageEffectPropMetalEnabled"
#define kOfxImageEffectPropMetalCommandQueue "OfxImageEffectPropMetalCommandQueue"

#define kOfxImageClipPropFieldOrder "OfxImageClipPropFieldOrder"
#define kOfxImageClipPropIsMask "OfxImageClipPropIsMask"
#define kOfxImageClipPropConnected "OfxImageClipPropConnected"
#define kOfxImageClipPropOptional "OfxImageClipPropOptional"
#define kOfxImagePropData "OfxImagePropData"
#define kOfxImagePropBounds "OfxImagePropBounds"
#define kOfxImagePropRegionOfDefinition "OfxImagePropRegionOfDefinition"
#define kOfxImagePropRowBytes "OfxImagePropRowBytes"

#define kOfxImageComponentRGBA "OfxImageComponentRGBA"
#define kOfxBitDepthFloat "OfxBitDepthFloat"
#define kOfxImageOpaque "OfxImageOpaque"
#define kOfxImageFieldNone "OfxImageFieldNone"
#define kOfxImageEffectContextFilter "OfxImageEffectContextFilter"
#define kOfxImageEffectContextGeneral "OfxImageEffectContextGeneral"
#define kOfxImageEffectSimpleSourceClipName "Source"
#define kOfxImageEffectOutputClipName "Output"

#define kOfxParamTypeDouble "OfxParamTypeDouble"
#define kOfxParamTypeChoice "OfxParamTypeChoice"
#define kOfxParamPropDefault "OfxParamPropDefault"
#define kOfxParamPropMin "OfxParamPropMin"
#define kOfxParamPropMax "OfxParamPropMax"
#define kOfxParamPropDisplayMin "OfxParamPropDisplayMin"
#define kOfxParamPropDisplayMax "OfxParamPropDisplayMax"
#define kOfxParamPropIncrement "OfxParamPropIncrement"
#define kOfxParamPropChoiceOption "OfxParamPropChoiceOption"
#define kOfxParamPropScriptName "OfxParamPropScriptName"

typedef struct OfxPlugin {
  const char* pluginApi;
  int apiVersion;
  const char* pluginIdentifier;
  unsigned int pluginVersionMajor;
  unsigned int pluginVersionMinor;
  void (*setHost)(void* host);
  OfxStatus (*mainEntry)(const char* action, const void* handle, OfxPropertySetHandle inArgs, OfxPropertySetHandle outArgs);
} OfxPlugin;

typedef struct OfxHost {
  OfxPropertySetHandle host;
  void* (*fetchSuite)(OfxPropertySetHandle host, const char* suiteName, int suiteVersion);
} OfxHost;

#define kOfxPropertySuite "OfxPropertySuite"
#define kOfxImageEffectSuite "OfxImageEffectSuite"
#define kOfxParameterSuite "OfxParameterSuite"

struct OfxPropertySuiteV1 {
  OfxStatus (*propSetPointer)(OfxPropertySetHandle, const char*, int, void*);
  OfxStatus (*propSetString)(OfxPropertySetHandle, const char*, int, const char*);
  OfxStatus (*propSetDouble)(OfxPropertySetHandle, const char*, int, double);
  OfxStatus (*propSetInt)(OfxPropertySetHandle, const char*, int, int);
  OfxStatus (*propSetPointerN)(OfxPropertySetHandle, const char*, int, void* const*);
  OfxStatus (*propSetStringN)(OfxPropertySetHandle, const char*, int, const char* const*);
  OfxStatus (*propSetDoubleN)(OfxPropertySetHandle, const char*, int, const double*);
  OfxStatus (*propSetIntN)(OfxPropertySetHandle, const char*, int, const int*);
  OfxStatus (*propGetPointer)(OfxPropertySetHandle, const char*, int, void**);
  OfxStatus (*propGetString)(OfxPropertySetHandle, const char*, int, char**);
  OfxStatus (*propGetDouble)(OfxPropertySetHandle, const char*, int, double*);
  OfxStatus (*propGetInt)(OfxPropertySetHandle, const char*, int, int*);
  OfxStatus (*propGetPointerN)(OfxPropertySetHandle, const char*, int, void**);
  OfxStatus (*propGetStringN)(OfxPropertySetHandle, const char*, int, char**);
  OfxStatus (*propGetDoubleN)(OfxPropertySetHandle, const char*, int, double*);
  OfxStatus (*propGetIntN)(OfxPropertySetHandle, const char*, int, int*);
  OfxStatus (*propReset)(OfxPropertySetHandle, const char*);
  OfxStatus (*propGetDimension)(OfxPropertySetHandle, const char*, int*);
};

struct OfxImageEffectSuiteV1 {
  OfxStatus (*getPropertySet)(OfxImageEffectHandle, OfxPropertySetHandle*);
  OfxStatus (*getParamSet)(OfxImageEffectHandle, OfxParamSetHandle*);
  OfxStatus (*clipDefine)(OfxImageEffectHandle, const char*, OfxPropertySetHandle*);
  OfxStatus (*clipGetHandle)(OfxImageEffectHandle, const char*, OfxImageClipHandle*, OfxPropertySetHandle*);
  OfxStatus (*clipGetPropertySet)(OfxImageClipHandle, OfxPropertySetHandle*);
  OfxStatus (*clipGetImage)(OfxImageClipHandle, double, const OfxRectD*, OfxPropertySetHandle*);
  OfxStatus (*clipReleaseImage)(OfxPropertySetHandle);
  OfxStatus (*clipGetRegionOfDefinition)(OfxImageClipHandle, double, OfxRectD*);
  int (*abort)(OfxImageEffectHandle);
  OfxStatus (*imageMemoryAlloc)(OfxImageEffectHandle, size_t, OfxImageMemoryHandle*);
  OfxStatus (*imageMemoryFree)(OfxImageMemoryHandle);
  OfxStatus (*imageMemoryLock)(OfxImageMemoryHandle, void**);
  OfxStatus (*imageMemoryUnlock)(OfxImageMemoryHandle);
};

struct OfxParameterSuiteV1 {
  OfxStatus (*paramDefine)(OfxParamSetHandle, const char*, const char*, OfxPropertySetHandle*);
  OfxStatus (*paramGetHandle)(OfxParamSetHandle, const char*, OfxParamHandle*, OfxPropertySetHandle*);
  OfxStatus (*paramSetGetPropertySet)(OfxParamSetHandle, OfxPropertySetHandle*);
  OfxStatus (*paramGetPropertySet)(OfxParamHandle, OfxPropertySetHandle*);
  OfxStatus (*paramGetValue)(OfxParamHandle, ...);
  OfxStatus (*paramGetValueAtTime)(OfxParamHandle, double, ...);
  OfxStatus (*paramGetDerivative)(OfxParamHandle, double, ...);
  OfxStatus (*paramGetIntegral)(OfxParamHandle, double, double, ...);
  OfxStatus (*paramSetValue)(OfxParamHandle, ...);
  OfxStatus (*paramSetValueAtTime)(OfxParamHandle, double, ...);
};
