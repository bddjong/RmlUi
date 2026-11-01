#include "EventParametersProxy.h"
#include <RmlUi/Core/Dictionary.h>
#include <RmlUi/Core/Variant.h>
#include <RmlUi/Lua/Utilities.h>
#include <cstring>

namespace Rml {
namespace Lua {

template <>
void ExtraInit<EventParametersProxy>(lua_State* L, int metatable_index)
{
	lua_pushcfunction(L, EventParametersProxy__index);
	lua_setfield(L, metatable_index, "__index");
	lua_pushcfunction(L, EventParametersProxy__pairs);
	lua_setfield(L, metatable_index, "__pairs");
}

int EventParametersProxy__index(lua_State* L)
{
	/*the table obj and the missing key are currently on the stack(index 1 & 2) as defined by the Lua language*/
	int keytype = lua_type(L, 2);
	if (keytype == LUA_TSTRING) // only valid key types
	{
		EventParametersProxy* obj = LuaType<EventParametersProxy>::check(L, 1);
		RMLUI_CHECK_OBJ(obj);
		RMLUI_CHECK_OBJ(obj->state->event);
		const char* key = lua_tostring(L, 2);
		auto it = obj->state->event->GetParameters().find(key);
		const Variant* param = (it == obj->state->event->GetParameters().end() ? nullptr : &it->second);
		if (obj->state->event->GetId() == EventId::Tabchange && std::strcmp(key, "tab_index") == 0 && param && param->GetType() == Variant::Type::INT)
		{
			PushIndex(L, param->Get<int>());
		}
		else
			PushVariant(L, param);
		return 1;
	}
	else
		return LuaType<EventParametersProxy>::index(L);
}

int EventParametersProxy__pairs(lua_State* L)
{
	EventParametersProxy* obj = LuaType<EventParametersProxy>::check(L, 1);
	RMLUI_CHECK_OBJ(obj);
	RMLUI_CHECK_OBJ(obj->state->event);
	lua_getglobal(L, "next");
	lua_newtable(L);
	for (const auto& parameter : obj->state->event->GetParameters())
	{
		lua_pushlstring(L, parameter.first.c_str(), parameter.first.size());
		PushVariant(L, &parameter.second);
		lua_rawset(L, -3);
	}
	lua_pushnil(L);
	return 3;
}

RegType<EventParametersProxy> EventParametersProxyMethods[] = {
	{nullptr, nullptr},
};
luaL_Reg EventParametersProxyGetters[] = {
	{nullptr, nullptr},
};
luaL_Reg EventParametersProxySetters[] = {
	{nullptr, nullptr},
};

RMLUI_LUATYPE_DEFINE(EventParametersProxy)
} // namespace Lua
} // namespace Rml
