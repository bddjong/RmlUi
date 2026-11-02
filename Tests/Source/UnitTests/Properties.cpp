#include "../Common/TestsInterface.h"
#include <RmlUi/Core/Animation.h>
#include <RmlUi/Core/ComputedValues.h>
#include <RmlUi/Core/TypeConverter.h>
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/DecorationTypes.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/PropertyDictionary.h>
#include <RmlUi/Core/StyleSheetSpecification.h>
#include <RmlUi/Core/StyleSheetTypes.h>
#include <doctest.h>
#include <limits>
#include <cmath>

using namespace Rml;

TEST_CASE("Properties")
{
	const Vector2i window_size(1024, 768);

	TestsSystemInterface system_interface;
	TestsRenderInterface render_interface;

	SetRenderInterface(&render_interface);
	SetSystemInterface(&system_interface);

	Rml::Initialise();

	Context* context = Rml::CreateContext("main", window_size);
	ElementDocument* document = context->CreateDocument();

	SUBCASE("inset")
	{
		struct InsetTestCase {
			String inset_value;

			struct ExpectedValues {
				String top;
				String right;
				String bottom;
				String left;
			} expected;
		};

		InsetTestCase tests[] = {
			{"auto", {"auto", "auto", "auto", "auto"}},
			{"0", {"0px", "0px", "0px", "0px"}},
			{"1px", {"1px", "1px", "1px", "1px"}},
			{"1dp", {"1dp", "1dp", "1dp", "1dp"}},
			{"1%", {"1%", "1%", "1%", "1%"}},
			{"10px 20px", {"10px", "20px", "10px", "20px"}},
			{"10px 20px 30px", {"10px", "20px", "30px", "20px"}},
			{"10px 20px 30px 40px", {"10px", "20px", "30px", "40px"}},
		};

		for (const InsetTestCase& test : tests)
		{
			CHECK(document->SetProperty("inset", test.inset_value));

			CHECK(document->GetProperty("top")->ToString() == test.expected.top);
			CHECK(document->GetProperty("right")->ToString() == test.expected.right);
			CHECK(document->GetProperty("bottom")->ToString() == test.expected.bottom);
			CHECK(document->GetProperty("left")->ToString() == test.expected.left);
		}
	}

	SUBCASE("flex")
	{
		struct FlexTestCase {
			String flex_value;

			struct ExpectedValues {
				float flex_grow;
				float flex_shrink;
				String flex_basis;
			} expected;
		};

		FlexTestCase tests[] = {
			{"", {0.f, 1.f, "auto"}},
			{"none", {0.f, 0.f, "auto"}},
			{"auto", {1.f, 1.f, "auto"}},
			{"1", {1.f, 1.f, "0px"}},
			{"2", {2.f, 1.f, "0px"}},
			{"2 0", {2.f, 0.f, "0px"}},
			{"2 3", {2.f, 3.f, "0px"}},
			{"2 auto", {2.f, 1.f, "auto"}},
			{"2 0 auto", {2.f, 0.f, "auto"}},
			{"0 0 auto", {0.f, 0.f, "auto"}},
			{"0 0 50px", {0.f, 0.f, "50px"}},
			{"0 0 50px", {0.f, 0.f, "50px"}},
			{"0 0 0", {0.f, 0.f, "0px"}},
		};

		for (const FlexTestCase& test : tests)
		{
			if (!test.flex_value.empty())
			{
				CHECK(document->SetProperty("flex", test.flex_value));
			}

			CHECK(document->GetProperty<float>("flex-grow") == test.expected.flex_grow);
			CHECK(document->GetProperty<float>("flex-shrink") == test.expected.flex_shrink);
			CHECK(document->GetProperty("flex-basis")->ToString() == test.expected.flex_basis);
		}
	}

	SUBCASE("gradient")
	{
		auto ParseGradient = [&](const String& value) -> Property {
			document->SetProperty("decorator", "linear-gradient(" + value + ")");
			auto decorators = document->GetProperty<DecoratorsPtr>("decorator");
			if (!decorators || decorators->list.size() != 1)
				return {};
			for (auto& id_property : decorators->list.front().properties.GetProperties())
			{
				if (id_property.second.unit == Unit::COLORSTOPLIST)
					return id_property.second;
			}
			return {};
		};

		struct GradientTestCase {
			String value;
			String expected_parsed_string;
			ColorStopList expected_color_stops;
		};

		GradientTestCase test_cases[] = {
			{
				"red, blue",
				"#ff0000, #0000ff",
				{
					ColorStop{ColourbPremultiplied(255, 0, 0), NumericValue{}},
					ColorStop{ColourbPremultiplied(0, 0, 255), NumericValue{}},
				},
			},
			{
				"red 5px, blue 50%",
				"#ff0000 5px, #0000ff 50%",
				{
					ColorStop{ColourbPremultiplied(255, 0, 0), NumericValue{5.f, Unit::PX}},
					ColorStop{ColourbPremultiplied(0, 0, 255), NumericValue{50.f, Unit::PERCENT}},
				},
			},
			{
				"red, #00f 50%, rgba(0, 255,0, 150) 10dp",
				"#ff0000, #0000ff 50%, #00ff0096 10dp",
				{
					ColorStop{ColourbPremultiplied(255, 0, 0), NumericValue{}},
					ColorStop{ColourbPremultiplied(0, 0, 255), NumericValue{50.f, Unit::PERCENT}},
					ColorStop{ColourbPremultiplied(0, 150, 0, 150), NumericValue{10.f, Unit::DP}},
				},
			},
			{
				"hsl(10000, 0%, 50%), hsl(240, 100%, 50%) 50%, hsla(-240, 100%, 50%, 0.5) 10dp",
				"#7f7f7f, #0000ff 50%, #00ff007f 10dp",
				{
					ColorStop{ColourbPremultiplied(127, 127, 127), NumericValue{}},
					ColorStop{ColourbPremultiplied(0, 0, 255), NumericValue{50.f, Unit::PERCENT}},
					ColorStop{ColourbPremultiplied(0, 127, 0, 127), NumericValue{10.f, Unit::DP}},
				},
			},
			{
				"lab(55% none none), lab(30% 67% -110) 50%, lab(90% -90 80 / 0.5) 10dp",
				"#838383, #0000fb 50%, #00ff267f 10dp",
				{
					ColorStop{ColourbPremultiplied(131, 131, 131), NumericValue{}},
					ColorStop{ColourbPremultiplied(0, 0, 251), NumericValue{50.f, Unit::PERCENT}},
					ColorStop{ColourbPremultiplied(0, 127, 19, 127), NumericValue{10.f, Unit::DP}},
				},
			},
			{
				"lch(55% none 300.0), lch(30% 85% 180.0) 50%, lch(90% 90 100.0 / 50%) 10dp",
				"#838383, #006044 50%, #f0e6007f 10dp",
				{
					ColorStop{ColourbPremultiplied(131, 131, 131), NumericValue{}},
					ColorStop{ColourbPremultiplied(0, 96, 68), NumericValue{50.f, Unit::PERCENT}},
					ColorStop{ColourbPremultiplied(120, 115, 0, 127), NumericValue{10.f, Unit::DP}},
				},
			},
			{
				"oklab(85% -50% 70%), oklab(0.2 0.4 -0.4) 50%, oklab(100% none 0.4 / 0.25) 10dp",
				"#98ed00, #6600c1 50%, #ffde003f 10dp",
				{
					ColorStop{ColourbPremultiplied(152, 237, 0), NumericValue{}},
					ColorStop{ColourbPremultiplied(102, 0, 193), NumericValue{50.f, Unit::PERCENT}},
					ColorStop{ColourbPremultiplied(63, 55, 0, 63), NumericValue{10.f, Unit::DP}},
				},
			},
			{
				"oklch(75% 100% 30.0), oklch(0.5 0.2 270) 50%, oklch(1.0 0.1 none / 0.5) 10dp",
				"#ff0000, #3a50d2 50%, #ffe2fa7f 10dp",
				{
					ColorStop{ColourbPremultiplied(255, 0, 0), NumericValue{}},
					ColorStop{ColourbPremultiplied(58, 80, 210), NumericValue{50.f, Unit::PERCENT}},
					ColorStop{ColourbPremultiplied(127, 113, 125, 127), NumericValue{10.f, Unit::DP}},
				},
			},
			{
				"red 50px 20%, blue 10in",
				"#ff0000 50px, #ff0000 20%, #0000ff 10in",
				{
					ColorStop{ColourbPremultiplied(255, 0, 0), NumericValue{50.f, Unit::PX}},
					ColorStop{ColourbPremultiplied(255, 0, 0), NumericValue{20.f, Unit::PERCENT}},
					ColorStop{ColourbPremultiplied(0, 0, 255), NumericValue{10.f, Unit::INCH}},
				},
			},
		};

		for (const GradientTestCase& test_case : test_cases)
		{
			const Property result = ParseGradient(test_case.value);
			CHECK(result.ToString() == test_case.expected_parsed_string);
			CHECK(result.Get<ColorStopList>() == test_case.expected_color_stops);
		}
	}

	Rml::Shutdown();
}

TEST_CASE("Property.ToString")
{
	TestsSystemInterface system_interface;
	TestsRenderInterface render_interface;
	SetRenderInterface(&render_interface);
	SetSystemInterface(&system_interface);

	Rml::Initialise();

	CHECK(Property(5.2f, Unit::CM).ToString() == "5.2cm");
	CHECK(Property(150, Unit::PERCENT).ToString() == "150%");
	CHECK(Property(Colourb{170, 187, 204, 255}, Unit::COLOUR).ToString() == "#aabbcc");

	auto ParsedValue = [](const String& name, const String& value) -> String {
		PropertyDictionary properties;
		StyleSheetSpecification::ParsePropertyDeclaration(properties, name, value);
		REQUIRE(properties.GetNumProperties() == 1);
		return properties.GetProperties().begin()->second.ToString();
	};

	CHECK(ParsedValue("width", "10px") == "10px");
	CHECK(ParsedValue("width", "10.00em") == "10em");
	CHECK(ParsedValue("width", "auto") == "auto");

	CHECK(ParsedValue("background-color", "#abc") == "#aabbcc");
	CHECK(ParsedValue("background-color", "red") == "#ff0000");

	CHECK(ParsedValue("transform", "translateX(10px)") == "translateX(10px)");
	CHECK(ParsedValue("transform", "translate(20in, 50em)") == "translate(20in, 50em)");

	CHECK(ParsedValue("box-shadow", "2px 2px 0px, rgba(0, 0, 255, 255) 4px 4px 2em") == "#000000 2px 2px 0px, #0000ff 4px 4px 2em");
	CHECK(ParsedValue("box-shadow", "2px 2px 0px, #00ff 4px 4px 2em") == "#000000 2px 2px 0px, #0000ff 4px 4px 2em");

	// Due to conversion to and from premultiplied alpha, some color information is lost.
	CHECK(ParsedValue("box-shadow", "#fff0 2px 2px 0px") == "#00000000 2px 2px 0px");

	CHECK(ParsedValue("decorator", "linear-gradient(110deg, #fff3, #fff 10%, #c33 250dp, #3c3, #33c, #000 90%, #0003) border-box") ==
		"linear-gradient(110deg, #fff3, #fff 10%, #c33 250dp, #3c3, #33c, #000 90%, #0003) border-box");

	CHECK(ParsedValue("filter", "drop-shadow(#000 30px 20px 5px) opacity(0.2) sepia(0.2)") ==
		"drop-shadow(#000 30px 20px 5px) opacity(0.2) sepia(0.2)");

	Rml::Shutdown();
}

TEST_CASE("Property conversion replaces output and preserves angle stops")
{
	AnimationList animations;
	String output = "stale";
	CHECK((TypeConverter<AnimationList, String>::Convert(animations, output)));
	CHECK(output.empty());
	ColorStopList stops = {ColorStop{ColourbPremultiplied(255, 0, 0), NumericValue{90.f, Unit::DEG}},
		ColorStop{ColourbPremultiplied(0, 0, 255), NumericValue{1.f, Unit::RAD}}};
	CHECK((TypeConverter<ColorStopList, String>::Convert(stops, output)));
	CHECK(output.find("90deg") != String::npos);
	CHECK(output.find("1rad") != String::npos);
}

TEST_CASE("Property import accepts zero specificity")
{
	PropertyDictionary source, destination;
	source.SetProperty(PropertyId::Width, Property(1.f, Unit::PX, 10));
	destination.Import(source, 0);
	REQUIRE(destination.GetProperty(PropertyId::Width));
	CHECK(destination.GetProperty(PropertyId::Width)->specificity == 0);
}

TEST_CASE("computed border defaults and radius bounds")
{
	Rml::Style::ComputedValues values(nullptr);
	CHECK(values.border_top_color() == Rml::Colourb(0, 0, 0, 255));
	values.border_top_left_radius(32768.f);
	CHECK(values.border_top_left_radius() == 32767.f);
	values.border_top_width(65535);
	CHECK(values.border_top_width() == 65535.f);
	const float nan = std::numeric_limits<float>::quiet_NaN();
	const float infinity = std::numeric_limits<float>::infinity();
	values.border_top_left_radius(nan);
	CHECK(values.border_top_left_radius() == 0.f);
	values.border_top_left_radius(infinity);
	CHECK(values.border_top_left_radius() == 32767.f);
	values.border_top_left_radius(-infinity);
	CHECK(values.border_top_left_radius() == 0.f);
}

TEST_CASE("parsed border lengths saturate computed values")
{
	TestsSystemInterface system_interface;
	TestsRenderInterface render_interface;
	SetRenderInterface(&render_interface);
	SetSystemInterface(&system_interface);
	Rml::Initialise();
	Context* context = Rml::CreateContext("border-bounds", Vector2i(1024, 768));
	ElementDocument* document = context->CreateDocument();

	struct Case { const char* input; float width; float radius; };
	const Case cases[] = {
		{"-32769px", 0.f, 0.f}, {"-0.5px", 0.f, 0.f}, {"0px", 0.f, 0.f},
		{"-0px", 0.f, 0.f}, {"0.1px", 1.f, 0.f},
		{"1.499px", 1.f, 1.f}, {"1.5px", 2.f, 1.f}, {"1.501px", 2.f, 1.f}, {"32768px", 32768.f, 32767.f},
		{"65534.49px", 65534.f, 32767.f}, {"65534.5px", 65535.f, 32767.f}, {"65534.51px", 65535.f, 32767.f},
		{"65535px", 65535.f, 32767.f}, {"65535.01px", 65535.f, 32767.f}, {"65536px", 65535.f, 32767.f},
		{"999999999999999999999999999999999999999999999px", 65535.f, 32767.f},
	};
	for (const Case& test : cases)
	{
		CAPTURE(test.input);
		REQUIRE(document->SetProperty("border-width", test.input));
		REQUIRE(document->SetProperty("border-radius", test.input));
		context->Update();
		const auto& values = document->GetComputedValues();
		CHECK(values.border_top_width() == test.width);
		CHECK(values.border_right_width() == test.width);
		CHECK(values.border_bottom_width() == test.width);
		CHECK(values.border_left_width() == test.width);
		CHECK(values.border_top_left_radius() == test.radius);
		CHECK(values.border_top_right_radius() == test.radius);
		CHECK(values.border_bottom_right_radius() == test.radius);
		CHECK(values.border_bottom_left_radius() == test.radius);
	}
	// RmlUi accepts negative lengths, but each computed radius must remain nonnegative.
	REQUIRE(document->SetProperty("border-radius", "-100px 10px 10px 10px"));
	context->Update();
	const auto& mixed = document->GetComputedValues();
	CHECK(mixed.border_top_left_radius() == 0.f);
	CHECK(mixed.border_top_right_radius() == 10.f);
	CHECK(mixed.border_bottom_right_radius() == 10.f);
	CHECK(mixed.border_bottom_left_radius() == 10.f);

	const float nan = std::numeric_limits<float>::quiet_NaN();
	const float infinity = std::numeric_limits<float>::infinity();
	REQUIRE(document->SetProperty(PropertyId::BorderTopLeftRadius, Property(nan, Unit::PX)));
	context->Update();
	CHECK(mixed.border_top_left_radius() == 0.f);
	CHECK(mixed.border_top_right_radius() == 10.f);
	CHECK(mixed.border_bottom_right_radius() == 10.f);
	CHECK(mixed.border_bottom_left_radius() == 10.f);
	for (const auto& test : {std::pair<float, float>{nan, 0.f}, {infinity, 65535.f}, {-infinity, 0.f}})
	{
		REQUIRE(document->SetProperty(PropertyId::BorderTopWidth, Property(test.first, Unit::PX)));
		context->Update();
		CHECK(document->GetComputedValues().border_top_width() == test.second);
	}
	REQUIRE(document->SetProperty("margin-left", "-12.5px"));
	REQUIRE(document->SetProperty("left", "-7.25px"));
	context->Update();
	CHECK(document->GetComputedValues().margin_left().type == Style::LengthPercentageAuto::Length);
	CHECK(document->GetComputedValues().margin_left().value == -12.5f);
	CHECK(document->GetComputedValues().left().type == Style::LengthPercentageAuto::Length);
	CHECK(document->GetComputedValues().left().value == -7.25f);
	context->SetDensityIndependentPixelRatio(2.f);
	REQUIRE(document->SetProperty("border-width", "1.25dp"));
	REQUIRE(document->SetProperty("border-radius", "1.25dp"));
	context->Update();
	CHECK(document->GetComputedValues().border_top_width() == 3.f);
	CHECK(document->GetComputedValues().border_top_left_radius() == 2.f);
	document->Close();
	Rml::Shutdown();
}

TEST_CASE("border radius geometry preserves corners and normalizes proportionally")
{
	class GeometryCapture : public TestsRenderInterface {
	public:
		Vector<Vector2f> positions;
		CompiledGeometryHandle CompileGeometry(Span<const Vertex> vertices, Span<const int> indices) override
		{
			for (const Vertex& vertex : vertices)
				positions.push_back(vertex.position);
			return TestsRenderInterface::CompileGeometry(vertices, indices);
		}
	} render_interface;
	TestsSystemInterface system_interface;
	SetRenderInterface(&render_interface);
	SetSystemInterface(&system_interface);
	Rml::Initialise();
	Context* context = Rml::CreateContext("border-geometry", Vector2i(200, 200));
	ElementDocument* document = context->CreateDocument();
	REQUIRE(document->SetProperty("width", "100px"));
	REQUIRE(document->SetProperty("height", "100px"));
	REQUIRE(document->SetProperty("background-color", "white"));
	document->Show();

	auto has_point = [&](float x, float y) {
		for (const Vector2f& point : render_interface.positions)
			if (std::abs(point.x - x) < 0.01f && std::abs(point.y - y) < 0.01f)
				return true;
		return false;
	};
	auto render = [&](const char* radius) {
		render_interface.positions.clear();
		REQUIRE(document->SetProperty("border-radius", radius));
		context->Update();
		context->Render();
		REQUIRE(!render_interface.positions.empty());
	};

	render("-100px 10px 10px 10px");
	CHECK(has_point(0, 0));
	for (const auto& point : {Vector2f(90, 0), Vector2f(100, 10), Vector2f(100, 90),
		Vector2f(90, 100), Vector2f(10, 100), Vector2f(0, 90)})
		CHECK(has_point(point.x, point.y));
	CHECK_FALSE(has_point(100, 0));
	CHECK_FALSE(has_point(100, 100));
	CHECK_FALSE(has_point(0, 100));

	// Right side is limiting: 80 + 120 scales all radii by 1/2.
	render("40px 80px 120px 20px");
	for (const auto& point : {Vector2f(0, 20), Vector2f(20, 0), Vector2f(60, 0), Vector2f(100, 40),
		Vector2f(40, 100), Vector2f(10, 100), Vector2f(0, 90)})
		CHECK(has_point(point.x, point.y));
	CHECK_FALSE(has_point(0, 0));
	CHECK_FALSE(has_point(100, 0));
	CHECK_FALSE(has_point(100, 100));
	CHECK_FALSE(has_point(0, 100));
	document->Close();
	Rml::Shutdown();
}
