#include "InlineLevelBox.h"
#include "../../../Include/RmlUi/Core/ComputedValues.h"
#include "../../../Include/RmlUi/Core/Core.h"
#include "../../../Include/RmlUi/Core/ElementText.h"
#include "../../../Include/RmlUi/Core/FontEngineInterface.h"
#include "LayoutDetails.h"
#include "LayoutPools.h"
#include <algorithm>

namespace Rml {

void* InlineLevelBox::operator new(size_t size)
{
	return LayoutPools::AllocateLayoutChunk(size);
}

void InlineLevelBox::operator delete(void* chunk, size_t size)
{
	LayoutPools::DeallocateLayoutChunk(chunk, size);
}

InlineLevelBox::~InlineLevelBox() {}

void InlineLevelBox::SubmitElementOnLayout()
{
	element->OnLayout();
}

const FontMetrics& InlineLevelBox::GetFontMetrics() const
{
	if (FontFaceHandle handle = element->GetFontFaceHandle())
		return GetFontEngineInterface()->GetFontMetrics(handle);

	// If there is no font face defined then we provide zero'd out font metrics. This situation can affect the layout,
	// in particular in terms of inline box sizing and vertical alignment. Thus, this is potentially a situation where
	// we might want to log a warning. However, in many cases it will produce the same layout with or without the font,
	// so in that sense the warnings can produce false positives.
	//
	// For now, we wait until we try to actually place text before producing any warnings, since that is a clear
	// erroneous situation producing no text. See 'LogMissingFontFace' in ElementText.cpp, which also lists some
	// possible reasons for the missing font face.

	static const FontMetrics font_metrics = {};
	return font_metrics;
}

void InlineLevelBox::SetHeightAndVerticalAlignment(float _height_above_baseline, float _depth_below_baseline, const InlineLevelBox* parent)
{
	RMLUI_ASSERT(parent);
	using Style::VerticalAlign;

	SetHeight(_height_above_baseline, _depth_below_baseline);

	const Style::VerticalAlign vertical_align = element->GetComputedValues().vertical_align();
	vertical_align_type = vertical_align.type;

	// Determine the offset from the parent baseline.
	float parent_baseline_offset = 0.f; // The anchor on the parent, as an offset from its baseline.
	float self_baseline_offset = 0.f;   // The offset from the parent anchor to our baseline.

	switch (vertical_align.type)
	{
	case VerticalAlign::Baseline: parent_baseline_offset = 0.f; break;
	case VerticalAlign::Length: parent_baseline_offset = -vertical_align.value; break;
	case VerticalAlign::Sub: parent_baseline_offset = (1.f / 5.f) * (float)parent->GetFontMetrics().size; break;
	case VerticalAlign::Super: parent_baseline_offset = (-1.f / 3.f) * (float)parent->GetFontMetrics().size; break;
	case VerticalAlign::TextTop:
		parent_baseline_offset = -parent->GetFontMetrics().ascent;
		self_baseline_offset = height_above_baseline;
		break;
	case VerticalAlign::TextBottom:
		parent_baseline_offset = parent->GetFontMetrics().descent;
		self_baseline_offset = -depth_below_baseline;
		break;
	case VerticalAlign::Middle:
		parent_baseline_offset = -0.5f * parent->GetFontMetrics().x_height;
		self_baseline_offset = 0.5f * (height_above_baseline - depth_below_baseline);
		break;
	case VerticalAlign::Top:
	case VerticalAlign::Center:
	case VerticalAlign::Bottom:
		// These are relative to the line box and handled later.
		break;
	}

	vertical_offset_from_parent = parent_baseline_offset + self_baseline_offset;
}

void InlineLevelBox::SetHeight(float _height_above_baseline, float _depth_below_baseline)
{
	height_above_baseline = _height_above_baseline;
	depth_below_baseline = _depth_below_baseline;
}

void InlineLevelBox::SetInlineBoxSpacing(float _spacing_left, float _spacing_right)
{
	spacing_left = _spacing_left;
	spacing_right = _spacing_right;
}

String InlineLevelBox::DebugDumpTree(int depth) const
{
	String value = String(depth * 2, ' ') + DebugDumpNameValue() + " | " + LayoutDetails::GetDebugElementName(GetElement()) + '\n';
	return value;
}

InlineLevelBox_Atomic::InlineLevelBox_Atomic(const InlineLevelBox* parent, Element* element, const Box& box) : InlineLevelBox(element), box(box)
{
	RMLUI_ASSERT(parent && element);
	RMLUI_ASSERT(box.GetSize().x >= 0.f && box.GetSize().y >= 0.f);

	const float outer_height = box.GetSizeAcross(BoxDirection::Vertical, BoxArea::Margin);

	const float descent = GetElement()->GetBaseline();
	const float ascent = outer_height - descent;
	SetHeightAndVerticalAlignment(ascent, descent, parent);
}

FragmentConstructor InlineLevelBox_Atomic::CreateFragment(InlineLayoutMode mode, float available_width, float right_spacing_width, bool /*first_box*/,
	LayoutOverflowHandle /*overflow_handle*/)
{
	const float outer_width = box.GetSizeAcross(BoxDirection::Horizontal, BoxArea::Margin);

	if (mode != InlineLayoutMode::WrapAny || outer_width + right_spacing_width <= available_width)
		return FragmentConstructor{FragmentType::SizedBox, outer_width, {}, {}};

	return {};
}

void InlineLevelBox_Atomic::Submit(const PlacedFragment& placed_fragment)
{
	const Vector2f margin_position = {placed_fragment.position.x, placed_fragment.position.y - GetHeightAboveBaseline()};
	const Vector2f margin_edge = {box.GetEdge(BoxArea::Margin, BoxEdge::Left), box.GetEdge(BoxArea::Margin, BoxEdge::Top)};
	const Vector2f border_position = margin_position + margin_edge;

	GetElement()->SetOffset(border_position, placed_fragment.offset_parent);
	GetElement()->SetBox(box);
	SubmitElementOnLayout();
}

InlineLevelBox_Text::InlineLevelBox_Text(ElementText* element, const TextParagraphCollector::Group* _paragraph,
	const TextParagraphCollector::Span* _paragraph_span) : InlineLevelBox(element), paragraph(_paragraph), paragraph_span(_paragraph_span) {}

int InlineLevelBox_Text::MeasureParagraphText(void* userdata, StringView value, int source_begin, int source_end, Character prior_character)
{
	auto* self = static_cast<InlineLevelBox_Text*>(userdata);
	if (!self->paragraph || !self->paragraph_span || source_begin < 0 || source_end < source_begin ||
		(size_t)source_end >= self->paragraph_span->mapping.source_to_paragraph.size()) return -1;
	const String& logical = self->paragraph->paragraph.GetText();
	const String item(value.begin(), value.end());
	size_t begin = 0;
	if (!self->paragraph->paragraph.MatchFragment(self->paragraph_span->mapping, source_begin, source_end, value, begin)) return -1;
	const size_t end = begin + item.size();
	if (begin == end) return 0;
	auto* engine = GetFontEngineInterface();
	if (!engine || !self->paragraph->bidi_valid) return -1;
	ElementText* element = self->GetTextElement();
	const auto& style = element->GetComputedValues();
	const auto face = element->GetFontFaceHandle();
	int width = 0;
	size_t covered = 0;
	const auto& runs = self->paragraph->bidi_runs;
	auto first_run = std::lower_bound(runs.begin(), runs.end(), begin,
		[](const TextBidiRun& run, size_t offset) { return run.offset + run.length <= offset; });
	for (auto it = first_run; it != runs.end() && it->offset < end; ++it) {
		const auto& run = *it;
		const size_t part_begin = Math::Max(begin, run.offset);
		const size_t part_end = Math::Min(end, run.offset + run.length);
		if (part_begin >= part_end) continue;
		const size_t shape_begin = Math::Max(run.offset, self->paragraph_span->shaping_begin);
		const size_t shape_end = Math::Min(run.offset + run.length, self->paragraph_span->shaping_end);
		if (shape_begin > part_begin || shape_end < part_end) return -1;
		TextShapingContext context{style.language(), run.direction, style.font_kerning(), style.letter_spacing()};
		context.paragraph_text = logical;
		context.paragraph_owner = self->paragraph->shared_text;
		context.paragraph_offset = part_begin;
		context.direction_resolved = true;
		context.shaping_run_offset = shape_begin;
		context.shaping_run_length = shape_end - shape_begin;
		width += engine->GetStringWidth(face, StringView(logical, part_begin, part_end - part_begin), context,
			part_begin == begin ? prior_character : Character::Null);
		covered += part_end - part_begin;
	}
	return covered == item.size() ? width : -1;
}

FragmentConstructor InlineLevelBox_Text::CreateFragment(InlineLayoutMode mode, float available_width, float right_spacing_width, bool first_box,
	LayoutOverflowHandle in_overflow_handle)
{
	ElementText* text_element = GetTextElement();

	const bool allow_empty = (mode == InlineLayoutMode::WrapAny);
	const bool decode_escape_characters = true;

	String line_contents;
	int line_begin = in_overflow_handle;
	int line_length = 0;
	float line_width = 0.f;
	const bool trim_prefix = first_box || (line_begin == 0 && paragraph_span && paragraph_span->collapse_leading_space);
	bool overflow = !text_element->GenerateLine(line_contents, line_length, line_width, line_begin, available_width, right_spacing_width, trim_prefix,
		decode_escape_characters, allow_empty, paragraph && paragraph_span ? &InlineLevelBox_Text::MeasureParagraphText : nullptr, this);

	if (overflow && line_contents.empty())
		// We couldn't fit anything on this line.
		return {};

	LayoutOverflowHandle out_overflow_handle = {};
	if (overflow)
		out_overflow_handle = line_begin + line_length;

	LayoutFragmentHandle fragment_handle = (LayoutFragmentHandle)fragments.size();
	fragments.push_back({std::move(line_contents), line_begin, line_begin + line_length});

	return FragmentConstructor{FragmentType::TextRun, line_width, fragment_handle, out_overflow_handle};
}

void InlineLevelBox_Text::Submit(const PlacedFragment& placed_fragment)
{
	RMLUI_ASSERT((size_t)placed_fragment.handle < fragments.size());

	const int fragment_index = (int)placed_fragment.handle;
	const bool principal_box = (fragment_index == 0);

	ElementText* text_element = GetTextElement();
	Vector2f line_offset;

	if (principal_box)
	{
		element_offset = placed_fragment.position;
		text_element->SetOffset(placed_fragment.position, placed_fragment.offset_parent);
		text_element->ClearLines();
	}
	else
	{
		line_offset = placed_fragment.position - element_offset;
	}

	if (placed_fragment.paragraph) {
		text_element->AddLine(line_offset, std::move(fragments[fragment_index].text), placed_fragment.paragraph,
			placed_fragment.paragraph_offset, placed_fragment.direction, placed_fragment.shaping_run_offset,
			placed_fragment.shaping_run_length);
	} else {
		text_element->AddLine(line_offset, std::move(fragments[fragment_index].text));
	}
}

void InlineLevelBox_Text::SubmitSlices(const Vector<PlacedFragment>& placed_fragments)
{
	if (placed_fragments.empty()) return;
	const int fragment_index = (int)placed_fragments.front().handle;
	RMLUI_ASSERT(fragment_index >= 0 && (size_t)fragment_index < fragments.size());
	ElementText* text_element = GetTextElement();
	if (fragment_index == 0) {
		element_offset = placed_fragments.front().position;
		text_element->SetOffset(element_offset, placed_fragments.front().offset_parent);
		text_element->ClearLines();
	}
	const String& text = fragments[(size_t)fragment_index].text;
	for (const auto& piece : placed_fragments) {
		RMLUI_ASSERT(piece.handle == fragment_index && piece.paragraph && piece.text_offset + piece.text_length <= text.size());
		text_element->AddLine(piece.position - element_offset, text.substr(piece.text_offset, piece.text_length), piece.paragraph,
			piece.paragraph_offset, piece.direction, piece.shaping_run_offset, piece.shaping_run_length);
	}
}

String InlineLevelBox_Text::DebugDumpNameValue() const
{
	return "InlineLevelBox_Text";
}

ElementText* InlineLevelBox_Text::GetTextElement()
{
	return rmlui_static_cast<ElementText*>(GetElement());
}
} // namespace Rml
