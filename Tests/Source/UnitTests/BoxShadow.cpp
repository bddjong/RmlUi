#include "../Common/Mocks.h"
#include "../Common/TestsInterface.h"
#include "../Common/TestsShell.h"
#include "../Common/TypesToString.h"
#include "../../../Source/Core/GeometryBoxShadow.h"
#include "../../../Source/Core/BoxShadowCache.h"
#include <RmlUi/Core/Core.h>
#include "../../../Source/Core/GeometryBackgroundBorder.h"
#include "RmlUi/Core/DecorationTypes.h"
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <doctest.h>

using namespace Rml;

TEST_CASE("border outer radii use outer dimensions")
{
	const auto metrics = GeometryBackgroundBorder::ComputeBorderMetrics({0, 0}, {10, 10, 10, 10}, {80, 80}, {45, 45, 45, 45});
	CHECK(metrics.outer_radii[0] == 45.f);
	CHECK(metrics.inner_radii[0].x == 35.f);
	const auto scaled = GeometryBackgroundBorder::ComputeBorderMetrics({0, 0}, {10, 10, 10, 10}, {80, 80}, {60, 60, 60, 60});
	CHECK(scaled.outer_radii[0] == 50.f);
}

TEST_CASE("box shadow rejects unrepresentable texture dimensions")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);
	ElementDocument* document = context->LoadDocumentFromMemory(R"(<rml><body><div id="shadow" style="width: 20px; height: 20px; box-shadow: black 3000000000px 0px 0px;"/></body></rml>)");
	REQUIRE(document);
	document->Show();
	context->Update();
	Element* element = document->GetElementById("shadow");
	REQUIRE(element);
	const auto info = GeometryBoxShadow::Resolve(element, {}, {}, {}, 1.f);
	CHECK(info.texture_dimensions.x == 0);
	CHECK(info.texture_dimensions.y == 0);
	document->Close();
	TestsShell::ShutdownShell();
}

TEST_CASE("box shadow cache isolates render managers and retires retained handles")
{
	REQUIRE(TestsShell::GetContext());
	TestsRenderInterface renderer_a, renderer_b;
	Context* a = CreateContext("shadow_cache_a", {200, 200}, &renderer_a);
	Context* b = CreateContext("shadow_cache_b", {200, 200}, &renderer_b);
	REQUIRE(a);
	REQUIRE(b);
	const String markup = R"(<rml><body><div id="shadow" style="width: 20px; height: 20px; box-shadow: black 2px 2px 4px;"/></body></rml>)";
	auto* doc_a = a->LoadDocumentFromMemory(markup);
	auto* doc_b = b->LoadDocumentFromMemory(markup);
	REQUIRE(doc_a);
	REQUIRE(doc_b);
	doc_a->Show();
	doc_b->Show();
	a->Update();
	b->Update();
	Element* element_a = doc_a->GetElementById("shadow");
	Element* element_b = doc_b->GetElementById("shadow");
	REQUIRE(element_a);
	REQUIRE(element_b);
	auto shadow_a = BoxShadowCache::GetHandle(element_a, element_a->GetComputedValues());
	auto same_a = BoxShadowCache::GetHandle(element_a, element_a->GetComputedValues());
	auto shadow_b = BoxShadowCache::GetHandle(element_b, element_b->GetComputedValues());
	REQUIRE(shadow_a);
	REQUIRE(shadow_b);
	CHECK(shadow_a == same_a);
	CHECK(shadow_a != shadow_b);
	shadow_a->geometry.Render({});
	shadow_b->geometry.Render({});
	const auto released_a = renderer_a.GetCounters().release_geometry;
	const auto released_b = renderer_b.GetCounters().release_geometry;
	RemoveContext("shadow_cache_a");
	ReleaseRenderManagers();
	CHECK_FALSE(shadow_a->geometry);
	CHECK(shadow_b->geometry);
	CHECK(renderer_a.GetCounters().release_geometry == released_a + 1);
	CHECK(renderer_b.GetCounters().release_geometry == released_b);
	shadow_a.reset();
	same_a.reset();
	CHECK(renderer_a.GetCounters().release_geometry == released_a + 1);
	RemoveContext("shadow_cache_b");
	ReleaseRenderManagers();
	CHECK_FALSE(shadow_b->geometry);
	CHECK(renderer_b.GetCounters().release_geometry == released_b + 1);
	shadow_b.reset();
	CHECK(renderer_b.GetCounters().release_geometry == released_b + 1);
	TestsShell::ShutdownShell();
}

TEST_CASE("box_shadow")
{
	static const String document_rml = R"(
<rml>
<head>
	<title>Test</title>
	<link type="text/rcss" href="/assets/rml.rcss"/>
	<style>
		body { inset: 0; }
		hr { display: block; border-bottom: 1px #ccc; }

		div {
			width: 80px;
			height: 22px;
			margin: 20px auto;
			background: #050;
		}
		.border { border: 2px #f00; }
		.zero_height { height: 0; }

		.shadow { box-shadow: blue 5px 6px 4px 0px; }
		.spread { box-shadow: blue 5px 6px 4px 5px; }
		.inset { box-shadow: yellow 5px 6px 4px 5px inset; }
		.spread_and_inset { box-shadow: blue 5px 6px 4px 5px, yellow 5px 6px 4px 5px inset; }

		.transparent_shadow { box-shadow: transparent 5px 6px 4px 0px; }
		.transparent_spread { box-shadow: transparent 5px 6px 4px 5px; }
		.transparent_inset { box-shadow: transparent 5px 6px 4px 5px inset; }
		.transparent_spread_and_inset { box-shadow: transparent 5px 6px 4px 5px, transparent 5px 6px 4px 5px inset; }
	</style>
</head>

<body>
	<div class="shadow" />
	<div class="spread" />
	<div class="inset" />
	<div class="spread_and_inset" />
	<hr />
	<div class="border shadow" />
	<div class="border spread" />
	<div class="border inset" />
	<div class="border spread_and_inset" />
	<hr />
	<div class="zero_height shadow" />
	<div class="zero_height spread" />
	<div class="zero_height inset" />
	<div class="zero_height spread_and_inset" />
	<hr />
	<div class="border zero_height shadow" />
	<div class="border zero_height spread" />
	<div class="border zero_height inset" />
	<div class="border zero_height spread_and_inset" />
	<hr/>
	<div class="transparent_shadow" />
	<div class="transparent_spread" />
	<div class="transparent_inset" />
	<div class="transparent_spread_and_inset" />
</body>
</rml>
)";

	Context* context = TestsShell::GetContext();

	ElementDocument* document = context->LoadDocumentFromMemory(document_rml);
	document->Show();

	TestsShell::RenderLoop();

	document->Close();

	TestsShell::ShutdownShell();
}
