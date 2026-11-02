#!/usr/bin/env python3
"""Exercise production texture upload helpers with mocked GL state."""
from pathlib import Path
import os, subprocess, tempfile
base = Path(__file__).resolve().parents[1]
preamble = r'''
#include <cassert>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <limits>
#include <initializer_list>
#define APIENTRY
#define RMLUI_PLATFORM_UNIX
using GLenum=unsigned; using GLuint=unsigned; using GLint=int; using GLubyte=unsigned char;
namespace Rml { using byte=unsigned char; using TextureHandle=uintptr_t; struct Vector2i {int x,y;}; template<class T> struct Span {const T* p; size_t n; const T* data() const{return p;} size_t size() const{return n;}}; namespace Log {enum {LT_ERROR}; void Message(int,const char*,...) {}} }
enum { GL_VERSION=1, GL_EXTENSIONS, GL_MAX_TEXTURE_SIZE, GL_TEXTURE_BINDING_2D, GL_UNPACK_ALIGNMENT, GL_UNPACK_ROW_LENGTH, GL_UNPACK_SKIP_ROWS, GL_UNPACK_SKIP_PIXELS, GL_PIXEL_UNPACK_BUFFER_BINDING=0x88ef, GL_PIXEL_UNPACK_BUFFER=0x88ec, GL_TEXTURE_2D=50, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, GL_TEXTURE_WIDTH, GL_TEXTURE_HEIGHT, GL_TEXTURE_MIN_FILTER, GL_TEXTURE_MAG_FILTER, GL_LINEAR, GL_TEXTURE_WRAP_S, GL_TEXTURE_WRAP_T, GL_REPEAT };
int tex=9,pbo=7,alignv=8,row=5,skipr=2,skipp=3,w=0,h=0; bool failgen=false,failupload=false, deleted=false, support=true, proc=true, extension=false, core=false; int uploads=0, extension_queries=0;
const GLubyte* glGetString(int n){if(n==GL_VERSION) return (const GLubyte*)(core?"3.2":support?"2.1":"2.0"); ++extension_queries; return (const GLubyte*)(extension?"GL_ARB_pixel_buffer_object":"");}
void glBindBuffer(unsigned, unsigned b){pbo=b;} void* glXGetProcAddressARB(const GLubyte*){return proc?(void*)&glBindBuffer:nullptr;}
void glGetIntegerv(int k,int* v){*v=k==GL_MAX_TEXTURE_SIZE?1024:k==GL_TEXTURE_BINDING_2D?tex:k==GL_PIXEL_UNPACK_BUFFER_BINDING?pbo:k==GL_UNPACK_ALIGNMENT?alignv:k==GL_UNPACK_ROW_LENGTH?row:k==GL_UNPACK_SKIP_ROWS?skipr:skipp;}
void glPixelStorei(int k,int v){(k==GL_UNPACK_ALIGNMENT?alignv:k==GL_UNPACK_ROW_LENGTH?row:k==GL_UNPACK_SKIP_ROWS?skipr:skipp)=v;}
void glGenTextures(int,unsigned* id){*id=failgen?0:42;} void glDeleteTextures(int,const unsigned*){deleted=true;} void glBindTexture(int,int t){tex=t;}
void glTexImage2D(int,int,int,int x,int y,int,int,int,const void* p){assert(p && pbo==0 && alignv==1 && row==0 && skipr==0 && skipp==0); ++uploads; w=failupload?0:x; h=failupload?0:y;}
void glGetTexLevelParameteriv(int,int,int k,int* v){*v=k==GL_TEXTURE_WIDTH?w:h;} void glTexParameteri(int,int,int){}
'''
gl2=(base/'RmlUi_BackwardCompatible/RmlUi_Renderer_BackwardCompatible_GL2.cpp').read_text()
gl3=(base/'RmlUi_BackwardCompatible/RmlUi_Renderer_BackwardCompatible_GL3.cpp').read_text()
modern=(base/'RmlUi_Renderer_GL3.cpp').read_text()
helper=gl2[gl2.index('#if defined RMLUI_PLATFORM_WIN32\nusing BindUnpackBuffer'):gl2.index('RenderInterface_BackwardCompatible_GL2::RenderInterface_')]
for kind in ('gl2','gl3','modern'):
 for mobile in ([False,True] if kind!='gl2' else [False]):
  src={'gl2':gl2,'gl3':gl3,'modern':modern}[kind]
  if kind=='modern':
   body=src[src.index('static GLuint CreateTexture('):src.index('static void BindTexture(')]
   invoke='return CreateTexture({pixels,4},{1,1}) != 0;'
   decl=''
  else:
   cls='RenderInterface_BackwardCompatible_'+kind.upper()
   start=src.index('bool '+cls+'::GenerateTexture('); end=src.index('\n}',start)+2
   body=src[start:end]; decl=f'struct {cls} {{ bool GenerateTexture(Rml::TextureHandle&,const Rml::byte*,const Rml::Vector2i&); }};'
   invoke=f'{cls} r; Rml::TextureHandle handle=0; return r.GenerateTexture(handle,pixels,{{1,1}});'
  main=r'''int main(){unsigned char pixels[4]={1,2,3,4};
#define RUN() ([&](){ INVOKE })()
 assert(RUN()); assert(uploads==1 && tex==9 && pbo==7 && alignv==8 && row==5 && skipr==2 && skipp==3);
 failgen=true; assert(!RUN()); failgen=false; failupload=true; deleted=false;
#ifdef MOBILE
 assert(RUN()); assert(!deleted);
#else
 assert(!RUN()); assert(deleted);
#endif
 assert(tex==9 && pbo==7 && alignv==8 && row==5 && skipr==2 && skipp==3);
#ifdef TEST_GL2
 failupload=false; core=true; int before=extension_queries; assert(RUN()); assert(extension_queries==before); core=false; proc=false; assert(!RUN()); support=false; extension=true; proc=true; assert(RUN()); assert(extension_queries>before && pbo==7); extension=false; pbo=0; assert(RUN()); assert(pbo==0);
#endif
}'''.replace('INVOKE',invoke)
  text=preamble+decl+'\n'+(helper if kind=='gl2' else '')+body+main
  flags=os.environ.get('CXXFLAGS','').split()+(['-DRMLUI_PLATFORM_EMSCRIPTEN','-DMOBILE'] if mobile else [])+(['-DTEST_GL2'] if kind=='gl2' else [])
  with tempfile.TemporaryDirectory() as tmp:
   cpp=Path(tmp)/'test.cpp'; exe=Path(tmp)/'test'; cpp.write_text(text)
   subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-g',*flags,str(cpp),'-o',str(exe)],check=True)
   subprocess.run([str(exe)],check=True)
  print(kind, 'mobile' if mobile else 'desktop', 'passed')
