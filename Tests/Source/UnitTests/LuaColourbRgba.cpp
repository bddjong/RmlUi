#include "../../../Source/Lua/Colourb.h"
#include <doctest.h>
#include <string>

TEST_CASE("Lua Colourb rgba uses atomic raw table conversion")
{
	lua_State* L = luaL_newstate();
	REQUIRE(L);
	luaL_openlibs(L);
	Rml::Lua::LuaType<Rml::Colourb>::Register(L);
	const int result = luaL_dostring(L, R"lua(
		local c = Colourb.new(1, 2, 3, 4)
		local fields = {'red', 'green', 'blue', 'alpha'}
		local function check(a, b, d, e)
			local t = c.rgba
			assert(type(t) == 'table' and t[1] == a and t[2] == b and t[3] == d and t[4] == e)
			assert(c.red == a and c.green == b and c.blue == d and c.alpha == e)
		end
		c.rgba = {10, 20, 30, 40}; check(10, 20, 30, 40)
		local snapshot = c.rgba
		snapshot[1] = 99; check(10, 20, 30, 40)
		c.green = 77; assert(snapshot[2] == 20)
		c.rgba = snapshot; check(99, 20, 30, 40)
		c.rgba = c.rgba; check(99, 20, 30, 40)

		-- Compare success and conversion with each individual setter, including Lua's
		-- version-dependent treatment of fractional numbers and numeric strings.
		for _, value in ipairs({0, 255, 256, -1, 1.5, '42', '1.5', false, 'bad'}) do
			for i, field in ipairs(fields) do
				local single = Colourb.new(11, 22, 33, 44)
				local all = Colourb.new(11, 22, 33, 44)
				local t = {11, 22, 33, 44}; t[i] = value
				local ok1 = pcall(function() single[field] = value end)
				local ok2 = pcall(function() all.rgba = t end)
				assert(ok1 == ok2)
				for j, name in ipairs(fields) do
					assert(all[name] == (ok1 and single[name] or ({11, 22, 33, 44})[j]))
				end
			end
		end
		for i = 1, 4 do
			for _, bad in ipairs({false, 'bad'}) do
				local t = {1, 2, 3, 4}; t[i] = bad
				assert(not pcall(function() c.rgba = t end)); check(99, 20, 30, 40)
			end
			local t = {1, 2, 3, 4}; t[i] = nil
			assert(not pcall(function() c.rgba = t end)); check(99, 20, 30, 40)
		end
		assert(not pcall(function() c.rgba = 42 end)); check(99, 20, 30, 40)
		local calls = 0
		local t = setmetatable({1, 2, 3, 4, 5, extra = false}, {__index = function() calls = calls + 1; return 9 end})
		c.rgba = t; check(1, 2, 3, 4)
		t[2] = nil
		assert(not pcall(function() c.rgba = t end)); assert(calls == 0); check(1, 2, 3, 4)
		t = nil; collectgarbage('collect'); check(1, 2, 3, 4)
		local retained = c.rgba; c = nil; collectgarbage('collect')
		assert(retained[1] == 1 and retained[2] == 2 and retained[3] == 3 and retained[4] == 4)
		retained = nil; collectgarbage('collect')
	)lua");
	if (result != 0) { INFO(std::string(lua_tostring(L, -1))); CHECK(result == 0); }
	lua_close(L);
}
