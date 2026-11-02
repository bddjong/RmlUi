#include "../../Source/Lua/Colourb.h"
#include <cstdio>

int main(int argc, char** argv)
{
	lua_State* L = luaL_newstate();
	luaL_openlibs(L);
	Rml::Lua::LuaType<Rml::Colourb>::Register(L);
	const int result = luaL_dofile(L, argc > 1 ? argv[1] : "colourb.lua");
	if (result != 0)
		std::fprintf(stderr, "%s\n", lua_tostring(L, -1));
	lua_close(L);
	return result != 0;
}
