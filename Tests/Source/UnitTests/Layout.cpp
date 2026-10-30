#include "../Common/TestsShell.h"
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/ElementText.h>
#include "../../../Source/Core/TextParagraph.h"
#include <doctest.h>

using namespace Rml;

TEST_CASE("Layout.TextParagraph.NormalizationAndSourceMapping")
{
	TextParagraph paragraph;
	const String first = "  a  &amp;";
	const auto one = paragraph.Append(first, Style::WhiteSpace::Normal, Style::TextTransform::Uppercase, true, true);
	CHECK(paragraph.GetText() == "A &");
	REQUIRE(one.source_to_paragraph.size() == first.size() + 1);
	CHECK(one.source_to_paragraph[0] == 0);
	CHECK(one.source_to_paragraph[2] == 0);
	CHECK(one.source_to_paragraph[3] == 1);
	CHECK(one.source_to_paragraph[5] == 2);
	CHECK(one.source_to_paragraph[first.size()] == 3);
	const String second = "  ب";
	const auto two = paragraph.Append(second, Style::WhiteSpace::Normal, Style::TextTransform::None, true, false);
	CHECK(paragraph.GetText() == "A & ب");
	REQUIRE(two.source_to_paragraph.size() == second.size() + 1);
	CHECK(two.source_to_paragraph[0] == 3);
	CHECK(two.source_to_paragraph[2] == 4);
	CHECK(two.source_to_paragraph.back() == paragraph.GetText().size());
	paragraph.AppendBreak();
	const auto three = paragraph.Append("X\nY", Style::WhiteSpace::Preline, Style::TextTransform::Lowercase, true, false);
	CHECK(paragraph.GetText() == "A & ب\nx\ny");
	CHECK(three.source_to_paragraph.back() == paragraph.GetText().size());

	TextParagraph spaces;
	const String escaped = "a&nbsp;  b &bogus;";
	const auto four = spaces.Append(escaped, Style::WhiteSpace::Normal, Style::TextTransform::None, true, false);
	CHECK(spaces.GetText() == "a  b &bogus;");
	CHECK(four.source_to_paragraph[1] == 1);
	CHECK(four.source_to_paragraph[7] == 2);
	CHECK(four.source_to_paragraph.back() == spaces.GetText().size());

	TextParagraph preserved;
	preserved.Append(" A\n B", Style::WhiteSpace::Pre, Style::TextTransform::None, false, false);
	CHECK(preserved.GetText() == " A\n B");

	TextParagraph across_styles;
	const auto left = across_styles.Append("one ", Style::WhiteSpace::Normal, Style::TextTransform::None, true, false);
	const auto right = across_styles.Append("  two", Style::WhiteSpace::Normal, Style::TextTransform::None, true, false);
	CHECK(across_styles.GetText() == "one two");
	CHECK(left.source_to_paragraph.back() == 4);
	CHECK(right.source_to_paragraph[0] == 4);
	CHECK(right.source_to_paragraph[2] == 4);
	CHECK(right.source_to_paragraph.back() == 7);
}

TEST_CASE("Layout.TextParagraph.MatchesExistingLineNormalization")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);
	ElementDocument* document = context->LoadDocumentFromMemory("<rml><head><style>body { font-family: LatoLatin; font-size: 18px; }</style></head><body><p id='target'>seed</p></body></rml>");
	REQUIRE(document);
	document->Show();
	context->Update();
	Element* target = document->GetElementById("target");
	REQUIRE(target);
	auto* element = rmlui_dynamic_cast<ElementText*>(target->GetChild(0));
	REQUIRE(element);
	for (const String& source : {String("  one  two "), String("a &amp; b"), String("a&nbsp;  b"), String("hello ب")}) {
		element->SetText(source);
		context->Update();
		String actual;
		int consumed = 0;
		float width = 0;
		REQUIRE(element->GenerateLine(actual, consumed, width, 0, 100000.f, 0.f, true, true, false));
		TextParagraph paragraph;
		paragraph.Append(source, Style::WhiteSpace::Normal, Style::TextTransform::None, true, true);
		INFO(source);
		CHECK(paragraph.GetText() == actual);
	}
	document->Close();
	TestsShell::ShutdownShell();
}

TEST_CASE("Layout.TextParagraph.CollectsInlineContexts")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);
	ElementDocument* document = context->LoadDocumentFromMemory(
		"<rml><head><style>body { font-family: LatoLatin; font-size: 18px; }</style></head>"
		"<body><div id='root' style='display:block'>first <span>two</span><div style='display:block'>block</div><span>third</span>"
		"<span style='display:none'>hidden</span></div></body></rml>");
	REQUIRE(document);
	document->Show();
	context->Update();
	TextParagraphCollector collector;
	collector.Collect(document->GetElementById("root"));
	const auto& groups = collector.GetGroups();
	REQUIRE(groups.size() == 2);
	if (groups.size() == 2) {
		CHECK(groups[0].paragraph.GetText() == "first two");
		CHECK(groups[0].spans.size() == 2);
		CHECK(groups[1].paragraph.GetText() == "third");
		REQUIRE(groups[1].spans.size() == 1);
		if (groups[1].spans.size() == 1) CHECK(collector.Find(groups[1].spans[0].element) == &groups[1]);
	}
	document->Close();
	TestsShell::ShutdownShell();
}

TEST_CASE("Layout.TextParagraph.StyledBidiRuns")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);
	ElementDocument* document = context->LoadDocumentFromMemory(
		"<rml><head><style>body { font-family: LatoLatin; font-size: 18px; } "
		"#text { display: block; width: 400px; -rmlui-direction: rtl; }</style></head>"
		"<body><div id='text'><span id='ar'>سلام </span><span id='number'>123</span></div></body></rml>");
	REQUIRE(document);
	document->Show();
	context->Update();
	Element* ar = document->GetElementById("ar");
	Element* number = document->GetElementById("number");
	REQUIRE(ar);
	REQUIRE(number);
	CHECK(number->GetAbsoluteOffset().x < ar->GetAbsoluteOffset().x);
	document->Close();
	TestsShell::ShutdownShell();
}

TEST_CASE("Layout.TextParagraph.AtomicInlineBidi")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);
	ElementDocument* document = context->LoadDocumentFromMemory(
		"<rml><head><style>body { font-family: LatoLatin; font-size: 18px; } "
		"#text { display: block; width: 400px; -rmlui-direction: rtl; } "
		"#object { display: inline-block; width: 20px; height: 20px; }</style></head>"
		"<body><div id='text'><span id='ar'>سلام </span><span id='object'></span><span id='number'>123</span></div></body></rml>");
	REQUIRE(document);
	document->Show();
	context->Update();
	Element* ar = document->GetElementById("ar");
	Element* object = document->GetElementById("object");
	Element* number = document->GetElementById("number");
	REQUIRE(ar);
	REQUIRE(object);
	REQUIRE(number);
	CHECK(number->GetAbsoluteOffset().x < object->GetAbsoluteOffset().x);
	CHECK(object->GetAbsoluteOffset().x < ar->GetAbsoluteOffset().x);
	document->Close();
	TestsShell::ShutdownShell();
}

TEST_CASE("Layout.TextParagraph.AtomicOnlyBidi")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);
	ElementDocument* document = context->LoadDocumentFromMemory(
		"<rml><head><style>body { font-family:LatoLatin; font-size:18px; } "
		"#text { display:block; width:400px; -rmlui-direction:rtl; } "
		".object { display:inline-block; width:20px; height:20px; }</style></head>"
		"<body><div id='text'><span class='object' id='first'></span><span class='object' id='second'></span>"
		"</div></body></rml>");
	REQUIRE(document);
	document->Show();
	context->Update();
	Element* first = document->GetElementById("first");
	Element* second = document->GetElementById("second");
	REQUIRE(first);
	REQUIRE(second);
	CHECK(second->GetAbsoluteOffset().x < first->GetAbsoluteOffset().x);
	document->Close();
	TestsShell::ShutdownShell();
}

TEST_CASE("Layout.TextParagraph.InlineSpacingBidi")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);
	ElementDocument* document = context->LoadDocumentFromMemory(
		"<rml><head><style>body { font-family: LatoLatin; font-size: 18px; } "
		"#text { display: block; width: 400px; -rmlui-direction: rtl; } "
		"#ar { padding: 0px 8px; border: 2px #fff; }</style></head>"
		"<body><div id='text'><span id='ar'>سلام </span><span id='number'>123</span></div></body></rml>");
	REQUIRE(document);
	document->Show();
	context->Update();
	Element* ar = document->GetElementById("ar");
	Element* number = document->GetElementById("number");
	REQUIRE(ar);
	REQUIRE(number);
	CHECK(number->GetAbsoluteOffset(BoxArea::Border).x < ar->GetAbsoluteOffset(BoxArea::Border).x);
	CHECK(ar->GetBox().GetSize(BoxArea::Border).x == doctest::Approx(ar->GetBox().GetSize(BoxArea::Content).x + 20.f));
	document->Close();
	TestsShell::ShutdownShell();
}

TEST_CASE("Layout.TextParagraph.SplitVisualRunsWithinStyledText")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);
	ElementDocument* document = context->LoadDocumentFromMemory(
		"<rml><head><style>body { font-family: LatoLatin; font-size: 18px; } "
		"#text { display: block; width: 400px; -rmlui-direction: rtl; } "
		"#mixed { padding: 0px 8px; border: 2px #fff; }</style></head>"
		"<body><div id='text'><span id='mixed'>سلام 123</span><span>456</span></div></body></rml>");
	REQUIRE(document);
	document->Show();
	context->Update();
	Element* mixed = document->GetElementById("mixed");
	REQUIRE(mixed);
	auto* node = rmlui_dynamic_cast<ElementText*>(mixed->GetChild(0));
	REQUIRE(node);
	CHECK(node->GetLines().size() >= 2);
	CHECK(mixed->GetNumBoxes() >= 2);
	for (const auto& line : node->GetLines()) CHECK(line.paragraph != nullptr);
	document->Close();
	TestsShell::ShutdownShell();
}

TEST_CASE("Layout.TextParagraph.EmptyPaddedInlineBidi")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);
	ElementDocument* document = context->LoadDocumentFromMemory(
		"<rml><head><style>body { font-family: LatoLatin; font-size: 18px; } "
		"#text { display:block; width:400px; -rmlui-direction:rtl; } "
		"#empty { padding-left:20px; padding-right:20px; }</style></head>"
		"<body><div id='text'><span id='ar'>سلام </span><span id='empty'></span>"
		"<span id='number'>123</span></div></body></rml>");
	REQUIRE(document);
	document->Show();
	context->Update();
	Element* ar = document->GetElementById("ar");
	Element* empty = document->GetElementById("empty");
	Element* number = document->GetElementById("number");
	REQUIRE(ar);
	REQUIRE(empty);
	REQUIRE(number);
	const float left = empty->GetAbsoluteOffset(BoxArea::Border).x;
	const float right = left + empty->GetBox().GetSize(BoxArea::Border).x;
	CHECK(empty->GetBox().GetSize(BoxArea::Border).x == doctest::Approx(40.f));
	CHECK(number->GetAbsoluteOffset(BoxArea::Border).x < left);
	CHECK(right <= ar->GetAbsoluteOffset(BoxArea::Border).x + 0.01f);
	document->Close();
	TestsShell::ShutdownShell();
}

TEST_CASE("Layout.TextParagraph.DynamicEmptyTextBidi")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);
	ElementDocument* document = context->LoadDocumentFromMemory(
		"<rml><head><style>body { font-family:LatoLatin; font-size:18px; } "
		"#text { display:block; width:400px; -rmlui-direction:rtl; } "
		"#empty { padding-left:20px; padding-right:20px; }</style></head>"
		"<body><div id='text'><span id='ar'>سلام </span><span id='empty'>x</span>"
		"<span id='number'>123</span></div></body></rml>");
	REQUIRE(document);
	document->Show();
	Element* empty = document->GetElementById("empty");
	REQUIRE(empty);
	auto* node = rmlui_dynamic_cast<ElementText*>(empty->GetChild(0));
	REQUIRE(node);
	node->SetText("");
	context->Update();
	Element* ar = document->GetElementById("ar");
	Element* number = document->GetElementById("number");
	REQUIRE(ar);
	REQUIRE(number);
	const float left = empty->GetAbsoluteOffset(BoxArea::Border).x;
	const float right = left + empty->GetBox().GetSize(BoxArea::Border).x;
	CHECK(empty->GetBox().GetSize(BoxArea::Border).x == doctest::Approx(40.f));
	CHECK(number->GetAbsoluteOffset(BoxArea::Border).x < left);
	CHECK(right <= ar->GetAbsoluteOffset(BoxArea::Border).x + 0.01f);
	document->Close();
	TestsShell::ShutdownShell();
}

TEST_CASE("Layout.TextParagraph.HiddenOnlyInlineBidi")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);
	ElementDocument* document = context->LoadDocumentFromMemory(
		"<rml><head><style>body { font-family:LatoLatin; font-size:18px; } "
		"#text { display:block; width:400px; -rmlui-direction:rtl; } "
		"#empty { padding-left:20px; padding-right:20px; }</style></head>"
		"<body><div id='text'><span id='ar'>سلام </span><span id='empty'>"
		"<span style='display:none'>hidden</span></span><span id='number'>123</span></div></body></rml>");
	REQUIRE(document);
	document->Show();
	context->Update();
	Element* ar = document->GetElementById("ar");
	Element* empty = document->GetElementById("empty");
	Element* number = document->GetElementById("number");
	REQUIRE(ar);
	REQUIRE(empty);
	REQUIRE(number);
	const float left = empty->GetAbsoluteOffset(BoxArea::Border).x;
	const float right = left + empty->GetBox().GetSize(BoxArea::Border).x;
	CHECK(empty->GetBox().GetSize(BoxArea::Border).x == doctest::Approx(40.f));
	CHECK(number->GetAbsoluteOffset(BoxArea::Border).x < left);
	CHECK(right <= ar->GetAbsoluteOffset(BoxArea::Border).x + 0.01f);
	document->Close();
	TestsShell::ShutdownShell();
}

TEST_CASE("Layout.TextParagraph.EmptyTextDoesNotChangeBidiResolution")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);
	ElementDocument* document = context->LoadDocumentFromMemory(
		"<rml><head><style>body { font-family:LatoLatin; font-size:18px; } "
		"#text { display:block; width:400px; -rmlui-direction:rtl; }</style></head>"
		"<body><div id='text'><span id='first'>س1</span><span id='empty'>x</span>"
		"<span id='last'>A1A</span></div></body></rml>");
	REQUIRE(document);
	document->Show();
	Element* empty = document->GetElementById("empty");
	REQUIRE(empty);
	auto* empty_node = rmlui_dynamic_cast<ElementText*>(empty->GetChild(0));
	REQUIRE(empty_node);
	empty_node->SetText("");
	context->Update();
	auto* first = rmlui_dynamic_cast<ElementText*>(document->GetElementById("first")->GetChild(0));
	auto* last = rmlui_dynamic_cast<ElementText*>(document->GetElementById("last")->GetChild(0));
	REQUIRE(first);
	REQUIRE(last);
	REQUIRE(!last->GetLines().empty());
	float digit_x = -1.f;
	for (const auto& line : first->GetLines()) if (line.text == "1") digit_x = first->GetAbsoluteOffset().x + line.position.x;
	CHECK(digit_x >= 0.f);
	if (!last->GetLines().empty()) CHECK(digit_x < last->GetAbsoluteOffset().x + last->GetLines()[0].position.x);
	document->Close();
	TestsShell::ShutdownShell();
}

TEST_CASE("Layout.TextParagraph.CollapsedWhitespaceAcrossStyles")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);
	ElementDocument* document = context->LoadDocumentFromMemory(
		"<rml><head><style>body { font-family:LatoLatin; font-size:18px; } "
		"#text { display:block; width:400px; -rmlui-direction:rtl; }</style></head>"
		"<body><div id='text'><span id='ar'>سلام </span><span id='number'> 123</span></div></body></rml>");
	REQUIRE(document);
	document->Show();
	context->Update();
	Element* ar = document->GetElementById("ar");
	Element* number = document->GetElementById("number");
	REQUIRE(ar);
	REQUIRE(number);
	CHECK(number->GetAbsoluteOffset().x < ar->GetAbsoluteOffset().x);
	auto* node = rmlui_dynamic_cast<ElementText*>(number->GetChild(0));
	REQUIRE(node);
	CHECK(!node->GetLines().empty());
	if (!node->GetLines().empty()) CHECK(node->GetLines()[0].text == "123");
	document->Close();
	TestsShell::ShutdownShell();
}

TEST_CASE("Layout.TextParagraph.SharedStorageForManyStyledSpans")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);
	String markup = "<rml><head><style>body { font-family:LatoLatin; font-size:18px; } "
		"#text { display:block; width:8000px; -rmlui-direction:rtl; }</style></head><body><div id='text'>";
	for (int i = 0; i < 128; ++i) markup += "<span>x</span>";
	markup += "</div></body></rml>";
	ElementDocument* document = context->LoadDocumentFromMemory(markup);
	REQUIRE(document);
	document->Show();
	context->Update();
	Element* text = document->GetElementById("text");
	REQUIRE(text);
	const String* shared = nullptr;
	int checked = 0;
	for (int i = 0; i < text->GetNumChildren(); ++i) {
		Element* span = text->GetChild(i);
		auto* node = span->GetNumChildren() ? rmlui_dynamic_cast<ElementText*>(span->GetChild(0)) : nullptr;
		if (!node || node->GetLines().empty()) continue;
		const auto& paragraph = node->GetLines()[0].paragraph;
		REQUIRE(paragraph);
		if (!paragraph) continue;
		if (!shared) shared = paragraph.get();
		CHECK(paragraph.get() == shared);
		++checked;
	}
	CHECK(checked == 128);
	document->Close();
	TestsShell::ShutdownShell();
}

TEST_CASE("Layout.TextParagraph.NestedDirectionIsolation")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);
	ElementDocument* document = context->LoadDocumentFromMemory(
		"<rml><head><style>body { font-family:LatoLatin; font-size:18px; } "
		"#text { display:block; width:400px; -rmlui-direction:rtl; } "
		"#number { -rmlui-direction:ltr; }</style></head>"
		"<body><div id='text'><span id='ar'>سلام </span><span id='number'>(123)</span></div></body></rml>");
	REQUIRE(document);
	document->Show();
	context->Update();
	Element* number = document->GetElementById("number");
	REQUIRE(number);
	auto* text = rmlui_dynamic_cast<ElementText*>(number->GetChild(0));
	REQUIRE(text);
	REQUIRE(!text->GetLines().empty());
	if (!text->GetLines().empty()) {
		CHECK(text->GetLines().size() == 1);
		CHECK(text->GetLines()[0].direction == Style::Direction::Ltr);
		CHECK(text->GetLines()[0].text == "(123)");
	}
	document->Close();
	TestsShell::ShutdownShell();
}

TEST_CASE("Layout.TextParagraph.ForcedWidthSplitDoesNotAddEmptyLine")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);
	ElementDocument* document = context->LoadDocumentFromMemory(
		"<rml><head><style>body { font-family:LatoLatin; font-size:18px; } "
		"#text { display:block; width:1px; word-break:break-all; }</style></head>"
		"<body><div id='text'>AB</div></body></rml>");
	REQUIRE(document);
	document->Show();
	context->Update();
	Element* text = document->GetElementById("text");
	REQUIRE(text);
	auto* node = rmlui_dynamic_cast<ElementText*>(text->GetChild(0));
	REQUIRE(node);
	REQUIRE(node->GetLines().size() == 2);
	if (node->GetLines().size() == 2) {
		CHECK(node->GetLines()[0].text == "A");
		CHECK(node->GetLines()[1].text == "B");
	}
	document->Close();
	TestsShell::ShutdownShell();
}

TEST_CASE("Layout.TextParagraph.IsolatesSurviveHardBreaksAndBlockSplits")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);
	ElementDocument* document = context->LoadDocumentFromMemory(
		"<rml><head><style>body { font-family:LatoLatin; font-size:18px; } "
		".root { display:block; -rmlui-direction:ltr; } .rtl { -rmlui-direction:rtl; }"
		".block { display:block; } .pre { white-space:pre; }</style></head><body>"
		"<div id='block-root' class='root'><span class='rtl'>سلام<div class='block'>center</div><span>123</span></span></div>"
		"<div id='br-root' class='root'><span class='rtl'>سلام<br/>123</span></div>"
		"<div id='pre-root' class='root'><span class='rtl pre'>سلام\n123</span></div>"
		"<div id='auto-root' class='root'><span dir='auto'>سلام 123</span></div></body></rml>");
	REQUIRE(document);
	document->Show();
	context->Update();
	TextParagraphCollector collector;
	collector.Collect(document->GetElementById("block-root"));
	REQUIRE(collector.GetGroups().size() == 2);
	if (collector.GetGroups().size() == 2) {
		CHECK(collector.GetGroups()[0].paragraph.GetText() == "\xE2\x81\xA7" "سلام" "\xE2\x81\xA9");
		CHECK(collector.GetGroups()[1].paragraph.GetText() == "\xE2\x81\xA7" "123" "\xE2\x81\xA9");
	}
	for (const char* id : {"br-root", "pre-root"}) {
		collector.Collect(document->GetElementById(id));
		REQUIRE(collector.GetGroups().size() == 1);
		if (collector.GetGroups().size() == 1)
			CHECK(collector.GetGroups()[0].paragraph.GetText().find("\n\xE2\x81\xA7" "123") != String::npos);
	}
	collector.Collect(document->GetElementById("auto-root"));
	REQUIRE(collector.GetGroups().size() == 1);
	if (collector.GetGroups().size() == 1)
		CHECK(collector.GetGroups()[0].paragraph.GetText().find("\xE2\x81\xA8") == 0);
	document->Close();
	TestsShell::ShutdownShell();
}

static const String document_layout_rml = R"(
<rml>
<head>
	<link type="text/rcss" href="/assets/rml.rcss"/>
	<style>
		body {
			width: 500px;
			height: 300px;
			top: 100px;
			left: 100px;
			border: 10px #fff;
			background-color: #ccc;
		}
		#relative {
			width: 100%;
			height: 25%;
			background-color: red;
			position: relative;
			top: 50%;
		}
	</style>
</head>

<body>
	<div id="relative"/>
</body>
</rml>
)";

static const String document_layout_rml_nested = R"(
<rml>
<head>
	<link type="text/rcss" href="/assets/rml.rcss"/>
	<style>
		body {
			width: 500px;
			height: 300px;
			top: 100px;
			left: 100px;
			border: 10px #fff;
			background-color: #ccc;
		}
		#parent {
			background-color: green;
			position: relative;
			top: 50%;
		}
		#relative {
			width: 100%;
			height: 25%;
			background-color: red;
			position: relative;
			top: 50%;
		}
	</style>
</head>

<body>
	<div id="parent">
		<div id="relative"/>
	</div>
</body>
</rml>
)";

TEST_CASE("Layout.Position.Relative")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);

	// Test that percentage positioning in 'position: relative' elements is correctly resolved during the first layout run, and
	// does not change during the next layout run. See issue: https://github.com/mikke89/RmlUi/issues/262

	for (auto&& rml_source : {document_layout_rml, document_layout_rml_nested})
	{
		ElementDocument* document = context->LoadDocumentFromMemory(rml_source);
		REQUIRE(document);
		document->Show();

		Element* element = document->GetElementById("relative");
		REQUIRE(element);

		TestsShell::RenderLoop();

		const float absolute_top = element->GetAbsoluteTop();
		CHECK(absolute_top >= 150.f);

		// This forces a new layout run but shouldn't make any difference to the rendered output.
		document->SetProperty("width", "500px");
		TestsShell::RenderLoop();

		CHECK(absolute_top == element->GetAbsoluteTop());

		document->SetProperty("width", "400px");
		TestsShell::RenderLoop();

		CHECK(absolute_top == element->GetAbsoluteTop());

		document->Close();
	}

	TestsShell::ShutdownShell();
}

TEST_CASE("Layout.ScrollableOverflow.MarginTriggersScrollbar")
{
	const String document_rml = R"(
<rml>
<head>
	<link type="text/rcss" href="/../Tests/Data/style.rcss"/>
	<style>
		scrollbarvertical { width: 15px; }
		scrollbarhorizontal { height: 15px; }
		.scroller { width: 100px; height: 100px; background: #3a3; }
		.item { background: #33a; }
		.vertical { overflow-x: hidden; overflow-y: auto; }
		.horizontal { overflow-x: auto; overflow-y: hidden; }
		.flex { display: flex; }
		.flex.vertical { flex-direction: column; }
		.flex.horizontal { flex-direction: row; }
		.flex .item { flex-shrink: 0; }
		.vertical .item { width: 20px; height: 95px; margin-bottom: 20px; }
		.horizontal .item { width: 95px; height: 20px; margin-right: 20px; }
		.vertical .item.fits { height: 80px; }
	</style>
</head>
<body>
	<div class="scroller vertical"><div class="item"/></div>
	<div class="scroller flex vertical"><div class="item"/></div>
	<div class="scroller flex horizontal"><div class="item"/></div>

	<div class="scroller flex vertical"><div class="item fits"/></div>
	<div class="scroller vertical"><div class="item fits"/></div>
</body>
</rml>
)";

	// A direct child whose margin box extends past the scroll container's padding area should enable an auto-scrollbar, even
	// when the child's border box alone fits.
	Context* context = TestsShell::GetContext();
	REQUIRE(context);

	ElementDocument* document = context->LoadDocumentFromMemory(document_rml);
	REQUIRE(document);
	document->Show();
	TestsShell::RenderLoop();

	CHECK(document->GetChild(0)->GetScrollHeight() == doctest::Approx(115.f));
	CHECK(document->GetChild(0)->GetClientWidth() == doctest::Approx(100.f - 15.f)); // Height - Scrollbar

	CHECK(document->GetChild(1)->GetScrollHeight() == doctest::Approx(115.f));
	CHECK(document->GetChild(1)->GetClientWidth() == doctest::Approx(100.f - 15.f)); // Height - Scrollbar

	CHECK(document->GetChild(2)->GetScrollWidth() == doctest::Approx(115.f));
	CHECK(document->GetChild(2)->GetClientHeight() == doctest::Approx(100.f - 15.f)); // Width - Scrollbar

	// The last two items fits just about, no scrollbars should appear.
	CHECK(document->GetChild(3)->GetScrollHeight() == doctest::Approx(100.f));
	CHECK(document->GetChild(3)->GetClientWidth() == doctest::Approx(100.f)); // No scrollbar.

	CHECK(document->GetChild(4)->GetScrollHeight() == doctest::Approx(100.f));
	CHECK(document->GetChild(4)->GetClientWidth() == doctest::Approx(100.f)); // No scrollbar.

	document->Close();
	TestsShell::ShutdownShell();
}

TEST_CASE("Layout.ScrollableOverflow.AutoHeightHorizontalScrollbar")
{
	const String document_rml = R"(
<rml>
<head>
	<link type="text/rcss" href="/../Tests/Data/style.rcss"/>
	<style>
		scrollbarvertical { width: 15px; }
		scrollbarhorizontal { height: 15px; }
		.scroller { width: 120px; border: 2px #000; overflow: auto; background: #3a3; }
		.scroller div { background: #33a; }
		.flex { display: flex; flex-direction: row; }
		.item { flex-shrink: 0; width: 240px; height: 60px; }
		.tall { height: 200px; }
		.fixed { height: 80px; }
	</style>
</head>
<body>
	<div class="scroller flex"><div class="item"/></div>
	<div class="scroller"><div class="item"/></div>

	<div class="scroller flex fixed"><div class="item tall"/></div>
	<div class="scroller fixed"><div class="item tall"/></div>
</body>
</rml>
)";

	Context* context = TestsShell::GetContext();
	REQUIRE(context);

	ElementDocument* document = context->LoadDocumentFromMemory(document_rml);
	REQUIRE(document);
	document->Show();
	TestsShell::RenderLoop();

	// Auto height, horizontal scrollbar only.
	for (int i = 0; i < 2; i++)
	{
		INFO("Scroller: " << i);
		Element* e = document->GetChild(i);
		CHECK(e->GetClientWidth() == doctest::Approx(120.f)); // No vertical scrollbar.
		CHECK(e->GetClientHeight() == doctest::Approx(60.f)); // Container height - scrollbar height.
		CHECK(e->GetScrollWidth() == doctest::Approx(240.f));
	}

	// Fixed height with overflow on both axes, both scrollbars needed.
	for (int i = 2; i < 4; i++)
	{
		INFO("Scroller: " << i);
		Element* e = document->GetChild(i);
		CHECK(e->GetClientWidth() == doctest::Approx(105.f)); // Container width - vertical scrollbar.
		CHECK(e->GetClientHeight() == doctest::Approx(65.f)); // Container height - horizontal scrollbar.
	}

	document->Close();
	TestsShell::ShutdownShell();
}

TEST_CASE("Layout.ScrollableOverflow.ChildMarginTriggersScrollbar")
{
	const String document_rml = R"(
<rml>
<head>
	<link type="text/rcss" href="/../Tests/Data/style.rcss"/>
	<style>
		scrollbarvertical { width: 15px; }
		scrollbarhorizontal { height: 15px; }
		.scroller { width: 100px; height: 80px; padding-bottom: 20px; overflow: auto; background: #3a3; }
		.flex { display: flex; }
		/* Height + margin-bottom overflows onto the container padding. Both child margin and container padding should be reachable by scrolling. */
		.child { flex-shrink: 0; width: 20px; height: 70px; margin-bottom: 25px; background: #33a; }
	</style>
</head>
<body>
	<div id="block" class="scroller"><div class="child"/></div>
	<div id="flex" class="scroller flex"><div class="child"/></div>
</body>
</rml>
)";

	Context* context = TestsShell::GetContext();
	REQUIRE(context);

	ElementDocument* document = context->LoadDocumentFromMemory(document_rml);
	REQUIRE(document);
	document->Show();
	TestsShell::RenderLoop();

	for (const char* id : {"block", "flex"})
	{
		INFO("Scroller: " << id);
		Element* e = document->GetElementById(id);
		REQUIRE(e);
		CHECK(e->GetScrollHeight() == doctest::Approx(95.f + 20.f)); // Child height (w/margin) + container padding
		CHECK(e->GetClientWidth() == doctest::Approx(100.f - 15.f)); // Container width - Scrollbar width
	}

	document->Close();
	TestsShell::ShutdownShell();
}
