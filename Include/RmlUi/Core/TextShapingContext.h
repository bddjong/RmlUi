#pragma once

#include "StyleTypes.h"
#include "StringUtilities.h"
#include "Types.h"

namespace Rml {

/*
    Data extracted from the properties of an element to help provide context for text shaping and spacing.
*/
struct TextShapingContext {
	const String& language;
	Style::Direction text_direction = Style::Direction::Auto;
	Style::FontKerning font_kerning = Style::FontKerning::Auto;
	float letter_spacing = 0.0f; // Measured in pixels.

	// Optional logical paragraph context. When present, the string passed to
	// the font engine is the byte range beginning at paragraph_offset in this
	// UTF-8 text. It stays valid for the duration of the font-engine call.
	// Shapers may use the surrounding text to select contextual glyph forms.
	StringView paragraph_text = {};
	// When available, retain the immutable paragraph owner so font engines can
	// safely cache shaped runs by paragraph identity rather than copying text.
	SharedPtr<const String> paragraph_owner = {};
	size_t paragraph_offset = 0;

	// True when text_direction is the resolved bidi direction of this run.
	// A font engine must not resolve the run as a new paragraph in this case.
	bool direction_resolved = false;

	// Optional larger shaping run in paragraph_text. The engine shapes this
	// run together, then emits clusters whose first byte falls within the
	// requested string's paragraph range. This preserves ligatures across
	// purely visual style boundaries while assigning each glyph one owner.
	size_t shaping_run_offset = 0;
	size_t shaping_run_length = 0;
};

// A resolved visual run. Offsets and lengths refer to UTF-8 bytes in the
// original logical paragraph, not to positions in the visual sequence.
struct TextBidiRun {
	size_t offset = 0;
	size_t length = 0;
	Style::Direction direction = Style::Direction::Ltr;
};

} // namespace Rml
