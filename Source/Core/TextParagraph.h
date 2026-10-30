#pragma once

#include "../../Include/RmlUi/Core/StringUtilities.h"
#include "../../Include/RmlUi/Core/Header.h"
#include "../../Include/RmlUi/Core/StyleTypes.h"
#include "../../Include/RmlUi/Core/TextShapingContext.h"
#include "../../Include/RmlUi/Core/Types.h"

namespace Rml {

class Element;
class ElementText;

// Normalized logical paragraph text and source-byte boundary mapping. A
// fragment records one entry per source boundary, including its end. Entries
// are offsets into the complete paragraph, allowing layout to retain source
// offsets while the bidi resolver sees normalized text.
class RMLUICORE_API TextParagraph {
public:
	struct Fragment {
		Vector<size_t> source_to_paragraph;
	};

	Fragment Append(StringView source, Style::WhiteSpace white_space, Style::TextTransform transform,
		bool decode_escape_characters, bool trim_leading_whitespace, const Vector<Style::Direction>* active_isolates = nullptr);
	void AppendBreak();
	void AppendControl(StringView control);
	const String& GetText() const { return text; }
	bool EndsCollapsedSpace() const { return last_collapsed_space; }
	bool MatchFragment(const Fragment& mapping, int source_begin, int source_end, StringView emitted, size_t& paragraph_begin) const;

private:
	String text;
	bool last_collapsed_space = false;
	bool at_line_start = true;
};

// Prepass over inline children of one block container. Block descendants form
// separate inline formatting contexts; atomic inline descendants contribute an
// object replacement character to the surrounding logical text.
class RMLUICORE_API TextParagraphCollector {
public:
	struct Span {
		ElementText* element = nullptr;
		TextParagraph::Fragment mapping;
		bool collapse_leading_space = false;
		size_t shaping_begin = 0;
		size_t shaping_end = 0;
	};
	struct Group {
		struct Object { Element* element = nullptr; size_t begin = 0; size_t end = 0; };
		TextParagraph paragraph;
		SharedPtr<const String> shared_text;
		Vector<Span> spans;
		Vector<Object> objects;
		Style::Direction base_direction = Style::Direction::Auto;
		Vector<TextBidiRun> bidi_runs;
		bool bidi_valid = false;
	};

	void Collect(Element* block);
	const Vector<Group>& GetGroups() const { return groups; }
	const Group* Find(const ElementText* element) const;
	const Span* FindSpan(const Group* group, const ElementText* element) const;
	const Group* FindObject(const Element* element) const;

private:
	void AppendElement(Element* element);
	Group& Current();
	void FinishGroup();
	Vector<Group> groups;
	UnorderedMap<const ElementText*, Pair<size_t, size_t>> span_lookup;
	UnorderedMap<const Element*, size_t> object_lookup;
	bool group_open = false;
	Style::Direction base_direction = Style::Direction::Auto;
	Vector<Style::Direction> isolate_stack;
};

} // namespace Rml
