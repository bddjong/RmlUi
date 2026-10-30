#include "LineBox.h"
#include "../../../Include/RmlUi/Core/Element.h"
#include "../../../Include/RmlUi/Core/ElementText.h"
#include "../../../Include/RmlUi/Core/ComputedValues.h"
#include "../../../Include/RmlUi/Core/Core.h"
#include "../../../Include/RmlUi/Core/FontEngineInterface.h"
#include "../../../Include/RmlUi/Core/StyleTypes.h"
#include "InlineBox.h"
#include "InlineLevelBox.h"
#include "LayoutPools.h"
#include <float.h>
#include <algorithm>
#include <numeric>

namespace Rml {

LineBox::~LineBox() {}

void LineBox::SetLineBox(Vector2f _line_position, float _line_width, float _line_minimum_height)
{
	line_position = _line_position;
	line_width = _line_width;
	line_minimum_height = _line_minimum_height;
}

bool LineBox::AddBox(InlineLevelBox* box, InlineLayoutMode layout_mode, LayoutOverflowHandle& inout_overflow_handle)
{
	RMLUI_ASSERT(!is_closed);
	const bool first_box = !HasContent();

	// Find the spacing this element must leave on its right side, to account for its parent inline boxes to be closed later.
	float open_spacing_right = 0.f;
	ForAllOpenFragments([&](const Fragment& fragment) { open_spacing_right += fragment.box->GetSpacingRight(); });

	const float box_placement_cursor = box_cursor + open_spacing_left;

	float available_width = FLT_MAX;
	if (layout_mode != InlineLayoutMode::Nowrap)
	{
		available_width = Math::RoundUp(line_width - box_placement_cursor);
		if (available_width < 0.f)
		{
			if (layout_mode == InlineLayoutMode::WrapAny)
				return true;
			else
				available_width = 0.f;
		}
	}

	FragmentConstructor constructor = box->CreateFragment(layout_mode, available_width, open_spacing_right, first_box, inout_overflow_handle);

	if (constructor.type == FragmentType::Invalid)
	{
		// Could not place fragment on this line, try again on a new line.
		RMLUI_ASSERT(layout_mode == InlineLayoutMode::WrapAny);
		return true;
	}

	const FragmentIndex fragment_index = (FragmentIndex)fragments.size();

	fragments.push_back(Fragment{box, constructor, box->GetVerticalAlign(), box_placement_cursor, open_fragments_leaf});
	fragments.back().aligned_subtree_root = DetermineAlignedSubtreeRoot(fragment_index);

	inout_overflow_handle = {};
	bool continue_on_new_line = false;

	switch (constructor.type)
	{
	case FragmentType::InlineBox:
	{
		RMLUI_ASSERT(constructor.layout_width < 0.f);
		RMLUI_ASSERT(rmlui_static_cast<InlineBox*>(box));

		open_fragments_leaf = fragment_index;
		open_spacing_left += box->GetSpacingLeft();
	}
	break;
	case FragmentType::SizedBox:
	case FragmentType::TextRun:
	{
		RMLUI_ASSERT(constructor.layout_width >= 0.f);

		box_cursor = box_placement_cursor + constructor.layout_width;
		open_spacing_left = 0.f;

		if (constructor.overflow_handle)
		{
			continue_on_new_line = true;
			inout_overflow_handle = constructor.overflow_handle;
		}

		// Mark open fragments as having content so we later know whether we should split or move them in case of overflow.
		ForAllOpenFragments([&](Fragment& fragment) { fragment.has_content = true; });
		has_content = true;
	}
	break;
	case FragmentType::Invalid:
		RMLUI_ERROR; // Handled above;
		break;
	}

	return continue_on_new_line;
}

void LineBox::CloseFragment(Fragment& open_fragment, float right_inner_edge_position)
{
	RMLUI_ASSERT(open_fragment.type == FragmentType::InlineBox);

	open_fragment.children_end_index = (int)fragments.size();
	const float spacing_left = (open_fragment.split_left ? 0.f : open_fragment.box->GetSpacingLeft());
	open_fragment.layout_width = Math::Max(right_inner_edge_position - open_fragment.position.x - spacing_left, 0.f);
}

void LineBox::CloseInlineBox(InlineBox* inline_box)
{
	if (open_fragments_leaf == RootFragmentIndex || fragments[open_fragments_leaf].box != inline_box)
	{
		RMLUI_ERRORMSG("Inline box open/close mismatch.");
		return;
	}

	box_cursor += open_spacing_left;
	open_spacing_left = 0.f;

	Fragment& fragment = fragments[open_fragments_leaf];
	CloseFragment(fragment, box_cursor);
	box_cursor += fragment.box->GetSpacingRight();

	open_fragments_leaf = fragment.parent;
}

UniquePtr<LineBox> LineBox::SplitLine(bool split_all_open_boxes)
{
	if (open_fragments_leaf == RootFragmentIndex)
		return nullptr;

	int num_open_fragments = 0;
	ForAllOpenFragments([&](const Fragment& /*fragment*/) { num_open_fragments += 1; });

	// Move the box cursor to account for any newly opened inline boxes to be closed.
	box_cursor += open_spacing_left;
	open_spacing_left = 0.f;

	// Make a new line with the open fragments.
	auto new_line = MakeUnique<LineBox>();
	new_line->fragments.resize(num_open_fragments);

	// Copy all open fragments to the next line. Do it in reverse order of iteration, since we iterate from back to front.
	FragmentIndex new_index = num_open_fragments;
	ForAllOpenFragments([&](Fragment& old_fragment) {
		new_index -= 1;
		RMLUI_ASSERT((size_t)new_index < new_line->fragments.size() && old_fragment.children_end_index == 0);

		// Copy the old fragment.
		Fragment& new_fragment = new_line->fragments[new_index];
		new_fragment = old_fragment;
		new_fragment.position.x = 0.f;
		new_fragment.parent = new_index - 1;
		new_fragment.aligned_subtree_root = new_line->DetermineAlignedSubtreeRoot(new_index);

		// Only fragments with content placed on the previous line is split, otherwise the fragment is moved down.
		if (new_fragment.has_content || split_all_open_boxes)
		{
			new_fragment.split_left = true;
			new_fragment.has_content = false;

			CloseFragment(old_fragment, box_cursor);
			old_fragment.split_right = true;
		}
		else
		{
			const float spacing_left = (new_fragment.split_left ? 0.f : new_fragment.box->GetSpacingLeft());
			new_line->open_spacing_left += spacing_left;

			// The old fragment is not closed here, which ensures that it will not be placed/submitted on the previous
			// line. Backtrack the box cursor since this fragment is moved down to the next line.
			box_cursor -= spacing_left;
		}
	});

	new_line->open_fragments_leaf = (int)new_line->fragments.size() - 1;
	open_fragments_leaf = RootFragmentIndex;

#ifdef RMLUI_DEBUG
	// Verify integrity of the fragment tree after split.
	for (int i = 0; i < (int)new_line->fragments.size(); i++)
	{
		const Fragment& fragment = new_line->fragments[i];
		RMLUI_ASSERT(fragment.type == FragmentType::InlineBox);
		RMLUI_ASSERT(fragment.parent < i);
		RMLUI_ASSERT(fragment.parent == i - 1);
		RMLUI_ASSERT(fragment.parent == RootFragmentIndex || new_line->fragments[fragment.parent].type == FragmentType::InlineBox);
		RMLUI_ASSERT(
			fragment.aligned_subtree_root == RootFragmentIndex || new_line->IsAlignedSubtreeRoot(new_line->fragments[fragment.aligned_subtree_root]));
		RMLUI_ASSERT(fragment.children_end_index == 0);
	}
#endif

	return new_line;
}

UniquePtr<LineBox> LineBox::DetermineVerticalPositioning(const InlineBoxRoot* root_inline_box, bool split_all_open_boxes, float& out_height_of_line)
{
	RMLUI_ASSERT(!is_closed && !is_vertically_positioned);

	UniquePtr<LineBox> new_line_box = SplitLine(split_all_open_boxes);

	RMLUI_ASSERT(open_fragments_leaf == RootFragmentIndex); // Ensure all open fragments are either closed or split.

	// Vertical alignment and sizing.
	//
	// Aligned subtree CSS definition: "The aligned subtree of an inline element contains that element and the aligned
	// subtrees of all children inline elements whose computed vertical-align value is not 'top' or 'bottom'."
	//
	// We have already determined each box's offset relative to its parent baseline, and its layout size above and below
	// its baseline. Now, for each aligned subtree, place all fragments belonging to that subtree relative to the
	// subtree root baseline. Simultaneously, consider each fragment and keep track of the maximum height above root
	// baseline (max_ascent) and maximum depth below root baseline (max_descent). Their sum is the height of that
	// aligned subtree.
	//
	// Next, treat the root inline box like just another aligned subtree. Then the line box height is first determined
	// by the height of that subtree. Other aligned subtrees might push out that size to make them fit. After the line
	// box size is determined, position each aligned subtree according to its vertical-align, and then position each
	// fragment relative to the aligned subtree they belong to.
	float max_ascent = root_inline_box->GetHeightAboveBaseline();
	float max_descent = root_inline_box->GetDepthBelowBaseline();

	{
		const int subtree_root_index = RootFragmentIndex;
		const int children_end_index = (int)fragments.size();
		VerticallyAlignSubtree(subtree_root_index, children_end_index, max_ascent, max_descent);
	}

	// Find all the aligned subtrees, and vertically align each of them independently.
	for (int i = 0; i < (int)fragments.size(); i++)
	{
		Fragment& fragment = fragments[i];
		if (IsAlignedSubtreeRoot(fragment))
		{
			fragment.max_ascent = fragment.box->GetHeightAboveBaseline();
			fragment.max_descent = fragment.box->GetDepthBelowBaseline();

			if (fragment.type == FragmentType::InlineBox)
			{
				const int subtree_root_index = i;
				VerticallyAlignSubtree(subtree_root_index, fragment.children_end_index, fragment.max_ascent, fragment.max_descent);
			}

			const float subtree_height = fragment.max_ascent + fragment.max_descent;

			// Increase the line box size to fit all line-relative aligned fragments.
			switch (fragment.vertical_align)
			{
			case VerticalAlignType::Top: max_descent = Math::Max(max_descent, subtree_height - max_ascent); break;
			case VerticalAlignType::Bottom: max_ascent = Math::Max(max_ascent, subtree_height - max_descent); break;
			case VerticalAlignType::Center:
			{
				// Distribute the subtree's height equally to the ascent and descent.
				const float distribute_height = 0.5f * (subtree_height - (max_ascent + max_descent));
				if (distribute_height > 0.f)
				{
					max_ascent += distribute_height;
					max_descent += distribute_height;
				}
			}
			break;
			default: RMLUI_ERROR; break;
			}
		}
	}

	// Size the line.
	out_height_of_line = max_ascent + max_descent;
	total_height_above_baseline = max_ascent;

	// Now that the line is sized we can set the vertical position of the fragments.
	for (Fragment& fragment : fragments)
	{
		switch (fragment.vertical_align)
		{
		case VerticalAlignType::Top: fragment.position.y = fragment.max_ascent; break;
		case VerticalAlignType::Bottom: fragment.position.y = out_height_of_line - fragment.max_descent; break;
		case VerticalAlignType::Center: fragment.position.y = 0.5f * (fragment.max_ascent - fragment.max_descent + out_height_of_line); break;
		default:
		{
			RMLUI_ASSERT(!IsAlignedSubtreeRoot(fragment));
			const float aligned_subtree_baseline =
				(fragment.aligned_subtree_root < 0 ? max_ascent : fragments[fragment.aligned_subtree_root].position.y);
			fragment.position.y = aligned_subtree_baseline + fragment.baseline_offset;
		}
		break;
		}
	}

	is_vertically_positioned = true;

	return new_line_box;
}

bool LineBox::ResolveBidiLine(Element* offset_parent, Vector<Vector<BidiPlacement>>& placements)
{
	struct Item {
		size_t fragment_index, begin, end;
		InlineLevelBox_Text* box;
		FontFaceHandle face;
	};
	Vector<Item> items;
	Vector<size_t> atomic_fragments;
	struct OrderedPiece { size_t fragment_index, piece_index; };
	Vector<OrderedPiece> ordered;
	const TextParagraphCollector::Group* paragraph = nullptr;
	for (size_t i = 0; i < fragments.size(); ++i) {
		const Fragment& fragment = fragments[i];
		if (fragment.type == FragmentType::SizedBox) {
			const auto* current = fragment.box->GetParagraphGroup();
			if (!current || (paragraph && paragraph != current)) return false;
			paragraph = current;
			atomic_fragments.push_back(i);
			continue;
		}
		if (fragment.type == FragmentType::InlineBox) {
			if (fragment.children_end_index == (int)i + 1) {
				const auto* current = fragment.box->GetParagraphGroup();
				if (!current || (paragraph && paragraph != current)) return false;
				paragraph = current;
				atomic_fragments.push_back(i);
			}
			continue;
		}
		if (fragment.type != FragmentType::TextRun) continue;
		auto* box = static_cast<InlineLevelBox_Text*>(fragment.box);
		const auto* current = box->GetParagraph();
		const auto* span = box->GetParagraphSpan();
		if (!current || !span || (paragraph && paragraph != current)) return false;
		paragraph = current;
		const auto& source = box->GetFragment(fragment.fragment_handle);
		if (source.text.empty()) { atomic_fragments.push_back(i); continue; }
		if (source.source_begin < 0 || source.source_end < source.source_begin ||
			(size_t)source.source_end >= span->mapping.source_to_paragraph.size()) return false;
		size_t begin = 0;
		if (!paragraph->paragraph.MatchFragment(span->mapping, source.source_begin, source.source_end, source.text, begin))
			return false;
		items.push_back({i, begin, begin + source.text.size(), box, box->GetTextElement()->GetFontFaceHandle()});
	}
	if (!paragraph) return false;
	for (size_t index : atomic_fragments) {
		Element* element = fragments[index].box->GetLayoutElement();
		bool found = false;
		for (const auto& object : paragraph->objects) {
			if (object.element == element) {
				items.push_back({index, object.begin, object.end, nullptr, 0});
				found = true;
				break;
			}
		}
		if (!found) return false;
	}
	std::sort(items.begin(), items.end(), [](const Item& a, const Item& b) { return a.fragment_index < b.fragment_index; });
	if (items.empty()) return false;
	if (!paragraph->shared_text) return false;
	const String& logical = *paragraph->shared_text;
	Vector<TextBidiRun> runs;
	const size_t line_begin = items.front().begin;
	const size_t line_end = items.back().end;
	auto* engine = GetFontEngineInterface();
	if (!engine || line_begin >= line_end || !engine->ResolveTextBidi(logical, offset_parent->GetComputedValues().direction(),
		line_begin, line_end - line_begin, runs)) return false;

	placements.resize(fragments.size());
	size_t covered_bytes = 0;
	for (const TextBidiRun& run : runs) {
		Vector<size_t> in_run;
		auto first = std::lower_bound(items.begin(), items.end(), run.offset,
			[](const Item& item, size_t offset) { return item.end <= offset; });
		for (auto it = first; it != items.end() && it->begin < run.offset + run.length; ++it)
			in_run.push_back(size_t(it - items.begin()));
		if (run.direction == Style::Direction::Rtl) std::reverse(in_run.begin(), in_run.end());
		for (size_t item_index : in_run) {
			const Item& item = items[item_index];
			const size_t begin = Math::Max(item.begin, run.offset);
			const size_t end = Math::Min(item.end, run.offset + run.length);
			if (begin >= end) continue;
			if (!item.box) {
				if (begin != item.begin || end != item.end) return false;
				const float width = fragments[item.fragment_index].layout_width;
				placements[item.fragment_index].push_back({nullptr, begin, run.direction, begin, end - begin, 0, end - begin, 0.f, width});
				ordered.push_back({item.fragment_index, placements[item.fragment_index].size() - 1});
				covered_bytes += end - begin;
				continue;
			}
			const auto& style = item.box->GetTextElement()->GetComputedValues();
			size_t shape_begin = begin, shape_end = end;
			const auto* span = item.box->GetParagraphSpan();
			const auto& paragraph_runs = paragraph->bidi_runs;
			auto paragraph_run = std::lower_bound(paragraph_runs.begin(), paragraph_runs.end(), begin,
				[](const TextBidiRun& candidate, size_t offset) { return candidate.offset + candidate.length <= offset; });
			if (paragraph_run != paragraph_runs.end() && paragraph_run->direction == run.direction &&
				paragraph_run->offset <= begin && paragraph_run->offset + paragraph_run->length >= end) {
				shape_begin = Math::Max(paragraph_run->offset, span->shaping_begin);
				shape_end = Math::Min(paragraph_run->offset + paragraph_run->length, span->shaping_end);
			}
			TextShapingContext context{style.language(), run.direction, style.font_kerning(), style.letter_spacing()};
			context.paragraph_text = logical;
			context.paragraph_owner = paragraph->shared_text;
			context.paragraph_offset = begin;
			context.direction_resolved = true;
			context.shaping_run_offset = shape_begin;
			context.shaping_run_length = shape_end - shape_begin;
			const float width = (float)engine->GetStringWidth(item.face, StringView(logical, begin, end - begin), context);
			placements[item.fragment_index].push_back({paragraph->shared_text, begin, run.direction, shape_begin, shape_end - shape_begin,
				begin - item.begin, end - begin, 0.f, width});
			ordered.push_back({item.fragment_index, placements[item.fragment_index].size() - 1});
			covered_bytes += end - begin;
		}
	}
	size_t expected_bytes = 0;
	for (const Item& item : items) expected_bytes += item.end - item.begin;
	if (covered_bytes != expected_bytes) { placements.clear(); return false; }
	struct InlineSegment { size_t fragment_index, first, last; float left_edge, right_edge; bool first_in_box, last_in_box; };
	Vector<InlineSegment> segments;
	Vector<float> before(ordered.size(), 0.f), after(ordered.size(), 0.f);
	Vector<Vector<size_t>> memberships(fragments.size()), segment_indices(fragments.size());
	for (size_t i = 0; i < ordered.size(); ++i) {
		const size_t child = ordered[i].fragment_index;
		FragmentIndex ancestor = fragments[child].type == FragmentType::InlineBox ? (FragmentIndex)child : fragments[child].parent;
		while (ancestor != RootFragmentIndex) {
			memberships[(size_t)ancestor].push_back(i);
			ancestor = fragments[(size_t)ancestor].parent;
		}
	}
	for (size_t parent_index = 0; parent_index < fragments.size(); ++parent_index) {
		const Fragment& parent = fragments[parent_index];
		if (parent.type != FragmentType::InlineBox || parent.children_end_index <= (int)parent_index) continue;
		const size_t first_segment = segments.size();
		const auto& members = memberships[parent_index];
		for (size_t i = 0; i < members.size();) {
			const size_t first = members[i++];
			size_t last = first;
			while (i < members.size() && members[i] == last + 1) last = members[i++];
			segment_indices[parent_index].push_back(segments.size());
			segments.push_back({parent_index, first, last, 0.f, 0.f, false, false});
		}
		if (segments.size() != first_segment) {
			InlineSegment& first = segments[first_segment];
			InlineSegment& last = segments.back();
			first.first_in_box = true;
			last.last_in_box = true;
			first.left_edge = parent.split_left ? 0.f : parent.box->GetSpacingLeft();
			last.right_edge = parent.split_right ? 0.f : parent.box->GetSpacingRight();
			before[first.first] += first.left_edge;
			after[last.last] += last.right_edge;
		}
	}
	float cursor = FLT_MAX;
	for (const Fragment& fragment : fragments)
		if (fragment.type == FragmentType::TextRun || fragment.type == FragmentType::SizedBox || fragment.type == FragmentType::InlineBox)
			cursor = Math::Min(cursor, fragment.position.x);
	for (size_t i = 0; i < ordered.size(); ++i) {
		cursor += before[i];
		auto& piece = placements[ordered[i].fragment_index][ordered[i].piece_index];
		piece.position_x = cursor;
		cursor += piece.width + after[i];
	}
	for (const Item& item : items) {
		Fragment& fragment = fragments[item.fragment_index];
		float left = FLT_MAX, right = -FLT_MAX;
		for (const auto& piece : placements[item.fragment_index]) {
			left = Math::Min(left, piece.position_x);
			right = Math::Max(right, piece.position_x + piece.width);
		}
		if (left <= right) { fragment.position.x = left; fragment.layout_width = right - left; }
	}
	box_cursor = cursor;
	for (const InlineSegment& segment : segments) {
		Fragment& parent = fragments[segment.fragment_index];
		const auto& first = placements[ordered[segment.first].fragment_index][ordered[segment.first].piece_index];
		const auto& last = placements[ordered[segment.last].fragment_index][ordered[segment.last].piece_index];
		float left = first.position_x - before[segment.first];
		float right = last.position_x + last.width + after[segment.last];
		for (FragmentIndex ancestor = parent.parent; ancestor != RootFragmentIndex; ancestor = fragments[(size_t)ancestor].parent) {
			for (size_t index : segment_indices[(size_t)ancestor]) {
				const InlineSegment& outer = segments[index];
				if (outer.first == segment.first) left += outer.left_edge;
				if (outer.last == segment.last) right -= outer.right_edge;
			}
		}
		const bool left_split = parent.split_left || !segment.first_in_box;
		const bool right_split = parent.split_right || !segment.last_in_box;
		placements[segment.fragment_index].push_back({nullptr, 0, Style::Direction::Auto, 0, 0, 0, 0,
			left, Math::Max(0.f, right - left - segment.left_edge - segment.right_edge), left_split, right_split});
	}
	// Empty inline boxes contributed a zero-width object for bidi ordering;
	// only their generated box segments are submitted to InlineBox.
	for (size_t index : atomic_fragments)
		if ((fragments[index].type == FragmentType::InlineBox || fragments[index].type == FragmentType::TextRun) && !placements[index].empty())
			placements[index].erase(placements[index].begin());
	return true;
}

void LineBox::Close(Element* offset_parent, Vector2f offset_parent_position, Style::TextAlign text_align)
{
	RMLUI_ASSERT(is_vertically_positioned && !is_closed);
	Vector<Vector<BidiPlacement>> bidi_placements;
	if (!ResolveBidiLine(offset_parent, bidi_placements)) bidi_placements.clear();

	// Horizontal alignment using available space on our line.
	if (box_cursor < line_width)
	{
		switch (text_align)
		{
		case Style::TextAlign::Center: offset_horizontal_alignment = (line_width - box_cursor) * 0.5f; break;
		case Style::TextAlign::Right: offset_horizontal_alignment = (line_width - box_cursor); break;
		case Style::TextAlign::Left:    // Already left-aligned.
		case Style::TextAlign::Justify: // Justification occurs at the text level.
			break;
		}
	}

	// Position and size all inline-level boxes, place geometry boxes.
	for (size_t fragment_index = 0; fragment_index < fragments.size(); ++fragment_index)
	{
		const Fragment& fragment = fragments[fragment_index];
		// Skip inline boxes which have not been closed (moved down to next line).
		if (fragment.type == FragmentType::InlineBox && fragment.children_end_index == 0)
			continue;

		RMLUI_ASSERT(fragment.layout_width >= 0.f);

		PlacedFragment placed_fragment = {
			offset_parent,
			fragment.fragment_handle,
			line_position - offset_parent_position + fragment.position + Vector2f(offset_horizontal_alignment, 0.f),
			fragment.layout_width,
			fragment.split_left,
			fragment.split_right,
		};
		if ((fragment.type == FragmentType::TextRun || fragment.type == FragmentType::InlineBox) &&
			fragment_index < bidi_placements.size() && !bidi_placements[fragment_index].empty()) {
			Vector<PlacedFragment> pieces;
			for (const auto& bidi : bidi_placements[fragment_index]) {
				PlacedFragment piece = placed_fragment;
				piece.position.x = line_position.x - offset_parent_position.x + bidi.position_x + offset_horizontal_alignment;
				piece.layout_width = bidi.width;
				piece.paragraph = bidi.paragraph;
				piece.paragraph_offset = bidi.offset;
				piece.direction = bidi.direction;
				piece.shaping_run_offset = bidi.shaping_run_offset;
				piece.shaping_run_length = bidi.shaping_run_length;
				piece.text_offset = bidi.text_offset;
				piece.text_length = bidi.text_length;
				if (fragment.type == FragmentType::InlineBox) {
					piece.split_left = bidi.split_left;
					piece.split_right = bidi.split_right;
				}
				pieces.push_back(piece);
			}
			if (fragment.type == FragmentType::TextRun) static_cast<InlineLevelBox_Text*>(fragment.box)->SubmitSlices(pieces);
			else static_cast<InlineBox*>(fragment.box)->SubmitSlices(pieces);
		} else {
			fragment.box->Submit(placed_fragment);
		}
	}

	is_closed = true;
}

void LineBox::VerticallyAlignSubtree(const int subtree_root_index, const int children_end_index, float& max_ascent, float& max_descent)
{
	const int children_begin_index = subtree_root_index + 1;

	// Iterate all descendant fragments which belong to our aligned subtree.
	for (int i = children_begin_index; i < children_end_index; i++)
	{
		Fragment& fragment = fragments[i];
		if (fragment.aligned_subtree_root != subtree_root_index)
			continue;

		// Position the baseline of fragments relative to their subtree root.
		const float parent_absolute_baseline = (fragment.parent < 0 ? 0.f : fragments[fragment.parent].baseline_offset);
		fragment.baseline_offset = parent_absolute_baseline + fragment.box->GetVerticalOffsetFromParent();

		// Expand this aligned subtree's height based on the height contributions of its descendants.
		if (fragment.type != FragmentType::TextRun)
		{
			max_ascent = Math::Max(max_ascent, fragment.box->GetHeightAboveBaseline() - fragment.baseline_offset);
			max_descent = Math::Max(max_descent, fragment.box->GetDepthBelowBaseline() + fragment.baseline_offset);
		}
	}
}

InlineBox* LineBox::GetOpenInlineBox()
{
	if (open_fragments_leaf == RootFragmentIndex)
		return nullptr;

	return rmlui_static_cast<InlineBox*>(fragments[open_fragments_leaf].box);
}

bool LineBox::CanCollapseLine() const
{
	// Roughly, collapse lines with only empty text fragments or inline boxes not taking up any width or spacing.
	for (const Fragment& fragment : fragments)
	{
		if (fragment.layout_width > 0.f)
			return false;
		else if (fragment.type == FragmentType::SizedBox)
			return false;
		else if (fragment.type == FragmentType::InlineBox && fragment.children_end_index > 0)
		{
			const bool any_spacing_left = (!fragment.split_left && fragment.box->GetSpacingLeft() != 0.f);
			const bool any_spacing_right = (!fragment.split_right && fragment.box->GetSpacingRight() != 0.f);
			if (any_spacing_left || any_spacing_right)
				return false;
		}
	}
	return true;
}

float LineBox::GetExtentRight() const
{
	RMLUI_ASSERT(is_closed);
	return box_cursor + offset_horizontal_alignment;
}

float LineBox::GetBaseline() const
{
	RMLUI_ASSERT(is_closed);
	return total_height_above_baseline;
}

String LineBox::DebugDumpTree(int depth) const
{
	const String value = String(depth * 2, ' ') + "LineBox (" + ToString(fragments.size()) + " fragment" + (fragments.size() == 1 ? "" : "s") + ")\n";
	return value;
}

void* LineBox::operator new(size_t size)
{
	return LayoutPools::AllocateLayoutChunk(size);
}

void LineBox::operator delete(void* chunk, size_t size)
{
	LayoutPools::DeallocateLayoutChunk(chunk, size);
}

} // namespace Rml
