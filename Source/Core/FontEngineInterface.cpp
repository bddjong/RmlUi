#include "../../Include/RmlUi/Core/FontEngineInterface.h"
#include "../../Include/RmlUi/Core/StringUtilities.h"
#include <SheenBidi/SheenBidi.h>

namespace Rml {

FontEngineInterface::FontEngineInterface() {}

FontEngineInterface::~FontEngineInterface() {}

void FontEngineInterface::Initialize() {}

void FontEngineInterface::Shutdown() {}

bool FontEngineInterface::LoadFontFace(const String& /*file_path*/, int /*face_index*/, bool /*fallback_face*/, Style::FontWeight /*weight*/)
{
	return false;
}

bool FontEngineInterface::LoadFontFace(const String& /*file_path*/, int /*face_index*/, const String& /*family*/, Style::FontStyle /*style*/,
	Style::FontWeight /*weight*/, bool /*fallback_face*/)
{
	return false;
}

bool FontEngineInterface::LoadFontFace(Span<const byte> /*data*/, int /*face_index*/, const String& /*family*/, Style::FontStyle /*style*/,
	Style::FontWeight /*weight*/, bool /*fallback_face*/)
{
	return false;
}

void FontEngineInterface::SetFontFamilyRasterizationPolicy(const String& /*family*/, FontRasterizationPolicy /*policy*/) {}

void FontEngineInterface::ClearFontFamilyRasterizationPolicies() {}

FontFaceHandle FontEngineInterface::GetFontFaceHandle(const String& /*family*/, Style::FontStyle /*style*/, Style::FontWeight /*weight*/,
	int /*size*/)
{
	return 0;
}

FontEffectsHandle FontEngineInterface::PrepareFontEffects(FontFaceHandle /*handle*/, const FontEffectList& /*font_effects*/)
{
	return 0;
}

const FontMetrics& FontEngineInterface::GetFontMetrics(FontFaceHandle /*handle*/)
{
	static const FontMetrics metrics = {};
	return metrics;
}

bool FontEngineInterface::ResolveTextBidi(StringView paragraph, Style::Direction base_direction, size_t line_offset,
	size_t line_length, Vector<TextBidiRun>& visual_runs)
{
	visual_runs.clear();
	if (line_offset > paragraph.size() || line_length > paragraph.size() - line_offset)
		return false;
	const auto boundary = [&](size_t position) {
		return position == 0 || position == paragraph.size() ||
			(static_cast<unsigned char>(paragraph.begin()[position]) & 0xc0) != 0x80;
	};
	if (!boundary(line_offset) || !boundary(line_offset + line_length))
		return false;
	if (line_length == 0)
		return true;

	SBCodepointSequence sequence{SBStringEncodingUTF8, paragraph.begin(), paragraph.size()};
	SBAlgorithmRef algorithm = SBAlgorithmCreate(&sequence);
	if (!algorithm)
		return false;
	const SBLevel base = base_direction == Style::Direction::Rtl ? 1 :
		(base_direction == Style::Direction::Ltr ? 0 : SBLevelDefaultLTR);
	SBParagraphRef selected = nullptr;
	for (size_t offset = 0; offset < paragraph.size();) {
		SBParagraphRef current = SBAlgorithmCreateParagraph(algorithm, offset, paragraph.size() - offset, base);
		if (!current)
			break;
		const size_t length = SBParagraphGetLength(current);
		if (!length) { SBParagraphRelease(current); break; }
		if (line_offset >= offset && line_offset < offset + length) {
			if (line_offset + line_length <= offset + length)
				selected = current;
			else
				SBParagraphRelease(current);
			break;
		}
		SBParagraphRelease(current);
		offset += length;
	}
	bool success = false;
	if (selected) {
		SBLineRef line = SBParagraphCreateLine(selected, line_offset, line_length);
		if (line) {
			const SBRun* runs = SBLineGetRunsPtr(line);
			for (SBUInteger i = 0; i < SBLineGetRunCount(line); ++i) {
				const SBRun& run = runs[i];
				visual_runs.push_back({run.offset, run.length,
					(run.level & 1) ? Style::Direction::Rtl : Style::Direction::Ltr});
			}
			SBLineRelease(line);
			success = true;
		}
		SBParagraphRelease(selected);
	}
	SBAlgorithmRelease(algorithm);
	return success;
}

int FontEngineInterface::GetStringWidth(FontFaceHandle /*handle*/, StringView /*string*/, const TextShapingContext& /*text_shaping_context*/,
	Character /*prior_character*/)
{
	return 0;
}

int FontEngineInterface::GenerateString(RenderManager& /*render_manager*/, FontFaceHandle /*face_handle*/, FontEffectsHandle /*font_effects_handle*/,
	StringView /*string*/, Vector2f /*position*/, ColourbPremultiplied /*colour*/, float /*opacity*/,
	const TextShapingContext& /*text_shaping_context*/, TexturedMeshList& /*mesh_list*/)
{
	return 0;
}

int FontEngineInterface::GetVersion(FontFaceHandle /*handle*/)
{
	return 0;
}

void FontEngineInterface::ReleaseFontResources() {}

} // namespace Rml
