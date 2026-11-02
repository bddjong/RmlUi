#include "../../../Source/Lua/Event.h"
#include "../../../Source/Lua/EventParametersProxy.h"
#include <RmlUi/Core/Dictionary.h>
#include <doctest.h>

using namespace Rml;
using namespace Rml::Lua;

TEST_CASE("Lua event handles expire after callback, including errors")
{
	lua_State* L = luaL_newstate();
	REQUIRE(L);
	luaL_openlibs(L);
	LuaType<LuaEvent>::Register(L);
	LuaType<EventParametersProxy>::Register(L);

	Dictionary parameters;
	parameters["value"] = 42;
	Event event(nullptr, EventId::Invalid, "test", parameters, true);
	for (bool fail : {false, true})
	{
		{
			EventScope scope(event);
			scope.Push(L);
			lua_setglobal(L, "event");
#if LUA_VERSION_NUM >= 502
			const char* script = fail
				? "saved = event; params = event.parameters; iter, state, key = pairs(params); error('expected')"
				: "saved = event; params = event.parameters; iter, state, key = pairs(params); assert(saved.type == 'test' and params.value == 42)";
#else
			const char* script = fail
				? "saved = event; params = event.parameters; error('expected')"
				: "saved = event; params = event.parameters; assert(saved.type == 'test' and params.value == 42)";
#endif
			REQUIRE(luaL_loadstring(L, script) == 0);
			CHECK(lua_pcall(L, 0, LUA_MULTRET, 0) == (fail ? LUA_ERRRUN : 0));
			if (fail) lua_pop(L, 1);
		}
		// The event itself remains alive: expiration is tied to callback scope, not destruction.
#if LUA_VERSION_NUM >= 502
		CHECK(luaL_dostring(L, "assert(saved.type == nil and params.value == nil); saved:StopPropagation(); "
			"local k, v = iter(state, key); assert(k == 'value' and v == 42)") == 0);
#else
		CHECK(luaL_dostring(L, "assert(saved.type == nil and params.value == nil); saved:StopPropagation()") == 0);
#endif
		CHECK(event.IsPropagating());
	}
	lua_close(L);
}

#include "../Common/TestsShell.h"
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Lua/Lua.h>
#include "../../../Source/Lua/Context.h"
#include "../../../Source/Lua/Element.h"

TEST_CASE("Lua production callbacks expire retained events through nested dispatch and errors")
{
	lua_State* L = luaL_newstate();
	REQUIRE(L);
	luaL_openlibs(L);
	Lua::Initialise(L);
	Context* context = TestsShell::GetContext();
	REQUIRE(context);
	LuaType<Context>::push(L, context, false);
	lua_setglobal(L, "ctx");
	const char* setup = R"(
		refs = {}; count = 0
		model = ctx:OpenDataModel('event_model', { callback = function(e)
			count = count + 1
			refs[#refs+1] = {e, e.parameters}
			assert(e.type == 'click' and e.parameters.value == 42)
			if count == 1 then
				inner:DispatchEvent('click', {value=42})
				assert(e.type == 'click' and e.parameters.value == 42)
				assert(refs[2][1].type == nil and refs[2][2].value == nil)
			elseif count == 2 then
				error('expected data model error')
			end
		end })
	)";
	int result = luaL_dostring(L, setup);
	if (result) INFO(lua_tostring(L, -1));
	REQUIRE(result == 0);
	ElementDocument* doc = context->LoadDocumentFromMemory(
		"<rml><body data-model='event_model'><div id='outer' data-event-click='callback()'/><div id='inner' data-event-click='callback()'/><div id='listener'/><div id='tablistener'/></body></rml>");
	REQUIRE(doc);
	Element* outer = doc->GetElementById("outer");
	Element* inner = doc->GetElementById("inner");
	Element* listener = doc->GetElementById("listener");
	Element* tablistener = doc->GetElementById("tablistener");
	REQUIRE(outer); REQUIRE(inner); REQUIRE(listener); REQUIRE(tablistener);
	LuaType<Element>::push(L, inner, false); lua_setglobal(L, "inner");
	LuaType<Element>::push(L, listener, false); lua_setglobal(L, "listener");
	LuaType<Element>::push(L, tablistener, false); lua_setglobal(L, "tablistener");
	const char* listener_setup = R"(
		listener:AddEventListener('click', function(e)
			refs[#refs+1] = {e, e.parameters}
			assert(e.parameters.value == 42)
			local iter, state, key = pairs(e.parameters)
			local k, v = iter(state, key); assert(k == 'value' and v == 42)
			snapshot = {iter, state, key}
			error('expected listener error')
		end)
		tablistener:AddEventListener('tabchange', function(e)
			local direct, iterated = e.parameters.tab_index, nil
			local old_next = next; next = nil
			for k, v in pairs(e.parameters) do if k == 'tab_index' then iterated = v end end
			next = old_next
			assert(direct == 1 and iterated == direct)
			tab_checked = true
		end)
	)";
	result = luaL_dostring(L, listener_setup);
	if (result) INFO(lua_tostring(L, -1));
	REQUIRE(result == 0);
	Dictionary params; params["value"] = 42;
	Dictionary tab_params; tab_params["tab_index"] = 0;
	tablistener->DispatchEvent("tabchange", tab_params);
	CHECK(luaL_dostring(L, "assert(tab_checked)") == 0);
	TestsShell::SetNumExpectedWarnings(4);
	for (int i = 0; i < 3; ++i) {
		outer->DispatchEvent("click", params);
		listener->DispatchEvent("click", params);
		result = luaL_dostring(L, R"(
			collectgarbage('collect')
			local k, v = snapshot[1](snapshot[2], snapshot[3]); assert(k == 'value' and v == 42)
			for _, r in ipairs(refs) do
				assert(r[1].type == nil and r[1].parameters == nil and r[2].value == nil)
				r[1]:StopPropagation(); r[1]:StopImmediatePropagation()
				local n = 0; for _ in pairs(r[2]) do n = n + 1 end; assert(n == 0)
			end
		)");
		if (result) INFO(lua_tostring(L, -1));
		CHECK(result == 0);
	}
	TestsShell::SetNumExpectedWarnings(0);
	CHECK(luaL_dostring(L, "assert(count == 4 and #refs == 7); local p = refs[1][2]; refs = nil; snapshot = nil; collectgarbage('collect'); assert(p.value == nil); local old_next = next; next = nil; for _ in pairs(p) do error('expired proxy not empty') end; next = old_next; p = nil; collectgarbage('collect')") == 0);
	doc->Close();
	TestsShell::ShutdownShell();
	lua_close(L);
}
