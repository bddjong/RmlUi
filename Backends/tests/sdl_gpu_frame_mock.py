#!/usr/bin/env python3
"""Exercise production SDL GPU frame lifecycle with a strict fake SDL."""
from pathlib import Path
import os
import subprocess
import tempfile

source = (Path(__file__).resolve().parents[1] / 'RmlUi_Backend_SDL_GPU.cpp').read_text()
shutdown = source[source.index('void Backend::Shutdown()'):source.index('Rml::SystemInterface* Backend::GetSystemInterface()')]
frames = source[source.index('bool Backend::BeginFrame()'):]
preamble = r'''
#include <cassert>
#include <cstdint>
#include <memory>
#define RMLUI_ASSERT(x) assert(x)
namespace Rml { namespace Log { enum {LT_ERROR}; void Message(int,const char*,...) {} } }
struct SDL_Window {}; struct SDL_GPUDevice {}; struct SDL_GPUTexture {}; struct SDL_GPUCommandBuffer {}; struct SDL_GPURenderPass {};
struct SDL_GPUColorTargetInfo { SDL_GPUTexture* texture; int load_op, store_op; };
enum { SDL_GPU_LOADOP_CLEAR, SDL_GPU_STOREOP_STORE };
SDL_Window window; SDL_GPUDevice device; SDL_GPUTexture texture; SDL_GPUCommandBuffer buffer; SDL_GPURenderPass pass;
int scenario=0, acquired=0, cancels=0, submits=0, clears=0, begins=0, ends=0, shutdowns=0; bool active=false, has_texture=false;
const char* SDL_GetError(){return "fake";}
SDL_GPUCommandBuffer* SDL_AcquireGPUCommandBuffer(SDL_GPUDevice*) { assert(!active); if(scenario==1) return nullptr; active=true; has_texture=false; return &buffer; }
bool SDL_WaitAndAcquireGPUSwapchainTexture(SDL_GPUCommandBuffer*, SDL_Window*, SDL_GPUTexture** t, uint32_t* w, uint32_t* h) {
 assert(active); if(scenario==2) return false; *t=scenario==3?nullptr:&texture; has_texture=*t; *w=scenario==4?0:640; *h=480; if(has_texture) ++acquired; return true;
}
bool SDL_CancelGPUCommandBuffer(SDL_GPUCommandBuffer*) { assert(active && !has_texture); active=false; ++cancels; return true; }
bool SDL_SubmitGPUCommandBuffer(SDL_GPUCommandBuffer*) { assert(active); active=false; ++submits; return true; }
SDL_GPURenderPass* SDL_BeginGPURenderPass(SDL_GPUCommandBuffer*, SDL_GPUColorTargetInfo* info, int, void*) { assert(active && has_texture && info->texture==&texture); ++clears; return &pass; }
void SDL_EndGPURenderPass(SDL_GPURenderPass* p) { assert(p==&pass); }
struct Renderer { void BeginFrame(SDL_GPUCommandBuffer*,SDL_GPUTexture*,uint32_t w,uint32_t h){assert(w && h); ++begins;} void EndFrame(){++ends;} void Shutdown(){assert(!active); ++shutdowns;} };
struct BackendData { SDL_Window* window=&::window; SDL_GPUDevice* device=&::device; SDL_GPUCommandBuffer* command_buffer=nullptr; Renderer render_interface; };
std::unique_ptr<BackendData> data;
void SDL_ReleaseWindowFromGPUDevice(SDL_GPUDevice*,SDL_Window*) {assert(!active);}
void SDL_DestroyGPUDevice(SDL_GPUDevice*) {} void SDL_DestroyWindow(SDL_Window*) {} void SDL_Quit() {}
namespace Backend { bool BeginFrame(); void PresentFrame(); void Shutdown(); }
'''
main = r'''
int main() {
 data.reset(new BackendData);
 scenario=1; assert(!Backend::BeginFrame() && !active && !data->command_buffer);
 scenario=2; assert(!Backend::BeginFrame() && !active && !data->command_buffer && cancels==1);
 scenario=3; assert(!Backend::BeginFrame() && !active && !data->command_buffer && cancels==2);
 scenario=4; assert(!Backend::BeginFrame() && !active && !data->command_buffer && submits==1 && clears==0);
 scenario=0; assert(Backend::BeginFrame()); Backend::PresentFrame();
 assert(!active && !data->command_buffer && begins==1 && ends==1 && clears==1 && submits==2);
 scenario=3; assert(!Backend::BeginFrame() && !data->command_buffer);
 scenario=0; assert(Backend::BeginFrame()); Backend::Shutdown();
 assert(!active && !data && begins==2 && ends==2 && submits==3 && shutdowns==1 && acquired==3);
 data.reset(new BackendData); scenario=3; assert(!Backend::BeginFrame()); Backend::Shutdown(); assert(!active && shutdowns==2);
}
'''
with tempfile.TemporaryDirectory() as tmp:
    cpp = Path(tmp) / 'test.cpp'
    exe = Path(tmp) / 'test'
    cpp.write_text(preamble + shutdown + frames + main)
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-Wall', '-Wextra', '-Werror', str(cpp), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
