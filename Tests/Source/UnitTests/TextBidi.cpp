#include <RmlUi/Core/FontEngineInterface.h>
#include <RmlUi/Core/TextShapingContext.h>
#include <doctest.h>

using namespace Rml;

TEST_CASE("Paragraph bidi resolver uses logical UTF-8 offsets")
{
	FontEngineInterface engine;
	Vector<TextBidiRun> runs;
	const String text = "سلام 123";
	REQUIRE(engine.ResolveTextBidi(text, Style::Direction::Rtl, 0, text.size(), runs));
	REQUIRE(runs.size() == 2);
	CHECK(runs[0].offset == 9);
	CHECK(runs[0].length == 3);
	CHECK(runs[0].direction == Style::Direction::Ltr);
	CHECK(runs[1].offset == 0);
	CHECK(runs[1].length == 9);
	CHECK(runs[1].direction == Style::Direction::Rtl);

	CHECK_FALSE(engine.ResolveTextBidi(text, Style::Direction::Rtl, 1, 2, runs));
	CHECK(runs.empty());
	CHECK_FALSE(engine.ResolveTextBidi(text, Style::Direction::Rtl, text.size(), 1, runs));

	const String two_paragraphs = "first\nسلام 123";
	const size_t second = two_paragraphs.find("سلام");
	REQUIRE(engine.ResolveTextBidi(two_paragraphs, Style::Direction::Rtl, second, two_paragraphs.size() - second, runs));
	REQUIRE(runs.size() == 2);
	CHECK(runs[0].offset == second + 9);
	CHECK(runs[1].offset == second);
}

TEST_CASE("Bidi line retains paragraph isolate context")
{
	FontEngineInterface engine;
	Vector<TextBidiRun> runs;
	const String paragraph = "مرحبا \xE2\x81\xA8" "Ada 123 456" "\xE2\x81\xA9" " وداعا";
	const size_t line = paragraph.find("123");
	REQUIRE(line != String::npos);
	REQUIRE(engine.ResolveTextBidi(paragraph, Style::Direction::Rtl, line, 7, runs));
	REQUIRE(runs.size() == 1);
	CHECK(runs[0].offset == line);
	CHECK(runs[0].length == 7);
	CHECK(runs[0].direction == Style::Direction::Ltr);
}
