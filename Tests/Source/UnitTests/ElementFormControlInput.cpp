#include "../Common/TestsShell.h"
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <doctest.h>

using namespace Rml;

TEST_CASE("button label is removed on input type replacement")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);
	ElementDocument* document = context->LoadDocumentFromMemory(R"(<rml><head><style>body { font-family: LatoLatin; }</style></head><body><input id="button" type="button" value="Label"/></body></rml>)");
	REQUIRE(document);
	Element* button = document->GetElementById("button");
	REQUIRE(button);
	CHECK(button->GetNumChildren(true) == 1);
	button->SetAttribute("type", "checkbox");
	CHECK(button->GetNumChildren(true) == 0);
	button->SetAttribute("type", "button");
	CHECK(button->GetNumChildren(true) == 1);
	document->Close();
	TestsShell::ShutdownShell();
}

TEST_CASE("submit label is removed on input type replacement")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);
	ElementDocument* document = context->LoadDocumentFromMemory(R"(<rml><head><style>body { font-family: LatoLatin; }</style></head><body><input id="submit" type="submit" value="Send"/></body></rml>)");
	REQUIRE(document);
	Element* submit = document->GetElementById("submit");
	REQUIRE(submit);
	CHECK(submit->GetNumChildren(true) == 1);
	submit->SetAttribute("type", "checkbox");
	CHECK(submit->GetNumChildren(true) == 0);
	submit->SetAttribute("type", "submit");
	CHECK(submit->GetNumChildren(true) == 1);
	document->Close();
	TestsShell::ShutdownShell();
}

TEST_CASE("form.input.text.no_font")
{
	// Missing font style causes intrinsic width of 'input.text' to be set to zero pixels. Ensure we handle this gracefully without assertions.
	const String document_rml = R"(<rml>
	<head>
		<link type="text/rcss" href="/assets/rml.rcss" />
		<style>
			body {
				color: white;
				width: 300dp;
				height: 225dp;
			}
			input.text {
				border: 1px #000000;
				padding: 5dp 36dp 5dp 12dp;
				background: #fff;
				border-radius: 8dp;
			}
		</style>
	</head>

	<body>
		<input type="text"/>
	</body>
</rml>)";

	Context* context = TestsShell::GetContext();

	ElementDocument* document = context->LoadDocumentFromMemory(document_rml);
	document->Show();
	context->Update();
	context->Render();

	document->Close();
	TestsShell::ShutdownShell();
}

TEST_CASE("form.input.text.long_placeholder_mouse_click")
{
	const String document_rml = R"(<rml>
        <head>
                <link type="text/rcss" href="/assets/rml.rcss" />
                <style>
                        body {
                                width: 700dp;
                                height: 225dp;
                                margin: 0;
                        }
                        input[type="text"] {
                                font-family: LatoLatin;
                                display: block;
                                width: 600dp;
                                height: 40dp;
                                border: 1px #000000;
                                padding: 5dp;
                        }
                </style>
        </head>
        <body>
                <input type="text" placeholder="123456789-123456789-123456789-12345" />
        </body>
</rml>)";

	Context* context = TestsShell::GetContext();
	ElementDocument* document = context->LoadDocumentFromMemory(document_rml);
	document->Show();
	context->Update();
	context->Render();

	context->ProcessMouseMove(500, 20, 0);
	context->Update();
	context->ProcessMouseButtonDown(0, 0);
	context->Update();
	context->ProcessMouseButtonUp(0, 0);

	document->Close();
	TestsShell::ShutdownShell();
}

TEST_CASE("disabled control preserves authored focus and suppresses input")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);
	ElementDocument* document = context->LoadDocumentFromMemory(R"(
	<rml><head><style>body { margin: 0; } input { display: block; width: 100px; height: 30px; }</style></head>
	<body><input id="control" type="checkbox" style="focus: auto;"/><input id="next" type="checkbox"/></body></rml>)");
	REQUIRE(document);
	Element* control = document->GetElementById("control");
	Element* next = document->GetElementById("next");
	REQUIRE(control);
	REQUIRE(next);
	document->Show();
	context->Update();
	REQUIRE(control->Focus());
	control->SetAttribute("disabled", "");
	CHECK(context->GetFocusElement() != control);
	REQUIRE(control->GetLocalProperty("focus"));
	CHECK(control->GetLocalProperty("focus")->ToString() == "auto");
	CHECK_FALSE(control->Focus());
	context->ProcessMouseMove(10, 10, 0);
	context->Update();
	context->ProcessMouseButtonDown(0, 0);
	context->ProcessMouseButtonUp(0, 0);
	CHECK_FALSE(control->HasAttribute("checked"));
	context->ProcessKeyDown(Input::KI_TAB, 0);
	CHECK(context->GetFocusElement() == next);
	control->RemoveAttribute("disabled");
	REQUIRE(control->GetLocalProperty("focus"));
	CHECK(control->GetLocalProperty("focus")->ToString() == "auto");
	context->Update();
	CHECK(control->Focus());
	control->SetAttribute("disabled", "");
	CHECK(control->SetProperty("focus", "none"));
	control->RemoveAttribute("disabled");
	REQUIRE(control->GetLocalProperty("focus"));
	CHECK(control->GetLocalProperty("focus")->ToString() == "none");
	context->Update();
	CHECK_FALSE(control->Focus());
	control->SetAttribute("disabled", "");
	control->RemoveProperty("focus");
	control->RemoveAttribute("disabled");
	CHECK_FALSE(control->GetLocalProperty("focus"));
	context->Update();
	CHECK(control->Focus());
	document->Close();
	TestsShell::ShutdownShell();
}
