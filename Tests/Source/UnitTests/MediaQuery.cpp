#include "../Common/TestsShell.h"
#include "../../../Source/Core/PropertyParserRatio.h"
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <doctest.h>

using namespace Rml;

static const String document_media_query1_rml = R"(
<rml>
<head>
	<title>Test</title>
	<link type="text/rcss" href="/assets/rml.rcss"/>
	<style>
		body {
			left: 0;
			top: 0;
			right: 0;
			bottom: 0;
		}

		div {
			height: 48px;
			width: 48px;
			background: white;
		}

		@media (min-width: 640px) {
			div {
				height: 32px;
				width: 32px;
			}
		}

		@media (max-width: 639px) {
			div {
				height: 64px;
				width: 64px;
			}
		}
	</style>
</head>

<body>
<div/>
</body>
</rml>
)";

static const String document_media_query2_rml = R"(
<rml>
<head>
	<title>Test</title>
	<link type="text/rcss" href="/assets/rml.rcss"/>
	<style>
		body {
			left: 0;
			top: 0;
			right: 0;
			bottom: 0;
		}

		div {
			height: 48px;
			width: 48px;
			background: white;
		}

		@media (orientation: landscape) {
			div {
				height: 32px;
				width: 32px;
			}
		}

		@media (max-aspect-ratio: 3/4) {
			div {
				height: 64px;
				width: 64px;
			}
		}

		@media (min-resolution: 2x) {
			div {
				height: 128px;
			}
		}
	</style>
</head>

<body>
<div/>
</body>
</rml>
)";

static const String document_media_query3_rml = R"(
<rml>
<head>
	<title>Test</title>
	<link type="text/rcss" href="/assets/rml.rcss"/>
	<style>
		body {
			left: 0;
			top: 0;
			right: 0;
			bottom: 0;
		}

		div {
			height: 48px;
			width: 48px;
			background: white;
		}

		@media (orientation: landscape) and (min-width: 640px) {
			div {
				height: 32px;
				width: 32px;
			}
		}

		@media (max-aspect-ratio: 4/3) {
			div {
				height: 64px;
				width: 64px;
			}
		}
	</style>
</head>

<body>
<div/>
</body>
</rml>
)";

static const String document_media_query4_rml = R"(
<rml>
<head>
	<title>Test</title>
	<link type="text/rcss" href="/assets/rml.rcss"/>
	<style>
		body {
			left: 0;
			top: 0;
			right: 0;
			bottom: 0;
		}

		div {
			height: 48px;
			width: 48px;
			background: white;
		}

		@media (theme: big) {
			div {
				height: 96px;
				width: 96px;
			}
		}

		@media (theme: tiny) {
			div {
				height: 32px;
				width: 32px;
			}
		}
	</style>
</head>

<body>
<div/>
</body>
</rml>
)";

static const String document_media_query5_rml = R"(
<rml>
<head>
	<title>Test</title>
	<link type="text/rcss" href="/assets/rml.rcss"/>
	<style>
		body {
			left: 0;
			top: 0;
			right: 0;
			bottom: 0;
		}

		div {
			height: 48px;
			width: 48px;
			background: white;
		}

		@media not (theme: tiny) {
			div {
				height: 32px;
				width: 32px;
			}
		}

		@media (theme: big) {
			div {
				height: 96px;
				width: 96px;
			}
		}
	</style>
</head>

<body>
<div/>
</body>
</rml>
)";

static const String document_media_query6_rml = R"(
<rml>
<head>
	<title>Test</title>
	<link type="text/rcss" href="/assets/rml.rcss"/>
	<style>
		body {
			left: 0;
			top: 0;
			right: 0;
			bottom: 0;
		}

		div {
			height: 48px;
			width: 48px;
			background: white;
		}

		@media not not (theme: big) {
			div {
				height: 32px;
				width: 32px;
			}
		}
	</style>
</head>

<body>
<div/>
</body>
</rml>
)";

static const String document_media_query7_rml = R"(
<rml>
<head>
	<title>Test</title>
	<link type="text/rcss" href="/assets/rml.rcss"/>
	<style>
		body {
			left: 0;
			top: 0;
			right: 0;
			bottom: 0;
		}

		div {
			height: 48px;
			width: 48px;
			background: white;
		}

		@media (theme: big) (theme: small) {
			div {
				height: 32px;
				width: 32px;
			}
		}

		@media not (theme: big) (theme: small) {
			div {
				height: 32px;
				width: 32px;
			}
		}
	</style>
</head>

<body>
<div/>
</body>
</rml>
)";

TEST_CASE("mediaquery.basic")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);

	// There should be no warnings loading this document. There should be one div of 32px width & height
	ElementDocument* document = context->LoadDocumentFromMemory(document_media_query1_rml);
	REQUIRE(document);
	document->Show();

	context->Update();
	context->Render();

	TestsShell::RenderLoop();

	// Shell default window dimensions are 1500, 800

	ElementList elems;
	document->GetElementsByTagName(elems, "div");
	CHECK(elems.size() == 1);

	CHECK(elems[0]->GetBox() == Box(Vector2f(32.0f, 32.0f)));

	document->Close();

	TestsShell::ShutdownShell();
}

TEST_CASE("mediaquery.dynamic")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);

	// There should be no warnings loading this document. There should be one div of 32px width & height
	ElementDocument* document = context->LoadDocumentFromMemory(document_media_query1_rml);
	REQUIRE(document);
	document->Show();

	context->Update();
	context->Render();

	TestsShell::RenderLoop();

	// Shell default window dimensions are 1500, 800

	ElementList elems;
	document->GetElementsByTagName(elems, "div");
	CHECK(elems.size() == 1);

	CHECK(elems[0]->GetBox() == Box(Vector2f(32.0f, 32.0f)));

	context->SetDimensions(Vector2i(480, 320));

	context->Update();

	CHECK(elems[0]->GetBox() == Box(Vector2f(64.0f, 64.0f)));

	document->Close();

	TestsShell::ShutdownShell();
}

TEST_CASE("mediaquery.custom_properties")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);

	context->SetDensityIndependentPixelRatio(2.0f);

	// There should be no warnings loading this document. There should be one div of 32px width & height
	ElementDocument* document = context->LoadDocumentFromMemory(document_media_query2_rml);
	REQUIRE(document);
	document->Show();

	context->Update();
	context->Render();

	TestsShell::RenderLoop();

	// Shell default window dimensions are 1500, 800

	ElementList elems;
	document->GetElementsByTagName(elems, "div");
	CHECK(elems.size() == 1);

	CHECK(elems[0]->GetBox() == Box(Vector2f(32.0f, 128.0f)));

	document->Close();

	TestsShell::ShutdownShell();
}

TEST_CASE("mediaquery.composite")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);

	// There should be no warnings loading this document. There should be one div of 32px width & height
	ElementDocument* document = context->LoadDocumentFromMemory(document_media_query3_rml);
	REQUIRE(document);
	document->Show();

	context->Update();
	context->Render();

	TestsShell::RenderLoop();

	// Shell default window dimensions are 1500, 800

	ElementList elems;
	document->GetElementsByTagName(elems, "div");
	CHECK(elems.size() == 1);

	CHECK(elems[0]->GetBox() == Box(Vector2f(32.0f, 32.0f)));

	document->Close();

	TestsShell::ShutdownShell();
}

TEST_CASE("mediaquery.theme")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);

	ElementDocument* document = context->LoadDocumentFromMemory(document_media_query4_rml);
	REQUIRE(document);
	document->Show();

	context->Update();
	context->Render();

	TestsShell::RenderLoop();

	ElementList elems;
	document->GetElementsByTagName(elems, "div");
	CHECK(elems.size() == 1);

	CHECK(elems[0]->GetBox().GetSize().x == 48.0f);

	context->ActivateTheme("big", true);
	context->Update();
	context->Render();

	CHECK(elems[0]->GetBox().GetSize().x == 96.0f);

	context->ActivateTheme("big", false);
	context->Update();
	context->Render();

	CHECK(elems[0]->GetBox().GetSize().x == 48.0f);

	context->ActivateTheme("tiny", true);
	context->Update();
	context->Render();

	CHECK(elems[0]->GetBox().GetSize().x == 32.0f);

	context->ActivateTheme("big", true);
	context->Update();
	context->Render();

	CHECK(elems[0]->GetBox().GetSize().x == 32.0f);

	document->Close();

	TestsShell::ShutdownShell();
}

// test of `not`; `not` should be an inverted case
TEST_CASE("mediaquery.theme.notonly")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);

	ElementDocument* document = context->LoadDocumentFromMemory(document_media_query5_rml);
	REQUIRE(document);
	document->Show();

	context->Update();
	context->Render();

	TestsShell::RenderLoop();

	ElementList elems;
	document->GetElementsByTagName(elems, "div");
	CHECK(elems.size() == 1);

	CHECK(elems[0]->GetBox().GetSize().x == 32.0f);

	context->ActivateTheme("big", true);
	context->Update();
	context->Render();

	CHECK(elems[0]->GetBox().GetSize().x == 96.0f);

	context->ActivateTheme("big", false);
	context->Update();
	context->Render();

	CHECK(elems[0]->GetBox().GetSize().x == 32.0f);

	context->ActivateTheme("tiny", true);
	context->Update();
	context->Render();

	CHECK(elems[0]->GetBox().GetSize().x == 48.0f);

	context->ActivateTheme("big", true);
	context->Update();
	context->Render();

	CHECK(elems[0]->GetBox().GetSize().x == 96.0f);

	document->Close();

	TestsShell::ShutdownShell();
}

// test that `not` cannot appear multiple times
TEST_CASE("mediaquery.theme.notonly_one")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);
	
	INFO("Expected warnings: unexpected 'not'.");
	TestsShell::SetNumExpectedWarnings(1);

	ElementDocument* document = context->LoadDocumentFromMemory(document_media_query6_rml);
	REQUIRE(document);
	document->Close();

	TestsShell::ShutdownShell();
}


// test that an `and` must be between multiple conditions.
TEST_CASE("mediaquery.theme.condition_checks")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);
	
	INFO("Expected warnings: expected 'and'.");
	TestsShell::SetNumExpectedWarnings(2);

	ElementDocument* document = context->LoadDocumentFromMemory(document_media_query7_rml);
	REQUIRE(document);
	document->Close();

	TestsShell::ShutdownShell();
}

TEST_CASE("mediaquery.fractional_ratio_and_negated_conjunction")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);
	context->SetDimensions(Vector2i(600, 400));
	ElementDocument* document = context->LoadDocumentFromMemory(R"(
	<rml><head><style>
	div { display: block; width: 10px; height: 10px; }
	@media (aspect-ratio: 1.5/1) { div { width: 20px; } }
	@media not (min-width: 500px) and (min-height: 500px) { div { height: 30px; } }
	</style></head><body><div/></body></rml>)");
	REQUIRE(document);
	document->Show();
	context->Update();
	ElementList elements;
	document->GetElementsByTagName(elements, "div");
	REQUIRE(elements.size() == 1);
	CHECK(elements[0]->GetBox().GetSize() == Vector2f(20.f, 30.f));
	context->SetDimensions(Vector2i(600, 600));
	context->Update();
	CHECK(elements[0]->GetBox().GetSize() == Vector2f(10.f, 10.f));
	context->SetDimensions(Vector2i(400, 600));
	context->Update();
	CHECK(elements[0]->GetBox().GetSize() == Vector2f(10.f, 30.f));
	document->Close();
	TestsShell::ShutdownShell();
}

TEST_CASE("mediaquery.invalid_features_do_not_apply")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);
	context->SetDimensions(Vector2i(600, 400));
	TestsShell::SetNumExpectedWarnings(3);
	ElementDocument* document = context->LoadDocumentFromMemory(R"(
	<rml><head><style>
	div { display: block; width: 10px; height: 10px; }
	@media (unknown-feature: 1) { div { width: 20px; } }
	@media (min-width: 100px) and (unknown-feature: 1) { div { height: 20px; } }
	@media not (unknown-feature: 1) { div { width: 30px; } }
	</style></head><body><div/></body></rml>)");
	REQUIRE(document);
	document->Show();
	context->Update();
	ElementList elements;
	document->GetElementsByTagName(elements, "div");
	REQUIRE(elements.size() == 1);
	CHECK(elements[0]->GetBox().GetSize() == Vector2f(10.f, 10.f));
	document->Close();
	TestsShell::ShutdownShell();
}

TEST_CASE("mediaquery.ratio_parser_rejects_malformed_values")
{
	Rml::PropertyParserRatio parser;
	Rml::Property property;
	Rml::ParameterMap parameters;
	CHECK(parser.ParseValue(property, " 1.5 / 1 ", parameters));
	for (const char* value : {"1x/2", "1/2junk", "1//2", "1/0", "-1/2", "nan/2", "1/inf", "1e999/2", "/2", "1/"})
		CHECK_FALSE(parser.ParseValue(property, value, parameters));
}

TEST_CASE("mediaquery.ratio_evaluation_boundaries")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);
	context->SetDimensions(Vector2i(600, 400));
	ElementDocument* document = context->LoadDocumentFromMemory(R"(
	<rml><head><style>
	div { display: block; width: 10px; height: 10px; }
	@media (aspect-ratio: 3/2) and (min-aspect-ratio: 1.5/1) and (max-aspect-ratio: 6/4) { div { width: 20px; } }
	@media not (min-aspect-ratio: 2/1) and (max-width: 700px) { div { height: 30px; } }
	@media (min-aspect-ratio: 2e38/1e38) { div { width: 40px; } }
	</style></head><body><div/></body></rml>)");
	REQUIRE(document);
	document->Show();
	context->Update();
	ElementList elements;
	document->GetElementsByTagName(elements, "div");
	REQUIRE(elements.size() == 1);
	CHECK(elements[0]->GetBox().GetSize() == Vector2f(20.f, 30.f));
	context->SetDimensions(Vector2i(800, 400));
	context->Update();
	CHECK(elements[0]->GetBox().GetSize() == Vector2f(40.f, 30.f));
	context->SetDimensions(Vector2i(600, 600));
	context->Update();
	CHECK(elements[0]->GetBox().GetSize() == Vector2f(10.f, 30.f));
	document->Close();
	TestsShell::ShutdownShell();
}

TEST_CASE("mediaquery.invalid_ratios_discard_whole_query")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);
	context->SetDimensions(Vector2i(600, 400));
	const char* values[] = {"1x/2", "1/2junk", "1//2", "1/0", "0/1", "-1/2", "1/-2", "nan/2", "1/inf", "1e999/2", "/2", "1/"};
	for (const char* value : values)
	{
		INFO("ratio: ", value);
		TestsShell::SetNumExpectedWarnings(2);
		String source = String("<rml><head><style>div { display: block; width: 10px; height: 10px; }") +
			"@media (aspect-ratio: " + value + ") { div { width: 20px; } }" +
			"@media not (min-width: 1px) and (max-aspect-ratio: " + value + ") { div { height: 30px; } }" +
			"</style></head><body><div/></body></rml>";
		ElementDocument* document = context->LoadDocumentFromMemory(source);
		REQUIRE(document);
		document->Show();
		context->Update();
		ElementList elements;
		document->GetElementsByTagName(elements, "div");
		REQUIRE(elements.size() == 1);
		CHECK(elements[0]->GetBox().GetSize() == Vector2f(10.f, 10.f));
		document->Close();
		context->Update();
	}
	TestsShell::ShutdownShell();
}
