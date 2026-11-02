#include "../Common/TestsShell.h"
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/TextInputContext.h>
#include <RmlUi/Core/TextInputHandler.h>
#include <doctest.h>

using namespace Rml;

TEST_CASE("disabled select and range clear widget focus")
{
	Context* context = TestsShell::GetContext();
	ElementDocument* document = context->LoadDocumentFromMemory(R"(<rml><body><select id="select"><option>One</option><option>Two</option></select><input id="range" type="range"/></body></rml>)");
	REQUIRE(document);
	document->Show();
	context->Update();
	for (const char* id : {"select", "range"})
	{
		Element* control = document->GetElementById(id);
		REQUIRE(control);
		Element* indicator = nullptr;
		Element* select_box = nullptr;
		for (int i = 0; i < control->GetNumChildren(true); ++i)
		{
			Element* child = control->GetChild(i);
			if (child->GetTagName() == (String(id) == "select" ? "selectvalue" : "sliderbar"))
				indicator = child;
			if (child->GetTagName() == "selectbox")
				select_box = child;
		}
		REQUIRE(indicator);
		REQUIRE(control->Focus());
		CHECK(indicator->IsPseudoClassSet("focus"));
		if (select_box)
		{
			control->DispatchEvent("click", {});
			CHECK(select_box->GetLocalProperty("visibility")->ToString() == "visible");
		}
		control->SetAttribute("disabled", "");
		CHECK_FALSE(indicator->IsPseudoClassSet("focus"));
		CHECK_FALSE(control->Focus());
		if (select_box)
		{
			CHECK(select_box->GetLocalProperty("visibility")->ToString() == "hidden");
			control->DispatchEvent("click", {});
			CHECK(select_box->GetLocalProperty("visibility")->ToString() == "hidden");
		}
		control->RemoveAttribute("disabled");
		REQUIRE(control->Focus());
		CHECK(indicator->IsPseudoClassSet("focus"));
		control->Blur();
	}
	document->Close();
	TestsShell::ShutdownShell();
}

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

TEST_CASE("focused input type replacement restores text input context")
{
	REQUIRE(TestsShell::GetContext());
	struct Handler : TextInputHandler {
		TextInputContext* active = nullptr;
		int activated = 0, deactivated = 0, destroyed = 0;
		Element* replace_on_destroy = nullptr;
		void OnActivate(TextInputContext* context) override { active = context; ++activated; }
		void OnDeactivate(TextInputContext* context) override { if (active == context) active = nullptr; ++deactivated; }
		void OnDestroy(TextInputContext* context) override {
			if (active == context) active = nullptr;
			++destroyed;
			if (Element* target = replace_on_destroy) {
				replace_on_destroy = nullptr;
				target->SetAttribute("type", "checkbox");
			}
		}
	} handler;
	Context* context = CreateContext("input-type-focus", {640, 480}, nullptr, &handler);
	REQUIRE(context);
	struct ContextGuard {
		Handler* handler;
		bool active = true;
		~ContextGuard() { handler->replace_on_destroy = nullptr; if (active) RemoveContext("input-type-focus"); }
	} context_guard{&handler};
	ElementDocument* document = context->LoadDocumentFromMemory(R"(<rml><head><style>body { font-family: LatoLatin; }</style></head><body><input id="input" type="text" value="abc"/></body></rml>)");
	REQUIRE(document);
	document->Show();
	context->Update();
	Element* input = document->GetElementById("input");
	REQUIRE(input);
	REQUIRE(input->Focus());
	REQUIRE(handler.active);
	CHECK(handler.activated == 1);

	input->SetAttribute("type", "password");
	CHECK(context->GetFocusElement() == input);
	CHECK(handler.destroyed == 1);
	CHECK(handler.activated == 2);
	REQUIRE(handler.active);
	handler.active->SetText("X", 0, 0);
	CHECK(input->GetAttribute<String>("value", "") == "Xabc");

	input->SetAttribute("type", "checkbox");
	CHECK(context->GetFocusElement() == input);
	CHECK(handler.active == nullptr);
	CHECK(handler.destroyed == 2);
	input->SetAttribute("type", "text");
	CHECK(handler.activated == 3);
	REQUIRE(handler.active);
	input->SetAttribute("disabled", "");
	CHECK(context->GetFocusElement() != input);
	CHECK(handler.active == nullptr);
	CHECK(handler.deactivated == 1);
	input->SetAttribute("type", "password");
	CHECK(handler.active == nullptr);
	CHECK(handler.activated == 3);

	// The input handler may replace the type while its old context is being
	// destroyed. The outer replacement must not overwrite that newer choice.
	input->RemoveAttribute("disabled");
	context->Update();
	REQUIRE(input->Focus());
	REQUIRE(handler.active);
	handler.replace_on_destroy = input;
	input->SetAttribute("type", "text");
	CHECK(input->GetAttribute<String>("type", "") == "checkbox");
	CHECK(handler.active == nullptr);

	document->Close();
	const bool removed = RemoveContext("input-type-focus");
	context_guard.active = !removed;
	CHECK(removed);
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
