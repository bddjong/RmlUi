#!/usr/bin/env python3
"""Exercise the production framebuffer helper against a small GL state mock."""
from pathlib import Path
import os
import subprocess
import tempfile

source = (Path(__file__).resolve().parents[1] / 'RmlUi_Renderer_GL3.cpp').read_text()
helper = source[source.index('static bool CreateFramebuffer('):source.index('static GLuint CreateTexture(')]
texture_helper = source[source.index('static GLuint CreateTexture('):source.index('static void BindTexture(')]
preamble = r'''
#include <cassert>
#include <limits>
#include <set>
using GLuint = unsigned; using GLenum = unsigned; using GLint = int;
namespace Rml { struct Colourf { float v[4]{}; Colourf(float,float) {} const float& operator[](int i) const{return v[i];} }; namespace Log { enum {LT_ERROR}; void Message(int,const char*,...) {} } }
enum { GL_CLAMP_TO_BORDER=1, GL_CLAMP_TO_EDGE, GL_RGBA8, GL_LINEAR, GL_READ_FRAMEBUFFER_BINDING, GL_DRAW_FRAMEBUFFER_BINDING, GL_TEXTURE_BINDING_2D, GL_RENDERBUFFER_BINDING, GL_FRAMEBUFFER, GL_READ_FRAMEBUFFER, GL_DRAW_FRAMEBUFFER, GL_RENDERBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, GL_RGBA, GL_UNSIGNED_BYTE, GL_TEXTURE_MIN_FILTER, GL_TEXTURE_MAG_FILTER, GL_TEXTURE_WRAP_S, GL_TEXTURE_WRAP_T, GL_TEXTURE_BORDER_COLOR, GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL_ATTACHMENT, GL_FRAMEBUFFER_COMPLETE, GL_REPEAT, GL_BLEND, GL_PIXEL_UNPACK_BUFFER, GL_PIXEL_UNPACK_BUFFER_BINDING, GL_MAX_TEXTURE_SIZE, GL_UNPACK_ALIGNMENT, GL_UNPACK_ROW_LENGTH, GL_UNPACK_SKIP_ROWS, GL_UNPACK_SKIP_PIXELS };
struct FramebufferData { int width, height; GLuint framebuffer, color_tex_buffer, color_render_buffer, depth_stencil_buffer; bool owns_depth_stencil_buffer; };
enum class FramebufferAttachment { None, DepthStencil };
int readfb=10, drawfb=11, tex=12, rb=13, unpack_buffer=17; unsigned nextid=100; bool complete=false; int fail_at=0, generation=0; std::set<unsigned> live;
void glGetIntegerv(int p,int* v){if(p==211 || p==212){for(int i=0;i<4;i++)v[i]=0;return;} *v=p==GL_READ_FRAMEBUFFER_BINDING?readfb:p==GL_DRAW_FRAMEBUFFER_BINDING?drawfb:p==GL_TEXTURE_BINDING_2D?tex:p==GL_PIXEL_UNPACK_BUFFER_BINDING?unpack_buffer:p==GL_MAX_TEXTURE_SIZE?4096:rb;}
void glBindBuffer(int target,int value){if(target==GL_PIXEL_UNPACK_BUFFER)unpack_buffer=value;}
void glPixelStorei(int,int){}
void gen(int, unsigned* v){if (++generation==fail_at) {*v=0; return;} *v=nextid++; live.insert(*v);} void del(int,const unsigned* v){assert(live.erase(*v)==1);}
void glGenFramebuffers(int n,unsigned* p){gen(n,p);} void glGenTextures(int n,unsigned* p){gen(n,p);} void glGenRenderbuffers(int n,unsigned* p){gen(n,p);}
void glDeleteFramebuffers(int n,const unsigned* p){del(n,p);} void glDeleteTextures(int n,const unsigned* p){del(n,p);} void glDeleteRenderbuffers(int n,const unsigned* p){del(n,p);}
void glBindFramebuffer(int target,int id){if(target!=GL_DRAW_FRAMEBUFFER)readfb=id;if(target!=GL_READ_FRAMEBUFFER)drawfb=id;} void glBindTexture(int,int id){tex=id;} void glBindRenderbuffer(int,int id){rb=id;}
void glRenderbufferStorageMultisample(int,int,int,int,int){} void glFramebufferRenderbuffer(int,int,int,int){} void glTexImage2D(int,int,int,int,int,int,int,int,const void*){assert(unpack_buffer==0);} void glTexParameteri(int,int,int){} void glTexParameterfv(int,int,const float*){} void glFramebufferTexture2D(int,int,int,int,int){} int glCheckFramebufferStatus(int){return complete?GL_FRAMEBUFFER_COMPLETE:0;} void CheckGLError(const char*){}
'''
main = r'''
int main(){
 for(int samples: {0, 2}) for(bool shared: {false, true}) for(bool ok: {false, true}) {
  generation=0; complete=ok; FramebufferData fb{}; unsigned external=shared?99:0;
  bool result=CreateFramebuffer(fb, 40, 30, samples, FramebufferAttachment::DepthStencil, external);
  assert(result==ok); assert(readfb==10 && drawfb==11 && tex==12 && rb==13 && unpack_buffer==17);
  if(ok){assert(fb.framebuffer && fb.width==40 && fb.height==30); DestroyFramebuffer(fb);} else assert(!fb.framebuffer);
  assert(live.empty());
 }
 for(int samples: {0, 2}) for(int failure: {1, 2, 3}) {
  generation=0; fail_at=failure; complete=true; FramebufferData fb{};
  assert(!CreateFramebuffer(fb, 40, 30, samples, FramebufferAttachment::DepthStencil, 0));
  assert(!fb.framebuffer && live.empty());
  assert(readfb==10 && drawfb==11 && tex==12 && rb==13);
  generation=0; fail_at=0;
  assert(CreateFramebuffer(fb, 40, 30, samples, FramebufferAttachment::DepthStencil, 0));
  DestroyFramebuffer(fb); assert(live.empty());
 }
}
'''
stack = source[source.index('RenderInterface_GL3::RenderLayerStack::RenderLayerStack()'):source.index('const Rml::Matrix4f& RenderInterface_GL3::GetTransform()')]
ensure = source[source.index('bool RenderInterface_GL3::EnsurePostprocess(int mask)'):source.index('void RenderInterface_GL3::BlitLayerToPostprocessPrimary') ]
blit = source[source.index('void RenderInterface_GL3::BlitLayerToPostprocessPrimary'):source.index('void RenderInterface_GL3::RenderFilters') ]
public_push = source[source.index('Rml::LayerHandle RenderInterface_GL3::PushLayer()'):source.index('void RenderInterface_GL3::CompositeLayers(')]
public_composite = source[source.index('void RenderInterface_GL3::CompositeLayers('):source.index('void RenderInterface_GL3::PopLayer()')]
public_generate = source[source.index('Rml::TextureHandle RenderInterface_GL3::GenerateTexture('):source.index('void RenderInterface_GL3::DrawFullscreenQuad()')]
public_pop = source[source.index('void RenderInterface_GL3::PopLayer()'):source.index('Rml::TextureHandle RenderInterface_GL3::SaveLayerAsTexture()')]
frame_methods = source[source.index('void RenderInterface_GL3::BeginFrame()'):source.index('void RenderInterface_GL3::Clear()')]
save = source[source.index('Rml::TextureHandle RenderInterface_GL3::SaveLayerAsTexture()'):source.index('Rml::CompiledFilterHandle RenderInterface_GL3::SaveLayerAsMaskImage()')]
backend_source = (Path(__file__).resolve().parents[1] / 'RmlUi_BackwardCompatible/RmlUi_Backend_BackwardCompatible_GLFW_GL3.cpp').read_text()
backend_begin = backend_source[backend_source.index('bool Backend::BeginFrame()'):backend_source.index('void Backend::PresentFrame()')]
public_clear = source[source.index('void RenderInterface_GL3::Clear()'):source.index('Rml::CompiledGeometryHandle RenderInterface_GL3::CompileGeometry(')]
stack_preamble = r'''
#include <vector>
#include <utility>
#include <cstddef>
#include <bitset>
#define RMLUI_ASSERT(x) assert(x)
#define RMLUI_ASSERTMSG(x, m) assert(x)
#define RMLUI_NUM_MSAA_SAMPLES 2
#define GL_COLOR_BUFFER_BIT 123
#define GL_NEAREST 124
#define GL_TRUE 1
namespace Rml { using LayerHandle = unsigned; using TextureHandle = unsigned; using CompiledFilterHandle = unsigned; enum class BlendMode { Replace, Blend }; using byte = unsigned char; struct Vector2i { int x,y; }; struct Rectanglei { static Rectanglei MakeInvalid(){return {};} bool Valid() const{return true;} Vector2i Size() const{return {10,10};} int Left() const{return 0;} int Right() const{return 10;} int Top() const{return 0;} int Bottom() const{return 10;} int Width() const{return 10;} int Height() const{return 10;} }; template<class T> struct Span { const T* p; size_t n; const T* data() const {return p;} size_t size() const {return n;} }; template<class T> using Vector = std::vector<T>; }
static GLuint CreateTexture(Rml::Span<const Rml::byte>, Rml::Vector2i);
namespace Gfx { using ::CreateTexture; using ::FramebufferData; using ::FramebufferAttachment; using ::CreateFramebuffer; using ::DestroyFramebuffer; void CheckGLError(const char*) {} void BindTexture(const FramebufferData&) {} }
enum { GL_CULL_FACE=200, GL_STENCIL_TEST, GL_SCISSOR_TEST, GL_DEPTH_TEST, GL_CURRENT_PROGRAM, GL_VERTEX_ARRAY_BINDING, GL_ARRAY_BUFFER_BINDING, GL_BLEND_COLOR, GL_ACTIVE_TEXTURE, GL_TEXTURE0, GL_TEXTURE1, GL_VIEWPORT, GL_SCISSOR_BOX, GL_STENCIL_CLEAR_VALUE, GL_COLOR_CLEAR_VALUE, GL_COLOR_WRITEMASK, GL_BLEND_EQUATION_RGB, GL_BLEND_EQUATION_ALPHA, GL_BLEND_SRC_RGB, GL_BLEND_DST_RGB, GL_BLEND_SRC_ALPHA, GL_BLEND_DST_ALPHA, GL_STENCIL_FUNC, GL_STENCIL_REF, GL_STENCIL_VALUE_MASK, GL_STENCIL_WRITEMASK, GL_STENCIL_FAIL, GL_STENCIL_PASS_DEPTH_FAIL, GL_STENCIL_PASS_DEPTH_PASS, GL_STENCIL_BACK_FUNC, GL_STENCIL_BACK_REF, GL_STENCIL_BACK_VALUE_MASK, GL_STENCIL_BACK_WRITEMASK, GL_STENCIL_BACK_FAIL, GL_STENCIL_BACK_PASS_DEPTH_FAIL, GL_STENCIL_BACK_PASS_DEPTH_PASS, GL_FUNC_ADD, GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ALWAYS, GL_KEEP, GL_ARRAY_BUFFER, GL_FRONT, GL_BACK };
unsigned char color_mask[4]={1,1,1,1}, last_draw_mask[4]={}; bool check_clear_mask=false;
int glIsEnabled(int){return 0;} void glGetFloatv(int,float* p){for(int i=0;i<4;i++)p[i]=0;} void glGetBooleanv(int,unsigned char* p){for(int i=0;i<4;i++)p[i]=color_mask[i];}
void glActiveTexture(int){} void glViewport(int,int,int,int){} void glScissor(int,int,int,int){} void glClearStencil(int){} void glBlendEquation(int){} void glBlendFunc(int,int){} void glStencilFunc(int,int,unsigned){} void glStencilMask(unsigned){} void glStencilOp(int,int,int){} void glUseProgram(int){} void glBindVertexArray(int){} void glBlendColor(float,float,float,float){} void glColorMask(int r,int g,int b,int a){color_mask[0]=r; color_mask[1]=g; color_mask[2]=b; color_mask[3]=a;} void glBlendEquationSeparate(int,int){} void glBlendFuncSeparate(int,int,int,int){} void glStencilFuncSeparate(int,int,int,int){} void glStencilMaskSeparate(int,int){} void glStencilOpSeparate(int,int,int,int){} void glCopyTexSubImage2D(int,int,int,int,int,int,int,int){}
int clears=0, draws=0; std::vector<int> clear_targets; void glBlitFramebuffer(int,int,int,int,int,int,int,int,int,int) { assert(readfb && drawfb); ++draws; } bool allow_host_clear=false; void glDisable(int) {} void glEnable(int) {} void glClearColor(float,float,float,float) {} void glClear(int) { if(check_clear_mask) for(auto channel : color_mask) assert(channel); assert(allow_host_clear || (drawfb != 0 && drawfb != 11)); clear_targets.push_back(drawfb); ++clears; }
class RenderInterface_GL3 { public:
 class RenderLayerStack { public:
 RenderLayerStack(); ~RenderLayerStack(); Rml::LayerHandle PushLayer(); void PopLayer();
 const Gfx::FramebufferData& GetLayer(Rml::LayerHandle) const; const Gfx::FramebufferData& GetTopLayer() const;
 Rml::LayerHandle GetTopLayerHandle() const; const Gfx::FramebufferData& GetPostprocessSecondary() { return EnsureFramebufferPostprocess(1); } const Gfx::FramebufferData& GetPostprocessPrimary() { return EnsureFramebufferPostprocess(0); } void SwapPostprocessPrimarySecondary();
 bool EnsurePostprocess(int); void BeginFrame(int,int); void EndFrame(); bool IsValid() const {return valid;} bool HasLayer(Rml::LayerHandle h) const {return h < (unsigned)layers_size;} bool CanPopLayer() const {return layers_size > 1;} bool IsBalanced() const {return layers_size == 1;}
 private: void DestroyFramebuffers(); const Gfx::FramebufferData& EnsureFramebufferPostprocess(int);
 bool valid=true; int width=0,height=0,layers_size=0; Rml::Vector<Gfx::FramebufferData> fb_layers,fb_postprocess;
 } render_layers;
 bool frame_failed=false, frame_active=false; int viewport_width=40,viewport_height=30,viewport_offset_x=0,viewport_offset_y=0;
 struct Stencil { int func,ref,value_mask,writemask,fail,pass_depth_fail,pass_depth_pass; };
 struct Backup { int enable_cull_face,enable_blend,enable_stencil_test,enable_scissor_test,enable_depth_test; int read_framebuffer,draw_framebuffer,renderbuffer,program,vertex_array,array_buffer,active_texture,texture0,texture1,viewport[4],scissor[4],stencil_clear_value,blend_equation_rgb,blend_equation_alpha,blend_src_rgb,blend_dst_rgb,blend_src_alpha,blend_dst_alpha; float blend_color[4],color_clear_value[4]; unsigned char color_writemask[4]; Stencil stencil_front,stencil_back; } glstate_backup{};
 std::bitset<1> program_transform_dirty; Rml::Rectanglei scissor_state; void SetTransform(void*){} void SetScissor(Rml::Rectanglei){} void EnableScissorRegion(bool){}
 void BeginFrame(); void EndFrame(); Rml::TextureHandle SaveLayerAsTexture();
 enum class ProgramId { None, Passthrough };
 Rml::LayerHandle PushLayer(); void PopLayer(); void Clear();
 Rml::TextureHandle GenerateTexture(Rml::Span<const Rml::byte>, Rml::Vector2i);
 void CompositeLayers(Rml::LayerHandle,Rml::LayerHandle,Rml::BlendMode,Rml::Span<const Rml::CompiledFilterHandle>);
 bool EnsurePostprocess(int); void BlitLayerToPostprocessPrimary(Rml::LayerHandle); void RenderFilters(Rml::Span<const Rml::CompiledFilterHandle>) {} void UseProgram(ProgramId) {} void DrawFullscreenQuad() { ++draws; for (int i=0; i<4; ++i) last_draw_mask[i]=color_mask[i]; }
};
'''
backend_mock = r'''
namespace Backend { bool BeginFrame(); }
struct BackendData { RenderInterface_GL3 render_interface; }; BackendData* data;
'''
stack_main = r'''
int main() {
 // Host color write mask must not restrict layer clears and must be restored.
 { RenderInterface_GL3 renderer; complete=true; fail_at=0; color_mask[0]=0; color_mask[2]=0;
   check_clear_mask=true; renderer.BeginFrame(); check_clear_mask=false;
   renderer.EndFrame(); assert(!last_draw_mask[0] && last_draw_mask[1] && !last_draw_mask[2] && last_draw_mask[3]);
   assert(!color_mask[0] && color_mask[1] && !color_mask[2] && color_mask[3]);
   for(auto& channel : color_mask) channel=1;
 } assert(live.empty());
 for (int failure : {1,4}) {
  { RenderInterface_GL3 renderer; complete=true; generation=0; fail_at=failure;
    int outside_clears=clears, outside_draws=draws; allow_host_clear=true; renderer.Clear(); allow_host_clear=false; renderer.PushLayer(); renderer.PopLayer(); renderer.CompositeLayers(0,0,Rml::BlendMode::Blend,{}); assert(clears==outside_clears+1 && draws==outside_draws);
    renderer.BeginFrame();
    if (failure==1) assert(renderer.frame_failed);
    else {
      assert(!renderer.frame_failed);
      int before=draws;
      renderer.CompositeLayers(0,0,Rml::BlendMode::Blend,{});
      if (failure==4) { assert(renderer.frame_failed && draws==before); }
      else { assert(!renderer.frame_failed && draws>before); }
    }
    int before_end=draws; renderer.EndFrame();
    assert(draws==before_end && readfb==10 && drawfb==11 && rb==13);
    fail_at=0; generation=0; renderer.BeginFrame(); assert(!renderer.frame_failed);
    int before=draws; renderer.CompositeLayers(0,0,Rml::BlendMode::Blend,{}); assert(!renderer.frame_failed && draws>before);
    renderer.EndFrame();
  }
  assert(live.empty()); readfb=10; drawfb=11; tex=12; rb=13;
 }
 // Failure while allocating a later layer or EndFrame's postprocess must skip presentation.
 for (bool later_layer : {false, true}) {
  { RenderInterface_GL3 renderer; complete=true; fail_at=0; renderer.BeginFrame();
    int before=draws; fail_at=generation+1;
    if (later_layer) { renderer.PushLayer(); assert(renderer.frame_failed); }
    renderer.EndFrame(); assert(draws==before && readfb==10 && drawfb==11 && rb==13 && !renderer.frame_active);
    fail_at=0; renderer.BeginFrame(); assert(!renderer.frame_failed); renderer.EndFrame(); assert(draws>before);
    assert(readfb==10 && drawfb==11 && rb==13);
  } assert(live.empty());
 }
 // Backend clear must target host before the renderer binds its first layer.
 { BackendData backend; data=&backend; complete=true; fail_at=0; readfb=10; drawfb=11; tex=12; rb=13;
   int before=clears; allow_host_clear=true; assert(Backend::BeginFrame()); allow_host_clear=false;
   assert(clears==before+2 && clear_targets[clear_targets.size()-2]==11 && clear_targets.back()!=11 && drawfb!=11); backend.render_interface.EndFrame(); assert(readfb==10 && drawfb==11 && rb==13);
 } assert(live.empty());
 // Both postprocess buffers exist before forcing the saved texture allocation to fail.
 { RenderInterface_GL3 renderer; complete=true; fail_at=0; renderer.BeginFrame(); assert(renderer.EnsurePostprocess(3));
   int before=draws; fail_at=generation+1; assert(!renderer.SaveLayerAsTexture()); assert(renderer.frame_failed && draws==before);
   renderer.EndFrame(); assert(draws==before && readfb==10 && drawfb==11 && rb==13);
   fail_at=0; renderer.BeginFrame(); assert(!renderer.frame_failed); renderer.EndFrame(); assert(draws>before);
 } assert(live.empty());
 // Public texture creation rejects undersized pixel spans before calling GL.
 { RenderInterface_GL3 renderer; unsigned char pixels[4]{}; int old_generation=generation;
   assert(!renderer.GenerateTexture({pixels,3}, {1,1})); assert(generation==old_generation);
 } assert(live.empty());
 for (int index : {0,1,2,3}) {
  { RenderInterface_GL3 renderer; complete=true; generation=0; fail_at=0;
    renderer.BeginFrame();
    fail_at=generation+1; int before=draws;
    assert(!renderer.EnsurePostprocess(1<<index)); assert(renderer.frame_failed && draws==before);
    renderer.EndFrame(); fail_at=0;
    renderer.BeginFrame();
    assert(renderer.EnsurePostprocess(1<<index)); renderer.EndFrame();
  }
  assert(live.empty()); readfb=10; drawfb=11; tex=12; rb=13;
 }
}
'''
with tempfile.TemporaryDirectory() as tmp:
    cpp = Path(tmp) / 'test.cpp'
    cpp.write_text(preamble + helper + main)
    exe = Path(tmp) / 'test'
    compiler = os.environ.get('CXX', 'c++')
    subprocess.run([compiler, '-std=c++17', str(cpp), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
    cpp.write_text('#define RMLUI_PLATFORM_EMSCRIPTEN\n' + preamble + helper + stack_preamble + texture_helper + stack + ensure + blit + public_push + public_composite + public_pop + public_clear + public_generate + frame_methods + save + backend_mock + backend_begin + stack_main)
    subprocess.run([compiler, '-std=c++17', str(cpp), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
print('framebuffer mock passed')
