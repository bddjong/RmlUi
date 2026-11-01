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
