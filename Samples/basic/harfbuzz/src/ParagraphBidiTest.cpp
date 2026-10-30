#include "FontEngineInterfaceHarfBuzz.h"
#include <RmlUi/Core.h>
#include <RmlUi/Core/ElementText.h>
#include <cmath>
#include <cstdio>

// Headless smoke test for the sample's actual shaping engine and core layout.
class TestRenderer final : public Rml::RenderInterface {
public:
	Rml::CompiledGeometryHandle CompileGeometry(Rml::Span<const Rml::Vertex> vertices, Rml::Span<const int>) override {
		vertex_count += vertices.size();
		return ++next_handle;
	}
	void RenderGeometry(Rml::CompiledGeometryHandle, Rml::Vector2f, Rml::TextureHandle) override { ++draw_count; }
	void ReleaseGeometry(Rml::CompiledGeometryHandle) override {}
	Rml::TextureHandle LoadTexture(Rml::Vector2i&, const Rml::String&) override { return 0; }
	Rml::TextureHandle GenerateTexture(Rml::Span<const Rml::byte>, Rml::Vector2i) override { return ++next_handle; }
	void ReleaseTexture(Rml::TextureHandle) override {}
	void EnableScissorRegion(bool) override {}
	void SetScissorRegion(Rml::Rectanglei) override {}
	size_t vertex_count = 0, draw_count = 0;
	uintptr_t next_handle = 1;
};

int main(int argc, char** argv)
{
	if (argc != 2) return 1;
	TestRenderer renderer;
	FontEngineInterfaceHarfBuzz font;
	font.RegisterLanguage("ar", "Arab", TextFlowDirection::RightToLeft);
	font.RegisterLanguage("he", "Hebr", TextFlowDirection::RightToLeft);
	Rml::SetRenderInterface(&renderer);
	Rml::SetFontEngineInterface(&font);
	if (!Rml::Initialise() || !Rml::LoadFontFace(argv[1])) return 2;
	auto* context = Rml::CreateContext("paragraph-test", {640, 480});
	if (!context) return 3;
	auto* document = context->LoadDocumentFromMemory(R"(
		<rml><head><style>body { font-family: Cairo; font-size: 32px; }
		p { display: block; width: 400px; -rmlui-direction: rtl; }</style></head><body>
		<p id='mixed'><span id='arabic'>سلام </span><span id='number'>123</span><span style='display:inline-block;width:10px;height:10px'></span></p>
		<p id='ligature'><span id='lam' style='color: red'>ل</span><span id='alef'>ا</span></p>
		</body></rml>)");
	if (!document) return 4;
	document->Show();
	context->Update();
	context->Render();
	auto* arabic = document->GetElementById("arabic");
	auto* number = document->GetElementById("number");
	auto* lam = document->GetElementById("lam");
	auto* alef = document->GetElementById("alef");
	if (!arabic || !number || !lam || !alef || !(number->GetAbsoluteOffset().x < arabic->GetAbsoluteOffset().x)) return 5;
	auto* lam_text = dynamic_cast<Rml::ElementText*>(lam->GetChild(0));
	auto* alef_text = dynamic_cast<Rml::ElementText*>(alef->GetChild(0));
	if (!lam_text || !alef_text || lam_text->GetLines().size() != 1 || alef_text->GetLines().size() != 1) return 6;
	const Rml::String lang = "ar";
	const Rml::TextShapingContext rtl{lang, Rml::Style::Direction::Rtl};
	const auto face = font.GetFontFaceHandle("cairo", Rml::Style::FontStyle::Normal, Rml::Style::FontWeight::Normal, 32);
	if (!face || std::abs(lam_text->GetLines()[0].width + alef_text->GetLines()[0].width - font.GetStringWidth(face, "لا", rtl, Rml::Character::Null)) > 2) return 7;
	const auto mixed = std::make_shared<const Rml::String>("שלום سلام");
	const size_t arabic_offset = mixed->find("سلام");
	const Rml::String hebrew_language = "he";
	Rml::TextShapingContext mixed_context{hebrew_language, Rml::Style::Direction::Rtl};
	mixed_context.paragraph_owner = mixed;
	mixed_context.paragraph_text = *mixed;
	mixed_context.paragraph_offset = arabic_offset;
	mixed_context.shaping_run_length = mixed->size();
	mixed_context.direction_resolved = true;
	if (arabic_offset == Rml::String::npos || font.GetStringWidth(face, Rml::StringView(*mixed, arabic_offset), mixed_context, Rml::Character::Null) !=
		font.GetStringWidth(face, "سلام", rtl, Rml::Character::Null)) return 9;
	if (!renderer.vertex_count || !renderer.draw_count) return 8;
	Rml::Shutdown();
	std::puts("HarfBuzz paragraph layout passed.");
	return 0;
}
