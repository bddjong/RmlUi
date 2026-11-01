#include "FontEngineInterfaceHarfBuzz.h"
#include "FontFaceHandleHarfBuzz.h"
#include "FontProvider.h"
#include <RmlUi/Core.h>
#include <RmlUi/Core/TextShapingContext.h>
#include <SheenBidi/SheenBidi.h>
#include <algorithm>
#include <hb.h>
#include <iterator>

// Resolve bidi on each complete rendered line, then shape each visual run in
// its logical order. The same run boundaries are used for measuring and drawing.
template <typename F>
static int ForVisualRuns(StringView text, const TextShapingContext& context, F&& visit)
{
	if (text.empty()) return 0;
	// Layout has already resolved the paragraph and selected this visual run.
	// Resolving it again in isolation would lose embedding and isolate context.
	if (context.direction_resolved) return visit(text, context, 0);
	SBCodepointSequence sequence{SBStringEncodingUTF8, text.begin(), text.size()};
	SBAlgorithmRef algorithm = SBAlgorithmCreate(&sequence);
	if (!algorithm) return 0;
	SBLevel base = context.text_direction == Style::Direction::Rtl ? 1 :
		(context.text_direction == Style::Direction::Ltr ? 0 : SBLevelDefaultLTR);
	SBParagraphRef paragraph = SBAlgorithmCreateParagraph(algorithm, 0, text.size(), base);
	if (!paragraph) { SBAlgorithmRelease(algorithm); return 0; }
	SBLineRef line = SBParagraphCreateLine(paragraph, 0, SBParagraphGetLength(paragraph));
	int width = 0;
	if (line) {
		const SBRun* runs = SBLineGetRunsPtr(line);
		for (SBUInteger i = 0; i < SBLineGetRunCount(line); ++i) {
			const SBRun& run = runs[i];
			TextShapingContext run_context{context.language, (run.level & 1) ? Style::Direction::Rtl : Style::Direction::Ltr,
				context.font_kerning, context.letter_spacing};
			if (!context.paragraph_text.empty()) {
				run_context.paragraph_text = context.paragraph_text;
				run_context.paragraph_owner = context.paragraph_owner;
				run_context.paragraph_offset = context.paragraph_offset + run.offset;
				run_context.shaping_run_offset = context.shaping_run_offset;
				run_context.shaping_run_length = context.shaping_run_length;
			}
			run_context.direction_resolved = true;
			width += visit(StringView(text.begin() + run.offset, text.begin() + run.offset + run.length), run_context, width);
		}
		SBLineRelease(line);
	}
	SBParagraphRelease(paragraph);
	SBAlgorithmRelease(algorithm);
	return width;
}

// Arabic needs contextual shaping even when the surrounding UI uses a Latin
// face. The default glyph-by-glyph fallback cannot join its letters, so shape
// an Arabic bidi run with the bundled Arabic face instead.
FontFaceHandleHarfBuzz* FontEngineInterfaceHarfBuzz::FaceForRun(FontFaceHandleHarfBuzz* primary, StringView run, const TextShapingContext& context)
{
	// All styled items in one shaping run must use the same face, including
	// punctuation-only items. Otherwise ligatures cannot cross their boundary.
	const size_t start = context.shaping_run_offset;
	const size_t length = context.shaping_run_length;
	const size_t item = context.paragraph_offset;
	const bool has_run = !context.paragraph_text.empty() && length && start <= context.paragraph_text.size() &&
		length <= context.paragraph_text.size() - start && item >= start && item - start <= length &&
		run.size() <= length - (item - start) &&
		std::equal(run.begin(), run.end(), context.paragraph_text.begin() + item);
	const bool cacheable = has_run && context.paragraph_owner && context.paragraph_text.begin() == context.paragraph_owner->data() &&
		context.paragraph_text.size() == context.paragraph_owner->size();
	if (cacheable && face_run_owner == context.paragraph_owner && face_run_primary == primary &&
		face_run_offset == start && face_run_length == length) return face_run_result;
	if (has_run) {
		const char* begin = context.paragraph_text.begin() + start;
		run = StringView(begin, begin + length);
	}
	// Select one face that covers every Arabic character in the shaping run.
	auto supports_arabic_run = [&](FontFaceHandleHarfBuzz* face) {
		const char* p = run.begin();
		while (p < run.end()) {
			const Character c = Rml::StringUtilities::ToCharacter(p, run.end());
			if (hb_unicode_script(hb_unicode_funcs_get_default(), (hb_codepoint_t)c) == HB_SCRIPT_ARABIC && !face->SupportsCharacter(c))
				return false;
			const int bytes = (int)Rml::StringUtilities::BytesUTF8(c);
			if (bytes <= 0 || p + bytes > run.end()) break;
			p += bytes;
		}
		return true;
	};
	FontFaceHandleHarfBuzz* result = primary;
	if (!supports_arabic_run(primary)) {
		const int count = FontProvider::CountFallbackFontFaces();
		for (int i = 0; i < count; ++i) {
			auto* fallback = FontProvider::GetFallbackFontFace(i, primary->GetFontMetrics().size);
			if (fallback && supports_arabic_run(fallback)) { result = fallback; break; }
		}
	}
	face_run_owner = cacheable ? context.paragraph_owner : Rml::SharedPtr<const String>{};
	face_run_primary = primary;
	face_run_result = result;
	face_run_offset = start;
	face_run_length = length;
	return result;
}

void FontEngineInterfaceHarfBuzz::Initialize()
{
	FontProvider::Initialise();
}
void FontEngineInterfaceHarfBuzz::Shutdown()
{
	face_run_owner.reset();
	face_run_primary = face_run_result = nullptr;
	effects_by_face.clear();
	FontProvider::Shutdown();
}

bool FontEngineInterfaceHarfBuzz::LoadFontFace(const String& file_name, int face_index, bool fallback_face, Style::FontWeight weight)
{
	face_run_owner.reset();
	face_run_primary = face_run_result = nullptr;
	return FontProvider::LoadFontFace(file_name, face_index, fallback_face, weight);
}

bool FontEngineInterfaceHarfBuzz::LoadFontFace(Span<const byte> data, int face_index, const String& font_family, Style::FontStyle style,
	Style::FontWeight weight, bool fallback_face)
{
	face_run_owner.reset();
	face_run_primary = face_run_result = nullptr;
	return FontProvider::LoadFontFace(data, face_index, font_family, style, weight, fallback_face);
}

FontFaceHandle FontEngineInterfaceHarfBuzz::GetFontFaceHandle(const String& family, Style::FontStyle style, Style::FontWeight weight, int size)
{
	auto handle = FontProvider::GetFontFaceHandle(family, style, weight, size);
	return reinterpret_cast<FontFaceHandle>(handle);
}

FontEffectsHandle FontEngineInterfaceHarfBuzz::PrepareFontEffects(FontFaceHandle handle, const FontEffectList& font_effects)
{
	auto handle_harfbuzz = reinterpret_cast<FontFaceHandleHarfBuzz*>(handle);
	const auto configuration = (FontEffectsHandle)handle_harfbuzz->GenerateLayerConfiguration(font_effects);
	effects_by_face[handle][configuration] = font_effects;
	return configuration;
}

const FontMetrics& FontEngineInterfaceHarfBuzz::GetFontMetrics(FontFaceHandle handle)
{
	auto handle_harfbuzz = reinterpret_cast<FontFaceHandleHarfBuzz*>(handle);
	return handle_harfbuzz->GetFontMetrics();
}

int FontEngineInterfaceHarfBuzz::GetStringWidth(FontFaceHandle handle, StringView string, const TextShapingContext& text_shaping_context,
	Character prior_character)
{
	auto handle_harfbuzz = reinterpret_cast<FontFaceHandleHarfBuzz*>(handle);
	return ForVisualRuns(string, text_shaping_context, [&](StringView run, const TextShapingContext& run_context, int) {
		return FaceForRun(handle_harfbuzz, run, run_context)->GetStringWidth(run, run_context, registered_languages, prior_character);
	});
}

int FontEngineInterfaceHarfBuzz::GenerateString(RenderManager& render_manager, FontFaceHandle handle, FontEffectsHandle font_effects_handle,
	StringView string, Vector2f position, ColourbPremultiplied colour, float opacity, const TextShapingContext& text_shaping_context,
	TexturedMeshList& mesh_list)
{
	auto handle_harfbuzz = reinterpret_cast<FontFaceHandleHarfBuzz*>(handle);
	// ElementText accumulates every line into this list. Each run may use a
	// different face or texture, so append its meshes without discarding
	// geometry already emitted for earlier lines.
	return ForVisualRuns(string, text_shaping_context, [&](StringView run, const TextShapingContext& run_context, int offset) {
		auto* run_face = FaceForRun(handle_harfbuzz, run, run_context);
		int configuration = (int)font_effects_handle;
		if (run_face != handle_harfbuzz) {
			configuration = 0;
			const auto face_it = effects_by_face.find(handle);
			if (face_it != effects_by_face.end()) {
				const auto effects_it = face_it->second.find(font_effects_handle);
				if (effects_it != face_it->second.end()) configuration = run_face->GenerateLayerConfiguration(effects_it->second);
			}
		}
		TexturedMeshList run_meshes;
		const int width = run_face->GenerateString(render_manager, run_meshes, run, Vector2f(position.x + offset, position.y), colour, opacity,
			run_context, registered_languages, configuration);
		mesh_list.insert(mesh_list.end(), std::make_move_iterator(run_meshes.begin()), std::make_move_iterator(run_meshes.end()));
		return width;
	});
}

int FontEngineInterfaceHarfBuzz::GetVersion(FontFaceHandle handle)
{
	auto handle_harfbuzz = reinterpret_cast<FontFaceHandleHarfBuzz*>(handle);
	unsigned version = (unsigned)handle_harfbuzz->GetVersion();
	const int count = FontProvider::CountFallbackFontFaces();
	for (int i = 0; i < count; ++i) {
		const auto* fallback = FontProvider::GetFallbackFontFace(i, handle_harfbuzz->GetFontMetrics().size);
		if (fallback && fallback != handle_harfbuzz) version = version * 31u + (unsigned)fallback->GetVersion();
	}
	return (int)version;
}

void FontEngineInterfaceHarfBuzz::ReleaseFontResources()
{
	face_run_owner.reset();
	face_run_primary = face_run_result = nullptr;
	effects_by_face.clear();
	FontProvider::ReleaseFontResources();
}

void FontEngineInterfaceHarfBuzz::RegisterLanguage(const String& language_bcp47_code, const String& script_iso15924_code,
	const TextFlowDirection text_flow_direction)
{
	registered_languages[language_bcp47_code] = LanguageData{script_iso15924_code, text_flow_direction};
}
