#pragma once

#include <RmlUi/Core/Event.h>
#include <RmlUi/Lua/IncludeLua.h>
#include <memory>
#include <RmlUi/Lua/LuaType.h>

namespace Rml {
namespace Lua {
// Each callback owns a scope; Lua userdata and parameter proxies share its state.
struct EventState { Rml::Event* event; };
struct LuaEvent { std::shared_ptr<EventState> state; };
RMLUI_LUATYPE_DECLARE(LuaEvent)

struct EventScope {
	std::shared_ptr<EventState> state;
	explicit EventScope(Rml::Event& event) : state(std::make_shared<EventState>(EventState{&event})) {}
	~EventScope() { state->event = nullptr; }
	EventScope(const EventScope&) = delete;
	EventScope& operator=(const EventScope&) = delete;
	void Push(lua_State* L) const { LuaType<LuaEvent>::push(L, new LuaEvent{state}, true); }
};

template <>
void ExtraInit<LuaEvent>(lua_State* L, int metatable_index);
// methods
int EventStopPropagation(lua_State* L, LuaEvent* obj);
int EventStopImmediatePropagation(lua_State* L, LuaEvent* obj);

// getters
int EventGetAttrcurrent_element(lua_State* L);
int EventGetAttrtype(lua_State* L);
int EventGetAttrtarget_element(lua_State* L);
int EventGetAttrparameters(lua_State* L);

extern RegType<LuaEvent> EventMethods[];
extern luaL_Reg EventGetters[];
extern luaL_Reg EventSetters[];

} // namespace Lua
} // namespace Rml
