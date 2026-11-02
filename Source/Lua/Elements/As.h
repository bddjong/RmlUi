#pragma once
/*
    These are helper functions to fill up the Element.As table with types that are able to be casted
*/

#include <RmlUi/Lua/Utilities.h>

namespace Rml {
namespace Lua {

// Adds to the Element.As table the name of the type, and the function to use to cast
template <typename T>
void AddCastFunctionToElementAsTable(lua_State* L)
{
	int top = lua_gettop(L);
	lua_getglobal(L, "Element");
	lua_getfield(L, -1, "As");
	if (!lua_isnoneornil(L, -1))
	{
		lua_pushcfunction(L, CastFromElementTo<T>);
		lua_setfield(L, -2, GetTClassName<T>());
	}
	lua_settop(L, top); // pop "As" and "Element"
}

} // namespace Lua
} // namespace Rml
