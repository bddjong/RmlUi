#include "FontEffectOutline.h"
#include "FontEffectLimits.h"
#include "../../Include/RmlUi/Core/PropertyDefinition.h"

namespace Rml {

FontEffectOutline::FontEffectOutline()
{
	width = 0;
	SetLayer(Layer::Back);
}

FontEffectOutline::~FontEffectOutline() {}

bool FontEffectOutline::HasUniqueTexture() const
{
	return true;
}

bool FontEffectOutline::Initialise(int _width)
{
	if (_width <= 0 || _width > FontEffectLimits::max_width)
		return false;

	width = _width;

	filter.Initialise(width, FilterOperation::Dilation);
	for (int x = -width; x <= width; ++x)
	{
		for (int y = -width; y <= width; ++y)
		{
			float weight = 1;

			float distance = Math::SquareRoot(float(x * x + y * y));
			if (distance > width)
			{
				weight = (width + 1) - distance;
				weight = Math::Max(weight, 0.0f);
			}

			filter[x + width][y + width] = weight;
		}
	}

	return true;
}

bool FontEffectOutline::GetGlyphMetrics(Vector2i& origin, Vector2i& dimensions, const FontGlyph& /*glyph*/) const
{
	return FontEffectLimits::ExpandMetrics(origin, dimensions, width);
}

void FontEffectOutline::GenerateGlyphTexture(byte* destination_data, const Vector2i destination_dimensions, int destination_stride,
	const FontGlyph& glyph) const
{
	filter.Run(destination_data, destination_dimensions, destination_stride, ColorFormat::RGBA8, glyph.bitmap_data, glyph.bitmap_dimensions,
		Vector2i(width), glyph.color_format);

	FillColorValuesFromAlpha(destination_data, destination_dimensions, destination_stride);
}

FontEffectOutlineInstancer::FontEffectOutlineInstancer() : id_width(PropertyId::Invalid), id_color(PropertyId::Invalid)
{
	id_width = RegisterProperty("width", "1px", true).AddParser("length").GetId();
	id_color = RegisterProperty("color", "white", false).AddParser("color").GetId();
	RegisterShorthand("font-effect", "width, color", ShorthandType::FallThrough);
}

FontEffectOutlineInstancer::~FontEffectOutlineInstancer() {}

SharedPtr<FontEffect> FontEffectOutlineInstancer::InstanceFontEffect(const String& /*name*/, const PropertyDictionary& properties)
{
	float width = properties.GetProperty(id_width)->Get<float>();
	Colourb color = properties.GetProperty(id_color)->Get<Colourb>();

	int width_int;
	if (!FontEffectLimits::ConvertWidth(width, width_int))
		return nullptr;

	auto font_effect = MakeShared<FontEffectOutline>();
	if (font_effect->Initialise(width_int))
	{
		font_effect->SetColour(color);
		return font_effect;
	}

	return nullptr;
}

} // namespace Rml
