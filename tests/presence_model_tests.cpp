#include "PresenceCPU.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>
using namespace presence;

static float meanAbsDelta(const std::vector<float>& a,const std::vector<float>& b,int w,int h,int x0,int x1,int y0,int y1){
 double s=0;long n=0;int stride=w*4;for(int y=y0;y<y1;y++)for(int x=x0;x<x1;x++){int i=y*stride+x*4;for(int c=0;c<3;c++){s+=std::fabs(a[i+c]-b[i+c]);n++;}}return float(s/n);
}
static float stdChannel(const std::vector<float>& a,int w,int h,int ch,int x0,int x1,int y0,int y1){
 int stride=w*4;double m=0,v=0;long n=0;for(int y=y0;y<y1;y++)for(int x=x0;x<x1;x++){m+=a[y*stride+x*4+ch];n++;}m/=n;for(int y=y0;y<y1;y++)for(int x=x0;x<x1;x++){double d=a[y*stride+x*4+ch]-m;v+=d*d;}return float(std::sqrt(v/n));
}
int main(){
 const int w=192,h=96,stride=w*4;std::vector<float> src(stride*h),out(stride*h);
 // Sky on right, dark/warm object on left. Add tiny 8x8 codec-like variation to sky.
 for(int y=0;y<h;y++)for(int x=0;x<w;x++){int i=y*stride+x*4;if(x<80){src[i]=.12f;src[i+1]=.08f;src[i+2]=.04f;}else{float block=(((x/8)+(y/8))&1)?.0015f:-.0015f;src[i]=.34f+block;src[i+1]=.48f+block;src[i+2]=.53f+block;}src[i+3]=1;}
 Params p;p.amount=1;p.depth=.35f;p.micro=.20f;p.atmosphere=-.10f;p.edgeSoft=0;p.hiPresence=0;p.shPresence=.20f;p.texture=.12f;p.bloom=.05f;p.skinGuard=.70f;
 processRGBA(src.data(),out.data(),w,h,stride,stride,p);
 // The flat sky immediately beside the object must not acquire a broad halo.
 float nearEdge=meanAbsDelta(src,out,w,h,80,105,10,h-10);
 float farSky=meanAbsDelta(src,out,w,h,135,180,10,h-10);
 assert(nearEdge < 0.0040f);
 assert(nearEdge < farSky + 0.0030f);
 // Codec block variation in a flat sky must not materially grow.
 float s0=stdChannel(src,w,h,1,110,180,10,h-10),s1=stdChannel(out,w,h,1,110,180,10,h-10);
 assert(s1 <= s0*1.12f + 1e-7f);
 // Channel differences stay invariant in normal mode because output uses a shared RGB offset.
 for(int y=0;y<h;y++)for(int x=0;x<w;x++){int i=y*stride+x*4;assert(std::fabs((out[i]-out[i+1])-(src[i]-src[i+1]))<2e-6f);assert(std::fabs((out[i+1]-out[i+2])-(src[i+1]-src[i+2]))<2e-6f);}
 // Amount=0 exact bypass.
 p.amount=0;processRGBA(src.data(),out.data(),w,h,stride,stride,p);for(size_t i=0;i<src.size();i++)assert(std::fabs(src[i]-out[i])<1e-7f);
 std::cout<<"PresenceOFX v1.2 edge-aware regression tests passed\n";
 std::cout<<"near-edge mean abs delta: "<<nearEdge<<"\n";
 std::cout<<"flat-sky block std ratio: "<<(s1/s0)<<"\n";
}
