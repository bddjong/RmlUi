#include "../Common/TestsShell.h"
#include "../../../Source/Core/ElementDefinition.h"
#include "../../../Source/Core/DecoratorTiledVertical.h"
#include "../../../Source/Core/TextureLayout.h"
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Decorator.h>
#include <RmlUi/Core/PropertyDefinition.h>
#include <RmlUi/Core/Texture.h>
#include <RmlUi/Core/URL.h>
#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Factory.h>
#include <RmlUi/Core/ID.h>
#include <RmlUi/Core/Spritesheet.h>
#include <RmlUi/Core/StreamMemory.h>
#include <RmlUi/Core/StyleSheet.h>
#include <RmlUi/Core/StyleSheetContainer.h>
#include <doctest.h>

static const char spritesheet[] = R"(
@spritesheet test_sheet {
	src: /assets/high_scores_alien_3.tga;
	test00: 0px 0px 64px 64px;
	test01: 64px 0px 64px 64px;
	test10: 0px 64px 64px 64px;
	test11: 64px 64px 64px 64px;
	resolution: 1x;
}
)";

static const char spritesheet_with_path_string_encoding[] = R"(
@spritesheet test_sheet_with_path_string_encoding {
	src: "/assets/test.tga";
	test00: 0px 0px 128px 128px;
	test01: 128px 0px 128px 128px;
	test10: 0px 128px 128px 128px;
	test11: 128px 128px 128px 128px;
	resolution: 2x;
}
)";

static const char comments[] = R"(
/* A single line comment */
/* A multiline
   comment */
/** A comment starting with two '*' */
/* A comment ending with two '*' **/
/**
 * A multiline comment
 * starting with two '*'
 * and ending with two '*'
 **/
@spritesheet test_sheet {
	src: /assets/high_scores_alien_3.tga;
	/* A comment in-between rules */
	test00: 0px 0px 64px 64px;
	test01: 64px 0px 64px 64px;
	test10: 0px 64px 64px 64px; /* A comment after a rule */
	test11: 64px 64px 64px 64px;
	/* A comment before a rule */ resolution: 1x;
}
)";

static const char selector_list_with_escaped_comma[] = R"(
.escaped\,class, #normal_match {
	width: 10px;
}
)";

using namespace Rml;

TEST_CASE("style_sheet_parser.spritesheet")
{
	Context* context = TestsShell::GetContext();

	{
		StyleSheetContainer style_sheet_container;
		StreamMemory spritesheet_stream{reinterpret_cast<const byte*>(spritesheet), sizeof(spritesheet) - 1};
		style_sheet_container.LoadStyleSheetContainer(&spritesheet_stream, 0);

		style_sheet_container.UpdateCompiledStyleSheet(context);
		const auto* style_sheet = style_sheet_container.GetCompiledStyleSheet();
		CHECK(style_sheet != nullptr);

		const auto* sprite00 = style_sheet->GetSprite("test00");
		CHECK(sprite00 != nullptr);
		CHECK(sprite00->sprite_sheet->name == "test_sheet");
		CHECK(sprite00->sprite_sheet->texture_source.GetSource() == "/assets/high_scores_alien_3.tga");
		CHECK(sprite00->rectangle.TopLeft() == Vector2f(0.f, 0.f));
		CHECK(sprite00->rectangle.BottomRight() == Vector2f(64.f, 64.f));
		CHECK(sprite00->sprite_sheet->display_scale == 1.f);

		const auto* sprite01 = style_sheet->GetSprite("test01");
		CHECK(sprite01 != nullptr);
		CHECK(sprite01->sprite_sheet->name == "test_sheet");
		CHECK(sprite01->sprite_sheet->texture_source.GetSource() == "/assets/high_scores_alien_3.tga");
		CHECK(sprite01->rectangle.TopLeft() == Vector2f(64.f, 0.f));
		CHECK(sprite01->rectangle.BottomRight() == Vector2f(128.f, 64.f));
		CHECK(sprite01->sprite_sheet->display_scale == 1.f);

		const auto* sprite10 = style_sheet->GetSprite("test10");
		CHECK(sprite10 != nullptr);
		CHECK(sprite10->sprite_sheet->name == "test_sheet");
		CHECK(sprite10->sprite_sheet->texture_source.GetSource() == "/assets/high_scores_alien_3.tga");
		CHECK(sprite10->rectangle.TopLeft() == Vector2f(0.f, 64.f));
		CHECK(sprite10->rectangle.BottomRight() == Vector2f(64.f, 128.f));
		CHECK(sprite10->sprite_sheet->display_scale == 1.f);

		const auto* sprite11 = style_sheet->GetSprite("test11");
		CHECK(sprite11 != nullptr);
		CHECK(sprite11->sprite_sheet->name == "test_sheet");
		CHECK(sprite11->sprite_sheet->texture_source.GetSource() == "/assets/high_scores_alien_3.tga");
		CHECK(sprite11->rectangle.TopLeft() == Vector2f(64.f, 64.f));
		CHECK(sprite11->rectangle.BottomRight() == Vector2f(128.f, 128.f));
		CHECK(sprite11->sprite_sheet->display_scale == 1.f);
	}

	TestsShell::ShutdownShell();
}

TEST_CASE("style_sheet_parser.spritesheet_with_path_string_encoding")
{
	Context* context = TestsShell::GetContext();

	{
		StyleSheetContainer style_sheet_container;
		StreamMemory spritesheet_stream{reinterpret_cast<const byte*>(spritesheet_with_path_string_encoding),
			sizeof(spritesheet_with_path_string_encoding) - 1};
		style_sheet_container.LoadStyleSheetContainer(&spritesheet_stream, 0);

		style_sheet_container.UpdateCompiledStyleSheet(context);
		const auto* style_sheet = style_sheet_container.GetCompiledStyleSheet();
		CHECK(style_sheet != nullptr);

		const auto* sprite00 = style_sheet->GetSprite("test00");
		CHECK(sprite00 != nullptr);
		CHECK(sprite00->sprite_sheet->name == "test_sheet_with_path_string_encoding");
		CHECK(sprite00->sprite_sheet->texture_source.GetSource() == "/assets/test.tga");
		CHECK(sprite00->rectangle.TopLeft() == Vector2f(0.f, 0.f));
		CHECK(sprite00->rectangle.BottomRight() == Vector2f(128.f, 128.f));
		CHECK(sprite00->sprite_sheet->display_scale == 0.5f);

		const auto* sprite01 = style_sheet->GetSprite("test01");
		CHECK(sprite01 != nullptr);
		CHECK(sprite01->sprite_sheet->name == "test_sheet_with_path_string_encoding");
		CHECK(sprite01->sprite_sheet->texture_source.GetSource() == "/assets/test.tga");
		CHECK(sprite01->rectangle.TopLeft() == Vector2f(128.f, 0.f));
		CHECK(sprite01->rectangle.BottomRight() == Vector2f(256.f, 128.f));
		CHECK(sprite01->sprite_sheet->display_scale == 0.5f);

		const auto* sprite10 = style_sheet->GetSprite("test10");
		CHECK(sprite10 != nullptr);
		CHECK(sprite10->sprite_sheet->name == "test_sheet_with_path_string_encoding");
		CHECK(sprite10->sprite_sheet->texture_source.GetSource() == "/assets/test.tga");
		CHECK(sprite10->rectangle.TopLeft() == Vector2f(0.f, 128.f));
		CHECK(sprite10->rectangle.BottomRight() == Vector2f(128.f, 256.f));
		CHECK(sprite10->sprite_sheet->display_scale == 0.5f);

		const auto* sprite11 = style_sheet->GetSprite("test11");
		CHECK(sprite11 != nullptr);
		CHECK(sprite11->sprite_sheet->name == "test_sheet_with_path_string_encoding");
		CHECK(sprite11->sprite_sheet->texture_source.GetSource() == "/assets/test.tga");
		CHECK(sprite11->rectangle.TopLeft() == Vector2f(128.f, 128.f));
		CHECK(sprite11->rectangle.BottomRight() == Vector2f(256.f, 256.f));
		CHECK(sprite11->sprite_sheet->display_scale == 0.5f);
	}

	TestsShell::ShutdownShell();
}

TEST_CASE("style_sheet_parser.comments")
{
	Context* context = TestsShell::GetContext();

	{
		StyleSheetContainer style_sheet_container;
		StreamMemory comments_stream{reinterpret_cast<const byte*>(comments), sizeof(comments) - 1};
		style_sheet_container.LoadStyleSheetContainer(&comments_stream, 0);

		style_sheet_container.UpdateCompiledStyleSheet(context);
		const auto* style_sheet = style_sheet_container.GetCompiledStyleSheet();
		CHECK(style_sheet != nullptr);

		const auto* sprite00 = style_sheet->GetSprite("test00");
		CHECK(sprite00 != nullptr);
		CHECK(sprite00->sprite_sheet->name == "test_sheet");
		CHECK(sprite00->sprite_sheet->texture_source.GetSource() == "/assets/high_scores_alien_3.tga");
		CHECK(sprite00->rectangle.TopLeft() == Vector2f(0.f, 0.f));
		CHECK(sprite00->rectangle.BottomRight() == Vector2f(64.f, 64.f));
		CHECK(sprite00->sprite_sheet->display_scale == 1.f);

		const auto* sprite01 = style_sheet->GetSprite("test01");
		CHECK(sprite01 != nullptr);
		CHECK(sprite01->sprite_sheet->name == "test_sheet");
		CHECK(sprite01->sprite_sheet->texture_source.GetSource() == "/assets/high_scores_alien_3.tga");
		CHECK(sprite01->rectangle.TopLeft() == Vector2f(64.f, 0.f));
		CHECK(sprite01->rectangle.BottomRight() == Vector2f(128.f, 64.f));
		CHECK(sprite01->sprite_sheet->display_scale == 1.f);

		const auto* sprite10 = style_sheet->GetSprite("test10");
		CHECK(sprite10 != nullptr);
		CHECK(sprite10->sprite_sheet->name == "test_sheet");
		CHECK(sprite10->sprite_sheet->texture_source.GetSource() == "/assets/high_scores_alien_3.tga");
		CHECK(sprite10->rectangle.TopLeft() == Vector2f(0.f, 64.f));
		CHECK(sprite10->rectangle.BottomRight() == Vector2f(64.f, 128.f));
		CHECK(sprite10->sprite_sheet->display_scale == 1.f);

		const auto* sprite11 = style_sheet->GetSprite("test11");
		CHECK(sprite11 != nullptr);
		CHECK(sprite11->sprite_sheet->name == "test_sheet");
		CHECK(sprite11->sprite_sheet->texture_source.GetSource() == "/assets/high_scores_alien_3.tga");
		CHECK(sprite11->rectangle.TopLeft() == Vector2f(64.f, 64.f));
		CHECK(sprite11->rectangle.BottomRight() == Vector2f(128.f, 128.f));
		CHECK(sprite11->sprite_sheet->display_scale == 1.f);
	}

	TestsShell::ShutdownShell();
}

TEST_CASE("style_sheet_parser.selector_list_with_escaped_comma")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);

	StyleSheetContainer style_sheet_container;
	StreamMemory stylesheet_stream{reinterpret_cast<const byte*>(selector_list_with_escaped_comma), sizeof(selector_list_with_escaped_comma) - 1};
	REQUIRE(style_sheet_container.LoadStyleSheetContainer(&stylesheet_stream, 0));

	style_sheet_container.UpdateCompiledStyleSheet(context);
	const auto* style_sheet = style_sheet_container.GetCompiledStyleSheet();
	REQUIRE(style_sheet != nullptr);

	{
		ElementPtr class_match = Factory::InstanceElement(nullptr, "*", "div", {});
		ElementPtr normal_match = Factory::InstanceElement(nullptr, "*", "div", {});
		ElementPtr no_match = Factory::InstanceElement(nullptr, "*", "div", {});
		REQUIRE(class_match);
		REQUIRE(normal_match);
		REQUIRE(no_match);

		class_match->SetClassNames("escaped,class");
		normal_match->SetId("normal_match");
		no_match->SetId("no_match");

		SharedPtr<const ElementDefinition> class_definition = style_sheet->GetElementDefinition(class_match.get());
		SharedPtr<const ElementDefinition> normal_definition = style_sheet->GetElementDefinition(normal_match.get());
		SharedPtr<const ElementDefinition> no_definition = style_sheet->GetElementDefinition(no_match.get());

		REQUIRE(bool(class_definition));
		REQUIRE(bool(normal_definition));
		CHECK_FALSE(bool(no_definition));

		const Property* class_width = class_definition->GetProperty(PropertyId::Width);
		const Property* normal_width = normal_definition->GetProperty(PropertyId::Width);
		REQUIRE(class_width);
		REQUIRE(normal_width);
		REQUIRE(bool(class_width->source));
		REQUIRE(bool(normal_width->source));
		CHECK(class_width->source->rule_name == ".escaped,class");
		CHECK(normal_width->source->rule_name == "#normal_match");
	}

	TestsShell::ShutdownShell();
}

TEST_CASE("stream_memory.borrowed_and_seek_end")
{
	const byte data[] = {1, 2, 3};
	StreamMemory stream(data, sizeof(data));
	CHECK_FALSE(stream.IsWriteReady());
	CHECK(stream.Write(data, 1) == 0);
	CHECK(stream.PushFront(data, 1) == 0);
	stream.Erase(0, 1);
	CHECK(stream.Length() == 3);
	CHECK(stream.Seek(-1, SEEK_END));
	CHECK(stream.Tell() == 2);
	CHECK_FALSE(stream.Seek(1, SEEK_END));
	CHECK(stream.Tell() == 2);
}

TEST_CASE("stream copy retains unwritten source position")
{
	class LimitedSink : public Stream {
	public:
		void Close() override {}
		size_t Length() const override { return accepted; }
		size_t Tell() const override { return accepted; }
		bool Seek(long, int) const override { return false; }
		size_t Read(void*, size_t) const override { return 0; }
		size_t Write(const void*, size_t bytes) override
		{
			const size_t count = bytes < 2 - accepted ? bytes : 2 - accepted;
			accepted += count;
			return count;
		}
		size_t Truncate(size_t) override { return 0; }
		bool IsReadReady() override { return false; }
		bool IsWriteReady() override { return accepted < 2; }
		size_t accepted = 0;
	};
	const byte data[] = {'a', 'b', 'c', 'd'};
	StreamMemory source(data, sizeof(data));
	LimitedSink sink;
	CHECK(source.Read(&sink, sizeof(data)) == 2);
	CHECK(source.Tell() == 2);
	char remaining[2] = {};
	CHECK(source.Read(remaining, sizeof(remaining)) == 2);
	CHECK(remaining[0] == 'c');
	CHECK(remaining[1] == 'd');
}

TEST_CASE("style_sheet_parser.newlines_separate_tokens")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);
	ElementDocument* document = context->LoadDocumentFromMemory(R"(
	<rml><head><style>
	.parent
	.child { display: block; width: 12px; height: 13px; }
	</style></head><body><div class="parent"><div class="child"/></div></body></rml>)");
	REQUIRE(document);
	document->Show();
	context->Update();
	ElementList elements;
	document->GetElementsByTagName(elements, "div");
	REQUIRE(elements.size() == 2);
	CHECK(elements[1]->GetBox().GetSize() == Vector2f(12.f, 13.f));
	document->Close();
	TestsShell::ShutdownShell();
}

TEST_CASE("style_sheet_parser.selector_whitespace")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);
	ElementDocument* document = context->LoadDocumentFromMemory(
		"<rml><head><style>"
		".parent\t.tab { display: block; width: 11px; height: 12px; }"
		".parent\r.cr { display: block; width: 13px; height: 14px; }"
		".parent\t>\t.direct { display: block; width: 17px; height: 18px; }"
		"</style></head><body><div class='parent'>"
		"<div id='tab' class='tab'/><div id='cr' class='cr'/>"
		"<div id='direct' class='direct'/></div>"
		"<div id='outside' class='tab'/></body></rml>");
	REQUIRE(document);
	document->Show();
	context->Update();
	Element* tab = document->GetElementById("tab");
	Element* cr = document->GetElementById("cr");
	Element* direct = document->GetElementById("direct");
	Element* outside = document->GetElementById("outside");
	REQUIRE(tab);
	REQUIRE(cr);
	REQUIRE(direct);
	REQUIRE(outside);
	CHECK(tab->GetBox().GetSize() == Vector2f(11.f, 12.f));
	CHECK(cr->GetBox().GetSize() == Vector2f(13.f, 14.f));
	CHECK(direct->GetBox().GetSize() == Vector2f(17.f, 18.f));
	CHECK(outside->GetBox().GetSize() != Vector2f(11.f, 12.f));
	document->Close();
	TestsShell::ShutdownShell();
}

TEST_CASE("texture_layout.large_area_estimate")
{
	TextureLayout layout;
	layout.AddRectangle(0, Vector2i(50000, 50000));
	CHECK_FALSE(layout.GenerateLayout(128));
}

class TestVerticalInstancer : public DecoratorTiledVerticalInstancer {
public:
	using DecoratorTiledInstancer::GetTileProperties;
};

TEST_CASE("style_sheet_parser.named_decorator_sources")
{
	Context* context = TestsShell::GetContext();
	{
	const String css = R"(
		@decorator base : tiled-vertical { top-image-src: same.png; center-image-src: same.png; }
		@decorator child : base { center-image-src: other.png; }
		@decorator grandchild : child { bottom-image-src: same.png; }
	)";
	StreamMemory stream{reinterpret_cast<const byte*>(css.data()), css.size()};
	stream.SetSourceURL(URL("/declaration/theme.rcss"));
	StyleSheetContainer container;
	REQUIRE(container.LoadStyleSheetContainer(&stream));
	container.UpdateCompiledStyleSheet(context);
	StyleSheet* sheet = container.GetCompiledStyleSheet();
	REQUIRE(sheet);
	const NamedDecorator* base = sheet->GetNamedDecorator("base");
	const NamedDecorator* child = sheet->GetNamedDecorator("child");
	const NamedDecorator* grandchild = sheet->GetNamedDecorator("grandchild");
	REQUIRE(base);
	REQUIRE(child);
	REQUIRE(grandchild);
	const auto& spec = base->instancer->GetPropertySpecification();
	const auto top = spec.GetProperty("top-image-src")->GetId();
	const auto center = spec.GetProperty("center-image-src")->GetId();
	CHECK(child->properties.GetProperty(top)->source == base->properties.GetProperty(top)->source);
	CHECK(child->properties.GetProperty(center)->source != base->properties.GetProperty(center)->source);
	CHECK(grandchild->properties.GetProperty(center)->source == child->properties.GetProperty(center)->source);
	CHECK(grandchild->properties.GetProperty(top)->source->path == "/declaration/theme.rcss");

	// Simulate inherited properties originating in different directories, including equal filenames.
	PropertyDictionary properties = grandchild->properties;
	Property center_property = *properties.GetProperty(center);
	center_property.value = String("same.png");
	center_property.source = MakeShared<PropertySource>("/other/theme.rcss", 1, "");
	properties.SetProperty(center, center_property);
	DecoratorInstancerInterface interface(TestsShell::GetContext()->GetRenderManager(), *sheet, grandchild->source.get());
	auto decorator = grandchild->instancer->InstanceDecorator(grandchild->type, properties, interface);
	REQUIRE(decorator);
	TestVerticalInstancer tile_instancer;
	DecoratorTiled::Tile tiles[3];
	Texture textures[3];
	REQUIRE(tile_instancer.GetTileProperties(tiles, textures, 3, properties, interface));
	CHECK_FALSE(textures[0] == textures[2]);
	CHECK(interface.GetTexture("same.png", *properties.GetProperty(top)) == interface.GetTexture("same.png"));
	CHECK_FALSE(interface.GetTexture("same.png", center_property) == interface.GetTexture("same.png"));

	DecoratorDeclarationList declarations;
	declarations.value = "grandchild";
	declarations.list.push_back(DecoratorDeclaration{"grandchild", nullptr, {}, BoxArea::Border});
	PropertySource consumer_a("/consumer-a/view.rcss", 1, "");
	PropertySource consumer_b("/consumer-b/view.rcss", 1, "");
	const auto& first = sheet->InstanceDecorators(context->GetRenderManager(), declarations, &consumer_a);
	REQUIRE(first.size() == 1);
	auto first_decorator = first[0];
	const auto& again = sheet->InstanceDecorators(context->GetRenderManager(), declarations, &consumer_a);
	CHECK(again[0] == first_decorator);
	const auto& second = sheet->InstanceDecorators(context->GetRenderManager(), declarations, &consumer_b);
	REQUIRE(second.size() == 1);
	CHECK(second[0] != first_decorator);

	}
	TestsShell::ShutdownShell();
}
