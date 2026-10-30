#pragma once

#include "FontEngineDefault/FontTypes.h"
#include <RmlUi/Core.h>

using Rml::byte;
using Rml::FontFaceHandleFreetype;
using Rml::String;
using Rml::UniquePtr;
using Rml::Vector;
namespace Style = Rml::Style;

class FontFace;
class FontFaceHandleHarfBuzz;

/**
    Modified to support HarfBuzz text shaping.
 */

class FontFamily {
public:
	FontFamily(const String& name);
	~FontFamily();

	/// Returns a handle to the most appropriate font in the family, at the correct size.
	/// @param[in] style The style of the desired handle.
	/// @param[in] weight The weight of the desired handle.
	/// @param[in] size The size of desired handle, in points.
	/// @return A valid handle if a matching (or closely matching) font face was found, nullptr otherwise.
	FontFaceHandleHarfBuzz* GetFaceHandle(Style::FontStyle style, Style::FontWeight weight, int size);

	/// Adds a new face to the family.
	/// @param[in] ft_face The previously loaded FreeType face.
	/// @param[in] style The style of the new face.
	/// @param[in] weight The weight of the new face.
	/// @param[in,out] face_memory Font data ownership is transferred only when a new face is added.
	/// @return The new or existing matching face. Duplicate FreeType faces are released.
	FontFace* AddFace(FontFaceHandleFreetype ft_face, Style::FontStyle style, Style::FontWeight weight, UniquePtr<byte[]>& face_memory);

	/// Releases resources owned by sized font faces, including their textures and rendered glyphs.
	void ReleaseFontResources();

protected:
	String name;

	struct FontFaceEntry { UniquePtr<FontFace> face; };

	using FontFaceList = Vector<FontFaceEntry>;
	// Declared first so data remains alive until all faces and their HarfBuzz
	// handles are destroyed. Named variations may share the same buffer.
	Vector<UniquePtr<byte[]>> font_data;
	FontFaceList font_faces;
};
