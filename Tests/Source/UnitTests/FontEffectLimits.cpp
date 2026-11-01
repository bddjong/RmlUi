#include "../Common/TestsShell.h"
#include "../../../Source/Core/FontEffectBlur.h"
#include "../../../Source/Core/FontEffectGlow.h"
#include "../../../Source/Core/FontEffectOutline.h"
#include <RmlUi/Core/Factory.h>
#include <RmlUi/Core/FontGlyph.h>
#include <RmlUi/Core/PropertyDefinition.h>
#include <RmlUi/Core/PropertyDictionary.h>
#include <RmlUi/Core/PropertySpecification.h>
#include <doctest.h>
#include <limits>

using namespace Rml;

TEST_CASE("font_effect.checked_lengths")
{
	REQUIRE(TestsShell::GetContext());
	for (const char* name : {"blur", "outline", "glow"})
	{
		auto* instancer = Factory::GetFontEffectInstancer(name);
		REQUIRE(instancer);
		const auto& spec = instancer->GetPropertySpecification();
		PropertyDictionary properties;
		spec.SetPropertyDefaults(properties);
		const char* width = String(name) == "glow" ? "width-outline" : "width";
		const auto id = spec.GetProperty(width)->GetId();
		properties.SetProperty(id, Property(2.f, Unit::PX));
		CHECK(instancer->InstanceFontEffect(name, properties));
		for (float value : {129.f, 1e30f, std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN()})
		{
			properties.SetProperty(id, Property(value, Unit::PX));
			CHECK_FALSE(instancer->InstanceFontEffect(name, properties));
		}
	}
	auto* glow = Factory::GetFontEffectInstancer("glow");
	const auto& spec = glow->GetPropertySpecification();
	PropertyDictionary properties;
	spec.SetPropertyDefaults(properties);
	CHECK(glow->InstanceFontEffect("glow", properties)); // -1 blur means outline width
	for (const char* name : {"width-blur", "offset-x", "offset-y"})
	{
		PropertyDictionary invalid = properties;
		invalid.SetProperty(spec.GetProperty(name)->GetId(), Property(1e30f, Unit::PX));
		CHECK_FALSE(glow->InstanceFontEffect("glow", invalid));
	}
	FontEffectBlur blur;
	FontEffectOutline outline;
	FontEffectGlow glow_effect;
	CHECK_FALSE(blur.Initialise(129));
	CHECK_FALSE(outline.Initialise(129));
	CHECK_FALSE(glow_effect.Initialise(129, 1, Vector2i(0)));
	REQUIRE(glow_effect.Initialise(1, 1, Vector2i(3, 0)));
	FontGlyph glyph;
	Vector2i origin(std::numeric_limits<int>::max(), 0), dimensions(10, 10);
	CHECK_FALSE(glow_effect.GetGlyphMetrics(origin, dimensions, glyph));
	CHECK(origin.x == std::numeric_limits<int>::max());
	TestsShell::ShutdownShell();
}
