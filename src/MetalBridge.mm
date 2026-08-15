#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#include <dlfcn.h>
#include <mutex>
#include "MetalBridge.h"
namespace presence {
struct MetalState { id<MTLDevice> device=nil; id<MTLLibrary> library=nil; id<MTLComputePipelineState> pipeline=nil; bool ready=false; };
static std::mutex gMutex; static MetalState gState;
struct MetalParams { float amount,depth,micro,atmosphere,edgeSoft,hiPresence,shPresence,texture,bloom,skinGuard; int view,width,height,srcStrideFloats,dstStrideFloats; };
static NSURL* metallibURL(){ Dl_info i{}; if(dladdr((const void*)&metallibURL,&i)==0||!i.dli_fname) return nil; NSString* b=[NSString stringWithUTF8String:i.dli_fname]; NSString* contents=[[b stringByDeletingLastPathComponent] stringByDeletingLastPathComponent]; return [NSURL fileURLWithPath:[[[contents stringByAppendingPathComponent:@"Resources"] stringByAppendingPathComponent:@"PresenceKernels"] stringByAppendingPathExtension:@"metallib"]]; }
static bool initMetal(id<MTLCommandQueue> q){ std::lock_guard<std::mutex> l(gMutex); if(!q) return false; id<MTLDevice> d=[q device]; if(!d) return false; if(gState.ready&&gState.device==d) return true; gState=MetalState{}; gState.device=d; NSError* e=nil; NSURL* u=metallibURL(); if(!u) return false; gState.library=[d newLibraryWithURL:u error:&e]; if(!gState.library) return false; id<MTLFunction> f=[gState.library newFunctionWithName:@"presenceKernel"]; if(!f) return false; gState.pipeline=[d newComputePipelineStateWithFunction:f error:&e]; if(!gState.pipeline) return false; gState.ready=true; return true; }
bool runMetal(void* cq,int w,int h,void* srcH,void* dstH,int ss,int ds,const Params& p){ id<MTLCommandQueue> q=(__bridge id<MTLCommandQueue>)cq; id<MTLBuffer> inB=(__bridge id<MTLBuffer>)srcH; id<MTLBuffer> outB=(__bridge id<MTLBuffer>)dstH; if(!q||!inB||!outB||w<=0||h<=0||!initMetal(q)) return false; NSUInteger sb=(NSUInteger)ss*h*sizeof(float), db=(NSUInteger)ds*h*sizeof(float); if([inB length]<sb||[outB length]<db) return false; MetalParams mp{p.amount,p.depth,p.micro,p.atmosphere,p.edgeSoft,p.hiPresence,p.shPresence,p.texture,p.bloom,p.skinGuard,p.view,w,h,ss,ds}; id<MTLCommandBuffer> cb=[q commandBuffer]; if(!cb) return false; id<MTLComputeCommandEncoder> enc=[cb computeCommandEncoder]; if(!enc) return false; [enc setComputePipelineState:gState.pipeline]; [enc setBuffer:inB offset:0 atIndex:0]; [enc setBuffer:outB offset:0 atIndex:1]; [enc setBytes:&mp length:sizeof(mp) atIndex:2]; NSUInteger tw=gState.pipeline.threadExecutionWidth;
  if(tw<1) tw=1;
  NSUInteger available=gState.pipeline.maxTotalThreadsPerThreadgroup/tw;
  NSUInteger th=available<16?available:16;
  if(th<1) th=1; [enc dispatchThreads:MTLSizeMake(w,h,1) threadsPerThreadgroup:MTLSizeMake(tw,th,1)]; [enc endEncoding]; [cb commit]; [cb waitUntilCompleted]; return [cb status]==MTLCommandBufferStatusCompleted; }
}
