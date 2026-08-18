#include "PresenceCPU.h"
#include <algorithm>
#include <cmath>
namespace presence {
static inline float cf(float x,float a,float b){return std::max(a,std::min(b,x));}
static inline int ci(int x,int a,int b){return std::max(a,std::min(b,x));}
static inline float ss(float a,float b,float x){if(b<=a)return x>=b?1.f:0.f;x=cf((x-a)/(b-a),0.f,1.f);return x*x*(3.f-2.f*x);}
static inline float lum(float r,float g,float b){return .2126f*r+.7152f*g+.0722f*b;}
struct C3{float r,g,b;};
static inline C3 pix(const float* s,int w,int h,int stride,int x,int y){x=ci(x,0,w-1);y=ci(y,0,h-1);const float* p=s+y*stride+x*4;return {p[0],p[1],p[2]};}
static inline float gw(C3 c,C3 s,float sigma){float dr=s.r-c.r,dg=s.g-c.g,db=s.b-c.b;float d2=dr*dr+dg*dg+db*db;float s2=std::max(sigma*sigma,1e-8f);return 1.f/(1.f+d2/s2);}
static inline void add(const float* s,int w,int h,int stride,int x,int y,int dx,int dy,float sw,float sig,C3 c,float& sy,float& ww){C3 q=pix(s,w,h,stride,x+dx,y+dy);float z=sw*gw(c,q,sig);sy+=z*lum(q.r,q.g,q.b);ww+=z;}
static inline float blur(const float* s,int w,int h,int stride,int x,int y,int radius,float sig){
 int r=std::max(1,radius), hh=std::max(1,r/2);
 int h7=std::max(1,int(float(hh)*.70710678f+.5f));
 int r92=std::max(1,int(float(r)*.92387953f+.5f));
 int r38=std::max(1,int(float(r)*.38268343f+.5f));
 C3 c=pix(s,w,h,stride,x,y); float sy=4.f*lum(c.r,c.g,c.b), ww=4.f;
 add(s,w,h,stride,x,y, hh,0,1.5f,sig,c,sy,ww); add(s,w,h,stride,x,y,-hh,0,1.5f,sig,c,sy,ww);
 add(s,w,h,stride,x,y,0, hh,1.5f,sig,c,sy,ww); add(s,w,h,stride,x,y,0,-hh,1.5f,sig,c,sy,ww);
 add(s,w,h,stride,x,y, h7, h7,1.15f,sig,c,sy,ww); add(s,w,h,stride,x,y,-h7, h7,1.15f,sig,c,sy,ww);
 add(s,w,h,stride,x,y, h7,-h7,1.15f,sig,c,sy,ww); add(s,w,h,stride,x,y,-h7,-h7,1.15f,sig,c,sy,ww);
 add(s,w,h,stride,x,y, r92, r38,.8f,sig,c,sy,ww); add(s,w,h,stride,x,y, r38, r92,.8f,sig,c,sy,ww);
 add(s,w,h,stride,x,y,-r38, r92,.8f,sig,c,sy,ww); add(s,w,h,stride,x,y,-r92, r38,.8f,sig,c,sy,ww);
 add(s,w,h,stride,x,y,-r92,-r38,.8f,sig,c,sy,ww); add(s,w,h,stride,x,y,-r38,-r92,.8f,sig,c,sy,ww);
 add(s,w,h,stride,x,y, r38,-r92,.8f,sig,c,sy,ww); add(s,w,h,stride,x,y, r92,-r38,.8f,sig,c,sy,ww);
 return sy/std::max(ww,1e-8f);
}
static inline float dead(float x,float t){float a=std::fabs(x);if(a<=t)return 0.f;float d=a-t;float k=ss(0.f,t*2.f+1e-6f,d);return (x<0?-1.f:1.f)*d*k;}
static inline float limit(float x,float L){L=std::max(L,1e-6f);return x/(1.f+std::fabs(x)/L);}
static inline float skin(float r,float g,float b,float y){float sum=r+g+b+1e-6f,nr=r/sum,ng=g/sum;float mx=std::max(r,std::max(g,b)),mn=std::min(r,std::min(g,b));float chrom=ss(.025f,.22f,mx-mn);float rg=ss(.32f,.48f,nr)*(1-ss(.50f,.62f,nr));float gg=ss(.24f,.38f,ng)*(1-ss(.42f,.52f,ng));float yl=ss(.05f,.25f,y)*(1-ss(.92f,1.25f,y));return cf(rg*gg*yl*chrom,0.f,1.f);}
void processRGBA(const float* s,float* d,int w,int h,int ssf,int dsf,const Params& pin){
 Params p=pin;p.amount=cf(p.amount,0,2);p.depth=cf(p.depth,-1,1);p.micro=cf(p.micro,-1,1);p.atmosphere=cf(p.atmosphere,-1,1);p.edgeSoft=cf(p.edgeSoft,0,1);p.hiPresence=cf(p.hiPresence,0,1);p.shPresence=cf(p.shPresence,0,1);p.texture=cf(p.texture,0,1);p.bloom=cf(p.bloom,0,1);p.skinGuard=cf(p.skinGuard,0,1);
 int rs=std::max(1,w/420),rl=std::max(4,w/85),rb=std::max(6,w/55);
 for(int y=0;y<h;y++)for(int x=0;x<w;x++){const float* q=s+y*ssf+x*4;float* o=d+y*dsf+x*4;float r=q[0],g=q[1],b=q[2],Y=lum(r,g,b);
  if(p.amount==0.f&&p.view==0){o[0]=r;o[1]=g;o[2]=b;o[3]=q[3];continue;}
  float sm=blur(s,w,h,ssf,x,y,rs,.095f),la=blur(s,w,h,ssf,x,y,rl,.055f),bi=blur(s,w,h,ssf,x,y,rb,.085f);
  float br0=Y-la,mi0=sm-la,fi0=Y-sm,nf=.0018f+.0022f*cf(std::fabs(Y),0,1.5f);
  float br=dead(br0,nf*1.35f),mi=dead(mi0,nf*.85f),fi=dead(fi0,nf*1.10f);
  float ee=std::fabs(fi)+.45f*std::fabs(mi),em=ss(.005f,.05f,ee),dg=ss(nf*2.5f,nf*9.f+1e-6f,std::fabs(fi0));
  float sk=skin(r,g,b,Y),prot=1-p.skinGuard*sk,hi=ss(.55f,1.15f,Y),sh=1-ss(.05f,.35f,Y);
  float bg=1-.80f*em,hfg=(1-p.edgeSoft*.75f)*dg;
  float pres=0;pres+=p.depth*br*bg*.95f;pres+=p.micro*fi*hfg*.58f;pres+=p.hiPresence*hi*mi*dg*.52f;pres+=p.shPresence*sh*mi*dg*.56f;pres+=p.texture*fi*dg*(1-sk*.75f)*.18f;pres+=p.atmosphere*br*bg*.35f;
  float bloomE=std::max(0.f,bi-std::max(Y,.72f));float bl=p.bloom*bloomE*.07f;float delta=(pres*prot+bl)*p.amount;delta=limit(delta,.032f+.085f*cf(std::fabs(Y),0,1.5f));
  if(p.view==1){float m=cf(.5f+pres*7.f,0,1);o[0]=o[1]=o[2]=m;}else if(p.view==2){o[0]=o[1]=o[2]=cf(em,0,1);}else if(p.view==3){o[0]=o[1]=o[2]=cf(std::fabs(delta)*10.f,0,1);}else{o[0]=r+delta;o[1]=g+delta;o[2]=b+delta;}o[3]=q[3];
 }
}
}
