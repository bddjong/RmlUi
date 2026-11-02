#include "../Common/TestsShell.h"
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Event.h>
#include <RmlUi/Core/EventListener.h>
#include <RmlUi/Core/EventInstancer.h>
#include <RmlUi/Core/Factory.h>
#include <doctest.h>

using namespace Rml;

namespace {
struct MutableEvent : Event {
	using Event::Event;
	void MutateMouse() { parameters["mouse_x"] = 999.f; parameters["mouse_y"] = 998.f; }
};
struct MutableEventInstancer : EventInstancer {
	EventPtr InstanceEvent(Element* target, EventId id, const String& type, const Dictionary& parameters, bool interruptible) override
	{ return EventPtr(new MutableEvent(target, id, type, parameters, interruptible)); }
	void ReleaseEvent(Event* event) override { delete event; }
	void Release() override {}
};
struct ProjectionCall {
	Element* element;
	EventPhase phase;
	Vector2f position;
	Vector2f screen;
	bool propagating;
};
struct ProjectionListener : EventListener {
	Vector<ProjectionCall> calls;
	Element* mutate_on_capture = nullptr;
	void ProcessEvent(Event& event) override
	{
		calls.push_back({event.GetCurrentElement(), event.GetPhase(),
			{event.GetParameter<float>("mouse_x", -1), event.GetParameter<float>("mouse_y", -1)},
			event.GetUnprojectedMouseScreenPos(), event.IsPropagating()});
		if (event.GetCurrentElement() == mutate_on_capture && event.GetPhase() == EventPhase::Capture)
			static_cast<MutableEvent&>(event).MutateMouse();
	}
};
}

TEST_CASE("event.mouse_projection_dispatch")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);
	ElementDocument* document = context->LoadDocumentFromMemory(R"(
<rml><head><style>
#parent { transform: translateX(10px); }
#child { transform: translateY(20px); }
</style></head><body><div id="ancestor"><div id="parent"><div id="child"/></div></div></body></rml>)");
	REQUIRE(document);
	document->Show();
	context->Update();
	context->Render();
	Element* ancestor = document->GetElementById("ancestor");
	Element* parent = document->GetElementById("parent");
	Element* child = document->GetElementById("child");
	REQUIRE(ancestor); REQUIRE(parent); REQUIRE(child);
	REQUIRE_FALSE(ancestor->GetTransformState());
	REQUIRE(parent->GetTransformState()); REQUIRE(child->GetTransformState());
	MutableEventInstancer instancer;
	Factory::RegisterEventInstancer(&instancer);
	ProjectionListener listener;
	listener.mutate_on_capture = parent;
	ancestor->AddEventListener("click", &listener, true);
	parent->AddEventListener("click", &listener, true);
	child->AddEventListener("click", &listener);
	parent->AddEventListener("click", &listener);
	ancestor->AddEventListener("click", &listener);
	Dictionary parameters;
	parameters["mouse_x"] = 120.f;
	parameters["mouse_y"] = 80.f;
	CHECK(child->DispatchEvent("click", parameters));
	REQUIRE(listener.calls.size() == 5);
	const Vector<Element*> elements{ancestor, parent, child, parent, ancestor};
	const Vector<EventPhase> phases{EventPhase::Capture, EventPhase::Capture, EventPhase::Target, EventPhase::Bubble, EventPhase::Bubble};
	const Vector<Vector2f> positions{{120, 80}, {110, 80}, {110, 60}, {110, 80}, {120, 80}};
	for (size_t i = 0; i < listener.calls.size(); ++i)
	{
		CHECK(listener.calls[i].element == elements[i]);
		CHECK(listener.calls[i].phase == phases[i]);
		CHECK(listener.calls[i].position.x == doctest::Approx(positions[i].x));
		CHECK(listener.calls[i].position.y == doctest::Approx(positions[i].y));
		CHECK(listener.calls[i].screen == Vector2f(120, 80));
	}
	CHECK(Get<float>(parameters, "mouse_x", -1) == 120.f);
	CHECK(Get<float>(parameters, "mouse_y", -1) == 80.f);
	document->Close();
	TestsShell::ShutdownShell();
}

TEST_CASE("event.mouse_projection_failure_dispatch")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);
	ElementDocument* document = context->LoadDocumentFromMemory(R"(
<rml><head><style>#parent { transform: translateX(10px); } #child { transform: scale(0); }</style></head>
<body><div id="parent"><div id="child"/></div></body></rml>)");
	REQUIRE(document);
	document->Show();
	context->Update();
	context->Render();
	Element* parent = document->GetElementById("parent");
	Element* child = document->GetElementById("child");
	REQUIRE(parent); REQUIRE(child);
	Vector2f point(120, 80);
	REQUIRE(child->GetTransformState());
	REQUIRE_FALSE(child->Project(point));
	ProjectionListener listener;
	parent->AddEventListener("click", &listener, true);
	child->AddEventListener("click", &listener);
	parent->AddEventListener("click", &listener);
	Dictionary parameters;
	parameters["mouse_x"] = 120.f;
	parameters["mouse_y"] = 80.f;
	CHECK_FALSE(child->DispatchEvent("click", parameters));
	REQUIRE(listener.calls.size() == 2);
	CHECK(listener.calls[0].element == parent);
	CHECK(listener.calls[0].position.x == doctest::Approx(110));
	CHECK(listener.calls[1].element == child);
	CHECK(listener.calls[1].position == Vector2f(120, 80));
	CHECK(listener.calls[1].screen == Vector2f(120, 80));
	CHECK_FALSE(listener.calls[1].propagating);
	document->Close();
	TestsShell::ShutdownShell();
}
