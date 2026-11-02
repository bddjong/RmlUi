#!/usr/bin/env python3
"""Exercise actual gradient stop and repeat shader code on the CPU, without a GPU."""
import pathlib
import os
import re
import subprocess
import tempfile

backends = pathlib.Path(__file__).resolve().parents[1]
sources = {
    'GL3': (backends / 'RmlUi_Renderer_GL3.cpp', 'mix_stop_colors'),
    'DX11': (backends / 'RmlUi_Renderer_DX11.cpp', 'lerp_stop_colors'),
    'DX12': (backends / 'RmlUi_Renderer_DX12.cpp', 'lerp_stop_colors'),
    'Vulkan': (backends / 'RmlUi_Vulkan/shader_frag_gradient.frag', 'lerp_stop_colors'),
}

for name, (path, function) in sources.items():
    source = path.read_text()
    match = re.search(r'(?:vec4|float4) ' + function + r'\(float t\) \{.*?\n\}', source, re.S)
    assert match, name
    shader = match.group().replace('vec4', 'float').replace('float4', 'float')
    average = re.search(r'(?:vec4|float4) average_stop_colors\(\) \{.*?\n\}', source, re.S)
    assert average, name
    average_code = average.group().replace('vec4', 'float').replace('float4', 'float')
    assert 'if (t1 <= t0)' in source and 'average_stop_colors()' in source
    assert ('float t1 = _repeat_end;' if name == 'GL3' else 'float t1 = m_repeat_end.x;') in source
    code = r'''
#include <algorithm>
#include <cassert>
#include <cmath>
float POS[16], COLORS[16]; int COUNT; float REPEAT_END = NAN;
#define _stop_positions POS
#define _stop_colors COLORS
#define _num_stops COUNT
#define m_stop_colors COLORS
#define m_num_stops COUNT
#define GET_STOP_POS(i) POS[i]
float step(float edge, float t) { return t < edge ? 0.f : 1.f; }
float clamp(float x, float a, float b) { return std::clamp(x, a, b); }
float mix(float a, float b, float x) { return a*(1-x)+b*x; }
float lerp(float a, float b, float x) { return mix(a,b,x); }
float mod(float x, float y) { assert(y > 0); return x-y*std::floor(x/y); }
''' + shader + '\n' + average_code + r'''
void check(float t, bool repeating, float expected) {
    float result;
    const float end = std::isnan(REPEAT_END) ? POS[COUNT-1] : REPEAT_END;
    if (repeating && end <= POS[0]) result = average_stop_colors();
    else { if (repeating) t = POS[0] + mod(t-POS[0], end-POS[0]); result = FUNCTION(t); }
    assert(std::isfinite(result) && std::abs(result-expected) < 0.0001f);
}
void rgba(float t, bool repeating, const float stops[][4], const float expected[4]) {
    for (int channel=0; channel<4; ++channel) {
        for (int i=0; i<COUNT; ++i) COLORS[i]=stops[i][channel];
        check(t, repeating, expected[channel]);
    }
}
int main() {
    // Premultiplied red, transparent, and premultiplied blue.
    const float colors[][4] = {{0.5f,0,0,0.5f}, {0,0,0,0}, {0,0,0.25f,0.25f}};
    COUNT=2; POS[0]=0; POS[1]=1;
    const float quarter[] = {0.375f,0,0,0.375f};
    const float three_quarter[] = {0.125f,0,0,0.125f};
    rgba(0.25f, false, colors, quarter); rgba(0.75f, false, colors, three_quarter);
    rgba(1.25f, true, colors, quarter); rgba(-0.25f, true, colors, three_quarter);
    rgba(1.f, true, colors, colors[0]);
    COUNT=3; POS[0]=0; POS[1]=0.5f; POS[2]=1;
    const float red_half[] = {0.25f,0,0,0.25f};
    const float blue_quarter[] = {0,0,0.0625f,0.0625f};
    rgba(0.25f, false, colors, red_half); rgba(0.625f, false, colors, blue_quarter);
    POS[1]=POS[2]=0.5f;
    rgba(0.5f, false, colors, colors[2]); rgba(0.5f, true, colors, colors[0]);

    COUNT=2; POS[0]=POS[1]=0.5f; COLORS[0]=0; COLORS[1]=1;
    check(0, false, 0); check(0.5f, false, 1); check(1, false, 1);
    check(-10, true, 0.5f); check(0.5f, true, 0.5f); check(10, true, 0.5f);
    COUNT=3; POS[0]=POS[1]=POS[2]=0.5f; COLORS[0]=0; COLORS[1]=0.5f; COLORS[2]=1;
    check(0, false, 0); check(0.5f, false, 1); check(-1, true, 0.5f);
    const float average_rgba[] = {0.125f,0,0.0625f,0.1875f};
    rgba(-1, true, colors, average_rgba);
    COUNT=4; POS[0]=POS[1]=POS[2]=POS[3]=0; COLORS[0]=0; COLORS[1]=0; COLORS[2]=0; COLORS[3]=1;
    check(3, true, 1.f/6.f);
    COUNT=1; COLORS[0]=0.3f; check(2, true, 0.3f);
    COUNT=3; COLORS[0]=0; COLORS[1]=0.5f; COLORS[2]=1;
    POS[0]=0; POS[1]=POS[2]=1;
    check(0.5f, false, 0.25f); check(1, false, 1); check(1, true, 0); check(-0.5f, true, 0.25f);
    POS[0]=POS[1]=0; POS[2]=1;
    check(-1, false, 0); check(0, false, 0.5f); check(0.5f, true, 0.75f);
    // The full repeat endpoint can lie beyond the final packed stop.
    COUNT=16; for (int i=0; i<15; ++i) { POS[i]=0; COLORS[i]=0; }
    POS[15]=0.5f; COLORS[15]=1; REPEAT_END=1;
    check(0.75f, true, 1); check(1.25f, true, 0.5f);
}
'''.replace('FUNCTION', function)
    with tempfile.TemporaryDirectory(prefix='rml-gradient-') as tmp:
        cpp = pathlib.Path(tmp) / 'test.cpp'
        cpp.write_text(code)
        subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O0', str(cpp), '-o', str(cpp.with_suffix(''))], check=True)
        subprocess.run([str(cpp.with_suffix(''))], check=True)
    print(name + ': passed')

# Compile and execute each backend's actual packing lambda with minimal Rml types.
packing_stub = r"""
#include <algorithm>
#include <cassert>
#include <cmath>
#include <map>
#include <string>
#include <vector>
#define RMLUI_ASSERT assert
constexpr int MAX_NUM_STOPS = 16;
namespace Rml {
struct Colourf { float r=0, a=0; Colourf operator*(float x) const { return {r*x,a*x}; } Colourf& operator+=(Colourf x) { r+=x.r; a+=x.a; return *this; } Colourf& operator*=(float x) { r*=x; a*=x; return *this; } };
enum class Unit { NUMBER };
struct ColorStop { struct { float number; Unit unit; } position; Colourf color; };
using ColorStopList = std::vector<ColorStop>;
struct Variant { enum { COLORSTOPLIST }; ColorStopList list; int GetType() const { return COLORSTOPLIST; } template<class T> const T& GetReference() const { return list; } };
using Dictionary = std::map<std::string, Variant>;
bool repeating = true;
bool Get(const Dictionary&, const char*, bool) { return repeating; }
namespace Math { int Min(int a, int b) { return std::min(a,b); } }
}
Rml::Colourf ConvertToColorf(Rml::Colourf c) { return c; }
struct CompiledShader { float repeat_end; std::vector<float> stop_positions; std::vector<Rml::Colourf> stop_colors; };
int main() {
"""
for name in sources:
    path = backends / ('RmlUi_Renderer_' + ('VK' if name == 'Vulkan' else name) + '.cpp')
    source = path.read_text()
    match = re.search(r'auto ApplyColorStopList = \[\]\(CompiledShader& shader, const Rml::Dictionary& shader_parameters\) \{.*?\n\t\};', source, re.S)
    assert match, name
    shader_source, function = sources[name]
    color_function = re.search(r'(?:vec4|float4) ' + function + r'\(float t\) \{.*?\n\}', shader_source.read_text(), re.S)
    assert color_function, name
    scalar_shader = color_function.group().replace('vec4', 'float').replace('float4', 'float')
    shader_stub = r"""
float POS[16], COLORS[16]; int COUNT;
#define _stop_positions POS
#define _stop_colors COLORS
#define _num_stops COUNT
#define m_stop_colors COLORS
#define m_num_stops COUNT
#define GET_STOP_POS(i) POS[i]
float step(float edge, float t) { return t < edge ? 0.f : 1.f; }
float clamp(float x, float a, float b) { return std::clamp(x, a, b); }
float mix(float a, float b, float x) { return a*(1-x)+b*x; }
float lerp(float a, float b, float x) { return mix(a,b,x); }
""" + scalar_shader + '\n'
    code = packing_stub.replace('int main() {', shader_stub + 'int main() {') + match.group() + r"""
    Rml::Dictionary params; auto& stops=params["color_stop_list"].list;
    for (int i=0; i<15; ++i) stops.push_back({{0, Rml::Unit::NUMBER}, {0,1}});
    stops.push_back({{0.5f, Rml::Unit::NUMBER}, {1,1}});
    stops.push_back({{1, Rml::Unit::NUMBER}, {0,1}});
    CompiledShader shader{}; ApplyColorStopList(shader, params);
    assert(shader.stop_positions.size()==16 && shader.stop_positions[15]==0.5f);
    assert(shader.stop_colors[15].r==1 && shader.repeat_end==1);
    COUNT = (int)shader.stop_positions.size();
    for (int i=0; i<COUNT; ++i) { POS[i]=shader.stop_positions[i]; COLORS[i]=shader.stop_colors[i].r; }
    float t = 0.75f;
    t = POS[0] + (t-POS[0] - (shader.repeat_end-POS[0])*std::floor((t-POS[0])/(shader.repeat_end-POS[0])));
    assert(std::abs(FUNCTION(t)-1.f)<0.00001f);
    for (auto& stop: stops) { stop.position.number=0; stop.color={0,0}; }
    stops.back().color={0.5f, 0.5f};
    ApplyColorStopList(shader, params);
    assert(shader.stop_colors.size()==2 && std::abs(shader.stop_colors[0].a-0.5f/32.f)<0.00001f);
    assert(shader.stop_colors[0].r==shader.stop_colors[0].a && shader.repeat_end==0);
    Rml::repeating = false;
    ApplyColorStopList(shader, params);
    assert(shader.stop_positions.size()==16 && shader.stop_colors.size()==16);
}
""".replace('FUNCTION', function)
    with tempfile.TemporaryDirectory(prefix='rml-gradient-pack-') as tmp:
        cpp = pathlib.Path(tmp) / 'test.cpp'
        cpp.write_text(code)
        subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O0', str(cpp), '-o', str(cpp.with_suffix(''))], check=True)
        subprocess.run([str(cpp.with_suffix(''))], check=True)
    print(name + ' packing: passed')
