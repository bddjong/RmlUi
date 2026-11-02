#include "../Common/TestsShell.h"
#include "../../../Source/Lua/Element.h"
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Lua/Lua.h>
#include <doctest.h>

using namespace Rml;

TEST_CASE("Lua style snapshots survive mutation, destruction, nested calls and errors")
{
	lua_State* L = luaL_newstate();
	REQUIRE(L);
	luaL_openlibs(L);
	Lua::Initialise(L);
	Context* context = TestsShell::GetContext();
	REQUIRE(context);
	ElementDocument* doc = context->LoadDocumentFromMemory("<rml><body><div id='victim' style='width: 10px; height: 20px'/></body></rml>");
	REQUIRE(doc);
	Element* victim = doc->GetElementById("victim");
	REQUIRE(victim);
	Lua::LuaType<Element>::push(L, victim, false);
	lua_setglobal(L, "element");
	lua_pushlightuserdata(L, victim);
	lua_pushcclosure(L, [](lua_State* state) -> int {
		auto* element = static_cast<Element*>(lua_touserdata(state, lua_upvalueindex(1)));
		element->GetParentNode()->RemoveChild(element);
		return 0;
	}, 1);
	lua_setglobal(L, "destroy_victim");
	auto run = [L](const char* script) {
		int result = luaL_dostring(L, script);
		if (result) { INFO(String(lua_tostring(L, -1))); CHECK(result == 0); lua_pop(L, 1); }
	};
	run(R"(
		style = element.style; element = nil
		assert(style.width == '10px')
		local ok, err = pcall(function()
			it, state, key = pairs(style)
			style.width = '30px'; style.height = nil; style.color = 'red'
			local old = next; next = nil
			local nested = {}; for k,v in pairs(style) do nested[k] = v end
			next = old
			assert(nested.width == '30px' and nested.height == nil)
			error('expected')
		end)
		assert(not ok and string.find(err, 'expected', 1, true))
		local snapshot = {}; for k,v in it,state,key do snapshot[k] = v end
		assert(snapshot.width == '10px' and snapshot.height == '20px' and snapshot.color == nil)
		local seen, first = {}, true
		for k,v in pairs(style) do
			seen[k] = v
			if first then
				first = false
				style.width = '50px'
				destroy_victim(); destroy_victim = nil
			end
		end
		assert(seen.width == '30px' and seen.color ~= nil)
	)");
	run(R"(
		collectgarbage('collect')
		assert(style.width == nil); style.width = '40px'; style.height = nil
		local snapshot = {}; for k,v in it,state,key do snapshot[k] = v end
		assert(snapshot.width == '10px' and snapshot.height == '20px')
		local old = next; next = nil
		for _ in pairs(style) do error('expired style not empty') end
		next = old
		style = nil; it = nil; state = nil; collectgarbage('collect')
	)");
	doc->Close();
	TestsShell::ShutdownShell();
	lua_close(L);
}
