#include "Colourb.h"

namespace Rml {
namespace Lua {

// Keep the constructor and channel setters' existing Lua integer conversion.
static byte CheckComponent(lua_State* L, int index)
{
	return (byte)luaL_checkinteger(L, index);
}

template <>
void ExtraInit<Colourb>(lua_State* L, int metatable_index)
{
	lua_pushcfunction(L, Colourbnew);
	lua_setfield(L, metatable_index - 1, "new");

	lua_pushcfunction(L, Colourb__eq);
	lua_setfield(L, metatable_index, "__eq");

	lua_pushcfunction(L, Colourb__add);
	lua_setfield(L, metatable_index, "__add");

	lua_pushcfunction(L, Colourb__mul);
	lua_setfield(L, metatable_index, "__mul");

	return;
}
int Colourbnew(lua_State* L)
{
	byte red = CheckComponent(L, 1);
	byte green = CheckComponent(L, 2);
	byte blue = CheckComponent(L, 3);
	byte alpha = CheckComponent(L, 4);

	Colourb* col = new Colourb(red, green, blue, alpha);

	LuaType<Colourb>::push(L, col, true);
	return 1;
}

int Colourb__eq(lua_State* L)
{
	Colourb* lhs = LuaType<Colourb>::check(L, 1);
	RMLUI_CHECK_OBJ(lhs);
	Colourb* rhs = LuaType<Colourb>::check(L, 2);
	RMLUI_CHECK_OBJ(rhs);

	lua_pushboolean(L, (*lhs) == (*rhs) ? 1 : 0);
	return 1;
}

int Colourb__add(lua_State* L)
{
	Colourb* lhs = LuaType<Colourb>::check(L, 1);
	RMLUI_CHECK_OBJ(lhs);
	Colourb* rhs = LuaType<Colourb>::check(L, 2);
	RMLUI_CHECK_OBJ(rhs);

	Colourb* res = new Colourb((*lhs) + (*rhs));

	LuaType<Colourb>::push(L, res, true);
	return 1;
}

int Colourb__mul(lua_State* L)
{
	Colourb* lhs = LuaType<Colourb>::check(L, 1);
	RMLUI_CHECK_OBJ(lhs);
	float rhs = (float)luaL_checknumber(L, 2);

	Colourb* res = new Colourb((*lhs) * rhs);

	LuaType<Colourb>::push(L, res, true);
	return 1;
}

// getters
int ColourbGetAttrred(lua_State* L)
{
	Colourb* obj = LuaType<Colourb>::check(L, 1);
	RMLUI_CHECK_OBJ(obj);
	lua_pushinteger(L, obj->red);
	return 1;
}

int ColourbGetAttrgreen(lua_State* L)
{
	Colourb* obj = LuaType<Colourb>::check(L, 1);
	RMLUI_CHECK_OBJ(obj);
	lua_pushinteger(L, obj->green);
	return 1;
}

int ColourbGetAttrblue(lua_State* L)
{
	Colourb* obj = LuaType<Colourb>::check(L, 1);
	RMLUI_CHECK_OBJ(obj);
	lua_pushinteger(L, obj->blue);
	return 1;
}

int ColourbGetAttralpha(lua_State* L)
{
	Colourb* obj = LuaType<Colourb>::check(L, 1);
	RMLUI_CHECK_OBJ(obj);
	lua_pushinteger(L, obj->alpha);
	return 1;
}

int ColourbGetAttrrgba(lua_State* L)
{
	Colourb* obj = LuaType<Colourb>::check(L, 1);
	RMLUI_CHECK_OBJ(obj);
	lua_createtable(L, 4, 0);
	lua_pushinteger(L, obj->red); lua_rawseti(L, -2, 1);
	lua_pushinteger(L, obj->green); lua_rawseti(L, -2, 2);
	lua_pushinteger(L, obj->blue); lua_rawseti(L, -2, 3);
	lua_pushinteger(L, obj->alpha); lua_rawseti(L, -2, 4);
	return 1;
}

// setters
int ColourbSetAttrred(lua_State* L)
{
	Colourb* obj = LuaType<Colourb>::check(L, 1);
	RMLUI_CHECK_OBJ(obj);
	byte red = CheckComponent(L, 2);
	obj->red = red;
	return 0;
}

int ColourbSetAttrgreen(lua_State* L)
{
	Colourb* obj = LuaType<Colourb>::check(L, 1);
	RMLUI_CHECK_OBJ(obj);
	byte green = CheckComponent(L, 2);
	obj->green = green;
	return 0;
}

int ColourbSetAttrblue(lua_State* L)
{
	Colourb* obj = LuaType<Colourb>::check(L, 1);
	RMLUI_CHECK_OBJ(obj);
	byte blue = CheckComponent(L, 2);
	obj->blue = blue;
	return 0;
}

int ColourbSetAttralpha(lua_State* L)
{
	Colourb* obj = LuaType<Colourb>::check(L, 1);
	RMLUI_CHECK_OBJ(obj);
	byte alpha = CheckComponent(L, 2);
	obj->alpha = alpha;
	return 0;
}

int ColourbSetAttrrgba(lua_State* L)
{
	Colourb* obj = LuaType<Colourb>::check(L, 1);
	RMLUI_CHECK_OBJ(obj);
	luaL_checktype(L, 2, LUA_TTABLE);
	byte components[4];
	for (int i = 0; i < 4; ++i)
	{
		lua_rawgeti(L, 2, i + 1);
		components[i] = CheckComponent(L, -1);
		lua_pop(L, 1);
	}
	obj->red = components[0];
	obj->green = components[1];
	obj->blue = components[2];
	obj->alpha = components[3];
	return 0;
}

RegType<Colourb> ColourbMethods[] = {
	{nullptr, nullptr},
};

luaL_Reg ColourbGetters[] = {
	RMLUI_LUAGETTER(Colourb, red),
	RMLUI_LUAGETTER(Colourb, green),
	RMLUI_LUAGETTER(Colourb, blue),
	RMLUI_LUAGETTER(Colourb, alpha),
	RMLUI_LUAGETTER(Colourb, rgba),
	{nullptr, nullptr},
};

luaL_Reg ColourbSetters[] = {
	RMLUI_LUASETTER(Colourb, red),
	RMLUI_LUASETTER(Colourb, green),
	RMLUI_LUASETTER(Colourb, blue),
	RMLUI_LUASETTER(Colourb, alpha),
	RMLUI_LUASETTER(Colourb, rgba),
	{nullptr, nullptr},
};

RMLUI_LUATYPE_DEFINE(Colourb)
} // namespace Lua
} // namespace Rml
