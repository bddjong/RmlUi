#!/usr/bin/env python3
"""Compile production TGA loaders against a file/upload mock, optionally under ASan."""
from pathlib import Path
import os
import subprocess
import tempfile

base = Path(__file__).resolve().parents[1]
files = [('RmlUi_BackwardCompatible/RmlUi_Renderer_BackwardCompatible_GL2.cpp', 'RenderInterface_BackwardCompatible_GL2'),
         ('RmlUi_BackwardCompatible/RmlUi_Renderer_BackwardCompatible_GL3.cpp', 'RenderInterface_BackwardCompatible_GL3'),
         ('RmlUi_Renderer_GL3.cpp', 'RenderInterface_GL3')]
preamble = r'''
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <limits>
#include <memory>
#include <new>
#include <cstdlib>
int fail_allocation = 0;
void* operator new[](size_t n, const std::nothrow_t&) noexcept { if (fail_allocation && --fail_allocation == 0) return nullptr; return std::malloc(n); }
void operator delete[](void* p) noexcept { std::free(p); }
#include <string>
#include <vector>
#include <algorithm>
namespace Rml {
using byte=unsigned char; using String=std::string; using TextureHandle=uintptr_t; using FileHandle=int;
template<class T> using UniquePtr=std::unique_ptr<T>;
template<class T> struct Span { const T* p; size_t n; };
struct Vector2i { int x=0,y=0; };
std::vector<byte> input, output; bool short_read=false; int fail_seek=0, seeks=0, closes=0;
struct FileInterface { int Open(const String&){return 1;} bool Seek(int,int,int){ return ++seeks != fail_seek; } size_t Tell(int){return input.size();} size_t Read(void* p,size_t n,int){size_t count=n-(short_read?1:0); memcpy(p,input.data(),count); return count;} void Close(int){++closes;} };
FileInterface file; FileInterface* GetFileInterface(){return &file;}
namespace Log { enum {LT_ERROR}; void Message(int,const char*,...){} }
}
'''
test = r'''
int main(){
 for(int bits: {24,32}) for(int alpha_bits: {0,8}) for(int origin: {0,16,32,48}) {
  if(bits==24 && alpha_bits==8) continue;
  Rml::input.assign(18+2+2*2*(bits/8),0); auto& a=Rml::input;
  a[0]=2; a[2]=2; a[12]=2; a[14]=2; a[16]=bits; a[17]=origin|alpha_bits;
  for(int y=0;y<2;y++) for(int x=0;x<2;x++) {int row=(origin&32)?y:1-y, col=(origin&16)?1-x:x; int i=20+(y*2+x)*(bits/8); a[i]=row*2+col+1; a[i+1]=20; a[i+2]=30; if(bits==32)a[i+3]=128;}
  Rml::Vector2i dim; Rml::output.clear(); assert(run(dim)); assert(dim.x==2 && dim.y==2 && Rml::output.size()==16);
  for(int i=0;i<4;i++){
   int alpha=alpha_bits?128:255;
#ifdef PREMULTIPLY
   assert(Rml::output[i*4]==30*alpha/255 && Rml::output[i*4+1]==20*alpha/255 && Rml::output[i*4+2]==(i+1)*alpha/255);
#else
   assert(Rml::output[i*4]==30 && Rml::output[i*4+1]==20 && Rml::output[i*4+2]==i+1);
#endif
   assert(Rml::output[i*4+3]==alpha);
  }
  for(int invalid: {1,4,15}) { a[17]=origin|invalid; assert(!run(dim)); } a[17]=origin|alpha_bits;
  if(bits==24) {a[17]=origin|8; assert(!run(dim)); a[17]=origin;}
  auto valid=a;
  for(int failure: {1,2}) { Rml::seeks=0; Rml::closes=0; Rml::fail_seek=failure; assert(!run(dim)); assert(Rml::closes==1); } Rml::fail_seek=0;
  for(int failure: {1,2}) { Rml::seeks=0; Rml::closes=0; fail_allocation=failure; assert(!run(dim)); assert(Rml::closes==1); } fail_allocation=0;
  for(size_t n=0;n<valid.size();n++){a.assign(valid.begin(),valid.begin()+n); assert(!run(dim));} a=valid;
  Rml::short_read=true; assert(!run(dim)); Rml::short_read=false;
  for(int offset: {1,2,12,14,16,17}) {a=valid; a[offset]=offset==17?0x40:offset==16?25:offset==2?10:offset==1?1:0; assert(!run(dim));}
 }
}
'''
for rel, cls in files:
 source=(base / rel).read_text(); start=source.index('struct TGAHeader {'); end=source.index('\n}',source.index('::LoadTexture(',start))+2
 loader='#pragma pack(1)\n'+source[start:end]
 modern=cls=='RenderInterface_GL3'
 if modern:
  decl=f'struct {cls} {{ Rml::TextureHandle LoadTexture(Rml::Vector2i&,const Rml::String&); Rml::TextureHandle GenerateTexture(Rml::Span<const Rml::byte> s,Rml::Vector2i) {{ Rml::output.assign(s.p,s.p+s.n); return 1; }} }};\n'
  run=f'bool run(Rml::Vector2i& d) {{ {cls} r; return r.LoadTexture(d, "x") != 0; }}\n'
 else:
  decl=f'struct {cls} {{ bool LoadTexture(Rml::TextureHandle&,Rml::Vector2i&,const Rml::String&); bool GenerateTexture(Rml::TextureHandle&,const Rml::byte* p,Rml::Vector2i d) {{ Rml::output.assign(p,p+d.x*d.y*4); return true; }} }};\n'
  run=f'bool run(Rml::Vector2i& d) {{ {cls} r; Rml::TextureHandle h=0; return r.LoadTexture(h,d,"x"); }}\n'
 with tempfile.TemporaryDirectory() as tmp:
  cpp=Path(tmp)/'test.cpp'; exe=Path(tmp)/'test'; cpp.write_text(preamble+decl+loader+'\n'+run+test)
  flags=os.environ.get('CXXFLAGS','').split()
  if modern or (cls.endswith('GL3') and '-DRMLUI_SRGB_PREMULTIPLIED_ALPHA' in flags): flags.append('-DPREMULTIPLY')
  subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-g',*flags,str(cpp),'-o',str(exe)],check=True)
  subprocess.run([str(exe)],check=True)
  print(rel, 'passed')
