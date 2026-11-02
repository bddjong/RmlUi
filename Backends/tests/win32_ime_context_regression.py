#!/usr/bin/env python3
"""Compile the Win32 IME state machine with mock text contexts on any platform."""
from pathlib import Path
import subprocess
import tempfile

backend = Path(__file__).resolve().parents[1]
header = (backend / 'RmlUi_Platform_Win32.h').read_text()
source = (backend / 'RmlUi_Platform_Win32.cpp').read_text()
klass = header[header.index('class TextInputMethodEditor_Win32 final'):]
methods = source[source.index('TextInputMethodEditor_Win32::TextInputMethodEditor_Win32()'):]
preamble = r'''
#include <cassert>
#include <string>
#include <string_view>
#define RMLUI_ASSERT(x) assert(x)
namespace Rml {
using StringView = std::string_view;
struct TextInputContext {
 virtual ~TextInputContext() = default;
 virtual void GetSelectionRange(int&, int&) const = 0;
 virtual void SetSelectionRange(int, int) = 0;
 virtual void SetCursorPosition(int) = 0;
 virtual void SetText(StringView, int, int) = 0;
 virtual void SetCompositionRange(int, int) = 0;
 virtual void CommitComposition(StringView) = 0;
};
struct TextInputHandler {
 virtual void OnActivate(TextInputContext*) {}
 virtual void OnDeactivate(TextInputContext*) {}
 virtual void OnDestroy(TextInputContext*) {}
};
struct StringUtilities { static size_t LengthUTF8(StringView s) { return s.size(); } };
}
'''
test = r'''
struct Mock : Rml::TextInputContext {
 std::string text; int start, end, cursor = -1, highlight = 0, calls = 0;
 Mock(const char* s, int a, int b) : text(s), start(a), end(b) {}
 void GetSelectionRange(int& a, int& b) const override { a = start; b = end; }
 void SetSelectionRange(int a, int b) override { start = a; end = b; ++calls; }
 void SetCursorPosition(int p) override { cursor = p; start = end = p; ++calls; }
 void SetText(Rml::StringView s, int a, int b) override { assert(a >= 0 && b >= a && b <= (int)text.size()); text.replace(a, b-a, s); ++calls; }
 void SetCompositionRange(int a, int b) override { highlight = b-a; ++calls; }
 void CommitComposition(Rml::StringView) override { ++calls; }
};
int main() {
 TextInputMethodEditor_Win32 ime; Mock a("abcdef", 2, 4), b("123456789", 7, 7);
 ime.OnActivate(&a); ime.StartComposition(); ime.SetCursorPosition(1, false); ime.SetComposition("XY");
 assert(a.text == "abXYef" && a.highlight == 2);
 ime.OnActivate(&b); assert(!ime.IsComposing() && a.text == "abXYef" && a.highlight == 0);
 ime.StartComposition(); ime.SetComposition("Z"); assert(b.text == "1234567Z89");
 ime.CancelComposition(); assert(b.text == "123456789" && b.cursor == 7);
 ime.StartComposition(); b.cursor = -1; ime.CancelComposition(); assert(b.cursor == -1);
 ime.StartComposition(); ime.SetComposition("Q"); ime.OnDeactivate(&b); assert(!ime.IsComposing() && b.highlight == 0);
 ime.OnActivate(&a); ime.StartComposition(); ime.SetComposition("M"); int calls = a.calls;
 ime.OnDestroy(&a); assert(a.calls == calls && !ime.IsComposing());
 ime.OnActivate(&b); ime.StartComposition(); ime.SetComposition("R"); assert(b.text == "1234567R89");
 ime.ConfirmComposition("S"); assert(b.text == "1234567S89" && !ime.IsComposing());
 Mock zero("abc", 0, 0); ime.OnActivate(&zero); ime.StartComposition(); ime.SetComposition("");
 zero.start = zero.end = 2; ime.SetComposition("X"); assert(zero.text == "Xabc");
}
'''
with tempfile.TemporaryDirectory() as tmp:
    cpp = Path(tmp) / 'test.cpp'
    exe = Path(tmp) / 'test'
    cpp.write_text(preamble + klass + '\n' + methods + '\n' + test)
    subprocess.run(['c++', '-std=c++17', '-Wall', '-Wextra', '-Werror', str(cpp), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
print('Win32 IME context regression passed')
