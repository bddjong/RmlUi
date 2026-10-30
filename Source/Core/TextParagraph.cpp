#include "TextParagraph.h"
#include "../../Include/RmlUi/Core/ComputedValues.h"
#include "../../Include/RmlUi/Core/Core.h"
#include "../../Include/RmlUi/Core/Element.h"
#include "../../Include/RmlUi/Core/ElementText.h"
#include "../../Include/RmlUi/Core/FontEngineInterface.h"
#include <algorithm>

namespace Rml {

TextParagraph::Fragment TextParagraph::Append(StringView source, Style::WhiteSpace white_space, Style::TextTransform transform,
	bool decode_escape_characters, bool trim_leading_whitespace, const Vector<Style::Direction>* active_isolates)
{
	Fragment fragment;
	fragment.source_to_paragraph.resize(source.size() + 1);
	const bool collapse = white_space == Style::WhiteSpace::Normal || white_space == Style::WhiteSpace::Nowrap ||
		white_space == Style::WhiteSpace::Preline;
	const bool preserve_newline = white_space == Style::WhiteSpace::Pre || white_space == Style::WhiteSpace::Prewrap ||
		white_space == Style::WhiteSpace::Preline;
	const char* data = source.begin();
	for (size_t i = 0; i < source.size();) {
		const size_t begin = i;
		char character = data[i++];
		bool nonbreaking = false;
		if (decode_escape_characters && character == '&') {
			size_t end = i;
			while (end < source.size() && data[end] != ';') ++end;
			if (end < source.size()) {
				const StringView code(data + i, data + end);
				if (code == "lt") character = '<';
				else if (code == "gt") character = '>';
				else if (code == "amp") character = '&';
				else if (code == "quot") character = '"';
				else if (code == "nbsp") { character = ' '; nonbreaking = true; }
				else end = source.size();
				if (end < source.size()) i = end + 1;
			}
		}
		const size_t output_begin = text.size();
		const bool newline = preserve_newline && character == '\n';
		const bool whitespace = !nonbreaking && StringUtilities::IsWhitespace(character);
		if (newline) {
			text += '\n';
			// UAX #9 terminates embeddings at a paragraph break. Reopen inline
			// isolates for the next hard line while retaining source mapping.
			if (active_isolates) for (Style::Direction direction : *active_isolates)
				AppendControl(direction == Style::Direction::Rtl ? "\xE2\x81\xA7" : direction == Style::Direction::Ltr ? "\xE2\x81\xA6" : "\xE2\x81\xA8");
			last_collapsed_space = false;
			at_line_start = true;
		} else if (whitespace && collapse) {
			if (!last_collapsed_space && !(trim_leading_whitespace && at_line_start)) {
				text += ' ';
				at_line_start = false;
			}
			last_collapsed_space = true;
		} else {
			if (whitespace) character = ' ';
			if (transform == Style::TextTransform::Uppercase && character >= 'a' && character <= 'z') character += 'A' - 'a';
			else if (transform == Style::TextTransform::Lowercase && character >= 'A' && character <= 'Z') character -= 'A' - 'a';
			text += character;
			last_collapsed_space = false;
			at_line_start = false;
		}
		for (size_t boundary = begin; boundary < i; ++boundary) fragment.source_to_paragraph[boundary] = output_begin;
		fragment.source_to_paragraph[i] = text.size();
	}
	if (source.empty()) fragment.source_to_paragraph[0] = text.size();
	return fragment;
}

void TextParagraph::AppendBreak()
{
	text += '\n';
	last_collapsed_space = false;
	at_line_start = true;
}

void TextParagraph::AppendControl(StringView control)
{
	text.append(control.begin(), control.end());
}

bool TextParagraph::MatchFragment(const Fragment& mapping, int source_begin, int source_end, StringView emitted, size_t& paragraph_begin) const
{
	if (source_begin < 0 || source_end < source_begin || (size_t)source_end >= mapping.source_to_paragraph.size()) return false;
	const size_t begin = mapping.source_to_paragraph[(size_t)source_begin];
	const size_t end = mapping.source_to_paragraph[(size_t)source_end];
	if (begin > end || end > text.size()) return false;
	const String value(emitted.begin(), emitted.end());
	auto matches = [&](size_t offset) {
		return offset <= end && value.size() <= end - offset && text.compare(offset, value.size(), value) == 0;
	};
	if (matches(begin)) { paragraph_begin = begin; return true; }
	// A soft wrap may trim the single collapsed space at the start of a line.
	if (begin < end && text[begin] == ' ' && matches(begin + 1)) { paragraph_begin = begin + 1; return true; }
	return false;
}

void TextParagraphCollector::Collect(Element* block)
{
	groups.clear();
	span_lookup.clear();
	object_lookup.clear();
	isolate_stack.clear();
	group_open = false;
	if (!block) return;
	base_direction = block->GetComputedValues().direction();
	for (int i = 0; i < block->GetNumChildren(); ++i) AppendElement(block->GetChild(i));
	FinishGroup();
	for (size_t group_index = 0; group_index < groups.size(); ++group_index)
	{
		for (size_t span_index = 0; span_index < groups[group_index].spans.size(); ++span_index)
			span_lookup.emplace(groups[group_index].spans[span_index].element, Pair<size_t, size_t>{group_index, span_index});
		for (const auto& object : groups[group_index].objects) object_lookup.emplace(object.element, group_index);
	}
}

const TextParagraphCollector::Group* TextParagraphCollector::Find(const ElementText* element) const
{
	const auto it = span_lookup.find(element);
	return it == span_lookup.end() ? nullptr : &groups[it->second.first];
}

const TextParagraphCollector::Span* TextParagraphCollector::FindSpan(const Group* group, const ElementText* element) const
{
	const auto it = span_lookup.find(element);
	if (it == span_lookup.end() || &groups[it->second.first] != group) return nullptr;
	return &group->spans[it->second.second];
}

const TextParagraphCollector::Group* TextParagraphCollector::FindObject(const Element* element) const
{
	const auto it = object_lookup.find(element);
	return it == object_lookup.end() ? nullptr : &groups[it->second];
}

TextParagraphCollector::Group& TextParagraphCollector::Current()
{
	if (!group_open) {
		groups.emplace_back();
		groups.back().base_direction = base_direction;
		group_open = true;
		for (Style::Direction direction : isolate_stack)
			groups.back().paragraph.AppendControl(direction == Style::Direction::Rtl ? "\xE2\x81\xA7" : direction == Style::Direction::Ltr ? "\xE2\x81\xA6" : "\xE2\x81\xA8");
	}
	return groups.back();
}

void TextParagraphCollector::FinishGroup()
{
	if (group_open) for (size_t i = 0; i < isolate_stack.size(); ++i) groups.back().paragraph.AppendControl("\xE2\x81\xA9");
	if (group_open && groups.back().spans.empty() && groups.back().objects.empty()) groups.pop_back();
	else if (group_open) {
		Group& group = groups.back();
		group.shared_text = MakeShared<const String>(group.paragraph.GetText());
		const auto compatible = [](const Span& left, const Span& right) {
			const auto& a = left.element->GetComputedValues();
			const auto& b = right.element->GetComputedValues();
			return left.element->GetFontFaceHandle() == right.element->GetFontFaceHandle() && a.language() == b.language() &&
				a.font_kerning() == b.font_kerning() && a.letter_spacing() == b.letter_spacing() &&
				left.mapping.source_to_paragraph.back() == right.mapping.source_to_paragraph.front();
		};
		for (size_t first = 0; first < group.spans.size();) {
			size_t end = first + 1;
			while (end < group.spans.size() && compatible(group.spans[end - 1], group.spans[end])) ++end;
			const size_t begin_offset = group.spans[first].mapping.source_to_paragraph.front();
			const size_t end_offset = group.spans[end - 1].mapping.source_to_paragraph.back();
			for (size_t i = first; i < end; ++i) {
				group.spans[i].shaping_begin = begin_offset;
				group.spans[i].shaping_end = end_offset;
			}
			first = end;
		}
		auto* engine = GetFontEngineInterface();
		group.bidi_valid = engine != nullptr;
		const String& text = *group.shared_text;
		for (size_t begin = 0; group.bidi_valid && begin < text.size();) {
			const size_t newline = text.find('\n', begin);
			const size_t end = newline == String::npos ? text.size() : newline;
			if (end > begin) {
				Vector<TextBidiRun> runs;
				group.bidi_valid = engine->ResolveTextBidi(text, group.base_direction, begin, end - begin, runs);
				group.bidi_runs.insert(group.bidi_runs.end(), runs.begin(), runs.end());
			}
			begin = newline == String::npos ? text.size() : newline + 1;
		}
		std::sort(group.bidi_runs.begin(), group.bidi_runs.end(), [](const TextBidiRun& a, const TextBidiRun& b) {
			return a.offset < b.offset;
		});
	}
	group_open = false;
}

void TextParagraphCollector::AppendElement(Element* element)
{
	if (!element) return;
	const auto& style = element->GetComputedValues();
	if (style.display() == Style::Display::None || style.position() == Style::Position::Absolute ||
		style.position() == Style::Position::Fixed || style.float_() != Style::Float::None) return;
	if (auto* text_element = rmlui_dynamic_cast<ElementText*>(element)) {
		auto& group = Current();
		const bool trim = group.paragraph.GetText().empty();
		const bool collapse_leading = group.paragraph.EndsCollapsedSpace();
		group.spans.push_back({text_element, group.paragraph.Append(text_element->GetText(), style.white_space(),
			style.text_transform(), true, trim, &isolate_stack), collapse_leading});
		// A dynamic text node can become empty after parsing. Retain an object
		// position for its inline box so its padding and neighbours still take
		// part in visual ordering.
		if (text_element->GetText().empty()) {
			const size_t begin = group.paragraph.GetText().size();
			// A word joiner is bidi-boundary-neutral, so an empty node does not
			// change the ordering of adjacent neutral punctuation or numbers.
			group.paragraph.AppendControl("\xE2\x81\xA0");
			group.objects.push_back({text_element, begin, group.paragraph.GetText().size()});
		}
		return;
	}
	if (element->GetTagName() == "br") {
		auto& paragraph = Current().paragraph;
		paragraph.AppendBreak();
		for (Style::Direction direction : isolate_stack)
			paragraph.AppendControl(direction == Style::Direction::Rtl ? "\xE2\x81\xA7" : direction == Style::Direction::Ltr ? "\xE2\x81\xA6" : "\xE2\x81\xA8");
		return;
	}
	if (style.display() == Style::Display::Inline) {
		bool has_in_flow_child = false;
		for (int i = 0; i < element->GetNumChildren(); ++i) {
			const auto& child_style = element->GetChild(i)->GetComputedValues();
			if (child_style.display() != Style::Display::None && child_style.position() != Style::Position::Absolute &&
				child_style.position() != Style::Position::Fixed && child_style.float_() == Style::Float::None) {
				has_in_flow_child = true;
				break;
			}
		}
		if (!has_in_flow_child) {
			auto& group = Current();
			const size_t begin = group.paragraph.GetText().size();
			group.paragraph.Append("\xEF\xBF\xBC", Style::WhiteSpace::Pre, Style::TextTransform::None, false, false);
			group.objects.push_back({element, begin, group.paragraph.GetText().size()});
			return;
		}
		const Element* parent = element->GetParentNode();
		const Style::Direction parent_direction = parent ? parent->GetComputedValues().direction() : base_direction;
		const bool explicit_auto = element->HasAttribute("dir") && element->GetAttribute<String>("dir", "") == "auto";
		const bool isolate = explicit_auto || (style.direction() != Style::Direction::Auto && style.direction() != parent_direction);
		const Style::Direction isolate_direction = explicit_auto ? Style::Direction::Auto : style.direction();
		if (isolate) {
			Current().paragraph.AppendControl(isolate_direction == Style::Direction::Rtl ? "\xE2\x81\xA7" : isolate_direction == Style::Direction::Ltr ? "\xE2\x81\xA6" : "\xE2\x81\xA8");
			isolate_stack.push_back(isolate_direction);
		}
		for (int i = 0; i < element->GetNumChildren(); ++i) AppendElement(element->GetChild(i));
		if (isolate) {
			if (group_open) Current().paragraph.AppendControl("\xE2\x81\xA9");
			isolate_stack.pop_back();
		}
		return;
	}
	if (style.display() == Style::Display::InlineBlock || style.display() == Style::Display::InlineFlex ||
		style.display() == Style::Display::InlineTable) {
		auto& group = Current();
		const size_t begin = group.paragraph.GetText().size();
		group.paragraph.Append("\xEF\xBF\xBC", Style::WhiteSpace::Pre, Style::TextTransform::None, false, false);
		group.objects.push_back({element, begin, group.paragraph.GetText().size()});
		return;
	}
	FinishGroup();
}

} // namespace Rml
