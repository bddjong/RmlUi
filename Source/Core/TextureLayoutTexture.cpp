#include "TextureLayoutTexture.h"
#include "TextureDatabase.h"
#include "TextureLayout.h"
#include <cmath>

namespace Rml {

TextureLayoutTexture::TextureLayoutTexture() : dimensions(0, 0) {}

TextureLayoutTexture::~TextureLayoutTexture()
{
	// Don't free texture data; freed in the texture loader.
}

Vector2i TextureLayoutTexture::GetDimensions() const
{
	return dimensions;
}

int TextureLayoutTexture::Generate(TextureLayout& layout, int maximum_dimensions)
{
	// Come up with an estimate for how big a texture we need. Calculate the total square pixels
	// required by the remaining rectangles to place, square-root it to get the dimensions of the
	// smallest texture necessary (under optimal circumstances) and round it up to the nearest
	// power of two.
	double square_pixels = 0;
	if (maximum_dimensions <= 0)
		return 0;
	int unplaced_rectangles = 0;
	for (int i = 0; i < layout.GetNumRectangles(); ++i)
	{
		const TextureLayoutRectangle& rectangle = layout.GetRectangle(i);

		if (!rectangle.IsPlaced())
		{
			const double x = double(rectangle.GetDimensions().x) + 1;
			const double y = double(rectangle.GetDimensions().y) + 1;

			square_pixels += x * y;
			++unplaced_rectangles;
		}
	}

	const double texture_width = std::sqrt(square_pixels);

	// Avoid converting or rounding an estimate beyond the usable texture size.
	if (texture_width >= maximum_dimensions || texture_width >= (1 << 30))
		dimensions.y = maximum_dimensions;
	else
		dimensions.y = Math::ToPowerOfTwo(int(texture_width));
	dimensions.x = dimensions.y >> 1;

	dimensions.x = Math::Min(dimensions.x, maximum_dimensions);
	dimensions.y = Math::Min(dimensions.y, maximum_dimensions);

	// Now we're layout out the rectangles in the texture. If we don't fit all the rectangles on
	// and have room to grow (ie, haven't hit the maximum texture size in both dimensions) then
	// we'll have another go with a bigger texture.
	int num_placed_rectangles = 0;
	for (;;)
	{
		bool success = true;
		int height = 1;

		while (num_placed_rectangles != unplaced_rectangles)
		{
			TextureLayoutRow row;
			int row_size = row.Generate(layout, dimensions.x, height);
			if (row_size == 0)
			{
				success = false;
				break;
			}

			height += row.GetHeight() + 1;
			if (height > dimensions.y)
			{
				// D'oh! We've exceeded our height boundaries. This row should be unplaced.
				row.Unplace();
				success = false;
				break;
			}

			rows.push_back(row);
			num_placed_rectangles += row_size;
		}

		// If the rectangles were successfully laid out within the texture limits, we're done.
		if (success)
			return num_placed_rectangles;

		// Couldn't do it! Increase the texture size, clear the rectangles and try again - unless
		// we've hit the maximum texture size, in which case return true if we've placed any
		// rectangles (ie, the layout isn't empty).
		if (dimensions.y > dimensions.x)
			dimensions.x = dimensions.y;
		else
		{
			if (dimensions.y > maximum_dimensions / 2)
				return num_placed_rectangles;

			dimensions.y <<= 1;
		}

		// Unplace all of the glyphs we tried to place and have an other crack.
		for (size_t i = 0; i < rows.size(); i++)
			rows[i].Unplace();

		rows.clear();
		num_placed_rectangles = 0;
	}
}

Vector<byte> TextureLayoutTexture::AllocateTexture()
{
	Vector<byte> texture_data;

	if (dimensions.x > 0 && dimensions.y > 0)
	{
		// Set the texture to transparent black.
		texture_data.resize(dimensions.x * dimensions.y * 4, 0);

		for (size_t i = 0; i < rows.size(); ++i)
			rows[i].Allocate(texture_data.data(), dimensions.x * 4);
	}

	return texture_data;
}

} // namespace Rml
