#include "../Common/TestsShell.h"
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/EventListener.h>
#include <RmlUi/Core/EventListenerInstancer.h>
#include <RmlUi/Core/Factory.h>
#include <doctest.h>

using namespace Rml;

static const String document_decorator_rml = R"(
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
		div, button {
			display: block;
			background: #333;
			height: 64px;
			width: 64px;
		}
	</style>
</head>

<body>
<div>
	<button id="exit" onclick="exit" />
</div>
</body>
</rml>
)";

class DemoEventListenerInstancer : public Rml::EventListenerInstancer {
public:
	Rml::EventListener* InstanceEventListener(const Rml::String& value, Rml::Element* element) override;
	bool has_exited = false;
};

class DemoEventListener : public Rml::EventListener {
public:
	DemoEventListener(const Rml::String& value, Rml::Element* element, DemoEventListenerInstancer* instancer) :
		value(value), element(element), instancer(instancer)
	{}

	void ProcessEvent(Rml::Event& /*event*/) override
	{
		if (value == "exit")
		{
			// Test replacing the current element. Need to be careful with regard to lifetime issues. The event's
			// current element will be destroyed, so we cannot use it after SetInnerRml(). The library should handle
			// this case safely internally when propagating the event further.
			Element* parent = element->GetParentNode();
			parent->SetInnerRML("<button onclick='confirm_exit' onmouseout='cancel_exit' />");
			if (Element* child = parent->GetChild(0))
				child->Focus();
		}
		else if (value == "confirm_exit")
		{
			instancer->has_exited = true;
		}
		else if (value == "cancel_exit")
		{
			if (Element* parent = element->GetParentNode())
				parent->SetInnerRML("<button id='exit' onclick='exit' />");
		}
	}

	void OnDetach(Rml::Element* /*element*/) override { delete this; }

private:
	Rml::String value;
	Rml::Element* element;
	DemoEventListenerInstancer* instancer;
};

Rml::EventListener* DemoEventListenerInstancer::InstanceEventListener(const Rml::String& value, Rml::Element* element)
{
	return new DemoEventListener(value, element, this);
}

TEST_CASE("event_listener.replace_current_element")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);

	DemoEventListenerInstancer event_listener_instancer;
	Rml::Factory::RegisterEventListenerInstancer(&event_listener_instancer);

	ElementDocument* document = context->LoadDocumentFromMemory(document_decorator_rml, "assets/");
	REQUIRE(document);
	document->Show();
	TestsShell::RenderLoop();

	context->ProcessMouseMove(32, 32, 0);
	TestsShell::RenderLoop();

	context->ProcessMouseButtonDown(0, 0);
	TestsShell::RenderLoop();

	context->ProcessMouseButtonUp(0, 0);
	TestsShell::RenderLoop();

	bool exit_cancelled = false;

	SUBCASE("move_mouse_outside_button")
	{
		context->ProcessMouseMove(100, 100, 0);
		TestsShell::RenderLoop();
		exit_cancelled = true;
	}
	SUBCASE("move_mouse_within_button")
	{
		context->ProcessMouseMove(33, 33, 0);
		TestsShell::RenderLoop();
	}

	context->ProcessMouseButtonDown(0, 0);
	TestsShell::RenderLoop();

	context->ProcessMouseButtonUp(0, 0);
	TestsShell::RenderLoop();

	CHECK(event_listener_instancer.has_exited == !exit_cancelled);

	document->Close();
	Rml::Factory::RegisterEventListenerInstancer(nullptr);
	TestsShell::ShutdownShell();
}

namespace {
class InputRegressionListener : public EventListener {
public:
	int count = 0;
	Element* remove_parent = nullptr;
	void ProcessEvent(Event&) override
	{
		++count;
		if (remove_parent)
			remove_parent->SetInnerRML("");
	}
};
}

TEST_CASE("input.cancel_and_detached_drag")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);
	ElementDocument* document = context->LoadDocumentFromMemory(R"(
	<rml><head><style>
	body { left: 0; top: 0; width: 200px; height: 200px; margin: 0; font-family: LatoLatin; }
	#target { display: block; width: 100px; height: 100px; drag: clone; }
	</style></head><body><div id='parent'><button id='target'>target</button></div></body></rml>)");
	REQUIRE(document);
	document->Show();
	TestsShell::RenderLoop();
	Element* target = document->GetElementById("target");
	REQUIRE(target);
	InputRegressionListener click, mouseup, dragstart, drag, drop;
	target->AddEventListener("click", &click);
	target->AddEventListener("mouseup", &mouseup);
	target->AddEventListener("dragstart", &dragstart);
	target->AddEventListener("drag", &drag);
	document->AddEventListener("dragdrop", &drop);

	SUBCASE("detaching dragged element during dragstart")
	{
		dragstart.remove_parent = document->GetElementById("parent");
		context->ProcessMouseMove(10, 10, 0);
		context->ProcessMouseButtonDown(0, 0);
		context->ProcessMouseMove(11, 10, 0);
		CHECK(dragstart.count == 1);
		CHECK(drag.count == 0);
		context->ProcessMouseButtonUp(0, 0);
	}
	SUBCASE("detaching dragged ancestor during dragstart")
	{
		dragstart.remove_parent = document->GetElementById("parent")->GetParentNode();
		context->ProcessMouseMove(10, 10, 0);
		context->ProcessMouseButtonDown(0, 0);
		context->ProcessMouseMove(11, 10, 0);
		CHECK(dragstart.count == 1);
		CHECK(drag.count == 0);
		context->ProcessMouseButtonUp(0, 0);
	}
	SUBCASE("cancelled touch does not click")
	{
		TouchList touches{{1, Vector2f(10, 10)}};
		context->ProcessTouchStart(touches, 0);
		context->ProcessTouchCancel(touches);
		CHECK(mouseup.count == 1);
		CHECK(click.count == 0);
		CHECK_FALSE(target->IsPseudoClassSet("active"));
		context->ProcessTouchStart(touches, 0);
		context->ProcessTouchEnd(touches, 0);
		CHECK(click.count == 1);
	}
	SUBCASE("cancelled drag does not drop")
	{
		TouchList touches{{1, Vector2f(10, 10)}};
		context->ProcessTouchStart(touches, 0);
		touches[0].position = Vector2f(11, 10);
		context->ProcessTouchMove(touches, 0);
		CHECK(dragstart.count == 1);
		context->ProcessTouchCancel(touches);
		CHECK(click.count == 0);
		CHECK(drop.count == 0);
	}
	document->Close();
	TestsShell::ShutdownShell();
}
