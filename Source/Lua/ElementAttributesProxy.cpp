#include "ElementAttributesProxy.h"
#include "Pairs.h"
#include <RmlUi/Core/Variant.h>
#include <RmlUi/Lua/Utilities.h>

namespace Rml {
namespace Lua {
template <>
void ExtraInit<ElementAttributesProxy>(lua_State* L, int metatable_index)
{
	lua_pushcfunction(L, ElementAttributesProxy__index);
	lua_setfield(L, metatable_index, "__index");
	lua_pushcfunction(L, ElementAttributesProxy__pairs);
	lua_setfield(L, metatable_index, "__pairs");
}

int ElementAttributesProxy__index(lua_State* L)
{
	/*the table obj and the missing key are currently on the stack(index 1 & 2) as defined by the Lua language*/
	int keytype = lua_type(L, 2);
	if (keytype == LUA_TSTRING) // only valid key types
	{
		ElementAttributesProxy* obj = LuaType<ElementAttributesProxy>::check(L, 1);
		RMLUI_CHECK_OBJ(obj);
		RMLUI_CHECK_OBJ(obj->owner.get());
		const char* key = lua_tostring(L, 2);
		Variant* attr = obj->owner->GetAttribute(key);
		PushVariant(L, attr); // Utilities.h
		return 1;
	}
	else
		return LuaType<ElementAttributesProxy>::index(L);
}

int ElementAttributesProxy__pairs(lua_State* L)
{
	ElementAttributesProxy* obj = LuaType<ElementAttributesProxy>::check(L, 1);
	RMLUI_CHECK_OBJ(obj);
	RMLUI_CHECK_OBJ(obj->owner.get());
	// Snapshot attributes so iteration remains valid after the element changes or dies.
	lua_getglobal(L, "next");
	lua_newtable(L);
	for (const auto& attribute : obj->owner->GetAttributes())
	{
		PushVariant(L, &attribute.second);
		lua_setfield(L, -2, attribute.first.c_str());
	}
	lua_pushnil(L);
	return 3;
}

RegType<ElementAttributesProxy> ElementAttributesProxyMethods[] = {
	{nullptr, nullptr},
};

luaL_Reg ElementAttributesProxyGetters[] = {
	{nullptr, nullptr},
};
luaL_Reg ElementAttributesProxySetters[] = {
	{nullptr, nullptr},
};

RMLUI_LUATYPE_DEFINE(ElementAttributesProxy)
} // namespace Lua
} // namespace Rml
