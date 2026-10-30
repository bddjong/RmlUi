#pragma once

#include "../../../Include/RmlUi/Core/Box.h"
#include "../../../Include/RmlUi/Core/StyleTypes.h"
#include "InlineTypes.h"
#include "../TextParagraph.h"

namespace Rml {

class ElementText;
struct FontMetrics;

/**
    A box that takes part in inline layout.

    The inline-level box is used to generate fragments that are placed within line boxes.
 */
class InlineLevelBox {
public:
	virtual ~InlineLevelBox();

	// Create a fragment from this box, if it can fit within the available width.
	virtual FragmentConstructor CreateFragment(InlineLayoutMode mode, float available_width, float right_spacing_width, bool first_box,
		LayoutOverflowHandle overflow_handle) = 0;

	// Submit a fragment's position and size to be displayed on the underlying element.
	virtual void Submit(const PlacedFragment& placed_fragment) = 0;

	float GetHeightAboveBaseline() const { return height_above_baseline; }
	float GetDepthBelowBaseline() const { return depth_below_baseline; }
	Style::VerticalAlign::Type GetVerticalAlign() const { return vertical_align_type; }
	float GetVerticalOffsetFromParent() const { return vertical_offset_from_parent; }
	float GetSpacingLeft() const { return spacing_left; }
	float GetSpacingRight() const { return spacing_right; }
	Element* GetLayoutElement() const { return element; }
	void SetParagraphGroup(const TextParagraphCollector::Group* value) { paragraph_group = value; }
	const TextParagraphCollector::Group* GetParagraphGroup() const { return paragraph_group; }

	virtual String DebugDumpNameValue() const = 0;
	virtual String DebugDumpTree(int depth) const;

	void* operator new(size_t size);
	void operator delete(void* chunk, size_t size);

protected:
	InlineLevelBox(Element* element) : element(element) { RMLUI_ASSERT(element); }

	Element* GetElement() const { return element; }
	const FontMetrics& GetFontMetrics() const;

	// Set the height used for inline layout, and the vertical offset relative to our parent box.
	void SetHeightAndVerticalAlignment(float height_above_baseline, float depth_below_baseline, const InlineLevelBox* parent);

	// Set the height used for inline layout.
	void SetHeight(float height_above_baseline, float depth_below_baseline);

	// Set the inner-to-outer spacing (margin + border + padding) for inline boxes.
	void SetInlineBoxSpacing(float spacing_left, float spacing_right);

	// Calls Element::OnLayout (proxy for private access to Element).
	void SubmitElementOnLayout();

private:
	float height_above_baseline = 0.f;
	float depth_below_baseline = 0.f;

	Style::VerticalAlign::Type vertical_align_type = {};
	float vertical_offset_from_parent = 0.f;

	float spacing_left = 0.f;  // Left margin-border-padding for inline boxes.
	float spacing_right = 0.f; // Right margin-border-padding for inline boxes.

	Element* element;
	const TextParagraphCollector::Group* paragraph_group = nullptr;
};

/**
    Atomic inline-level boxes are sized boxes that cannot be split.

    This includes inline-block elements, replaced inline-level elements, inline tables, and inline flex containers.
 */
class InlineLevelBox_Atomic final : public InlineLevelBox {
public:
	InlineLevelBox_Atomic(const InlineLevelBox* parent, Element* element, const Box& box);

	FragmentConstructor CreateFragment(InlineLayoutMode mode, float available_width, float right_spacing_width, bool first_box,
		LayoutOverflowHandle overflow_handle) override;

	void Submit(const PlacedFragment& placed_fragment) override;

	String DebugDumpNameValue() const override { return "InlineLevelBox_Atomic"; }

private:
	Box box;
};

/**
    Inline-level text boxes represent text nodes.

    Generates fragments to display its text, splitting it up as necessary to fit in the available space.
 */
class InlineLevelBox_Text final : public InlineLevelBox {
public:
	InlineLevelBox_Text(ElementText* element, const TextParagraphCollector::Group* paragraph,
		const TextParagraphCollector::Span* paragraph_span);

	FragmentConstructor CreateFragment(InlineLayoutMode mode, float available_width, float right_spacing_width, bool first_box,
		LayoutOverflowHandle overflow_handle) override;

	void Submit(const PlacedFragment& placed_fragment) override;
	void SubmitSlices(const Vector<PlacedFragment>& placed_fragments);

	String DebugDumpNameValue() const override;

	struct TextFragment {
		String text;
		int source_begin = 0;
		int source_end = 0;
	};
	const TextFragment& GetFragment(LayoutFragmentHandle handle) const { return fragments[(size_t)handle]; }
	ElementText* GetTextElement();
	const TextParagraphCollector::Group* GetParagraph() const { return paragraph; }
	const TextParagraphCollector::Span* GetParagraphSpan() const { return paragraph_span; }

private:
	static int MeasureParagraphText(void* userdata, StringView text, int source_begin, int source_end, Character prior_character);
	Vector2f element_offset;
	Vector<TextFragment> fragments;
	const TextParagraphCollector::Group* paragraph;
	const TextParagraphCollector::Span* paragraph_span;
};

} // namespace Rml
