#pragma once

#include <cstddef>

// HarfBuzz cluster values are byte offsets into the original UTF-8 input.
// A cluster can produce any number of glyphs, including more than codepoints.
struct CenturiesClusterRange {
	std::size_t begin;
	std::size_t end;
	int glyph_count;
};

template <typename GlyphInfo>
CenturiesClusterRange centuries_cluster_range(const GlyphInfo* glyphs, int count, int index, std::size_t text_size) {
	const std::size_t begin = glyphs[index].cluster;
	if (begin >= text_size) return {text_size, text_size, 1};
	int glyph_count = 1;
	while (index + glyph_count < count && glyphs[index + glyph_count].cluster == begin) ++glyph_count;
	std::size_t end = text_size;
	for (int i = 0; i < count; ++i) {
		const std::size_t offset = glyphs[i].cluster;
		if (offset > begin && offset < end) end = offset;
	}
	return {begin, end, glyph_count};
}
