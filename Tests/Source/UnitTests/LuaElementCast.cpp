#include "../Common/TestsShell.h"
#include "../../../Source/Lua/Element.h"
#include "../../../Source/Lua/Elements/ElementFormControl.h"
#include "../../../Source/Lua/Elements/ElementFormControlInput.h"
#include "../../../Source/Lua/Elements/ElementFormControlSelect.h"
#include "../../../Source/Lua/Elements/ElementFormControlTextArea.h"
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/ElementInstancer.h>
#include <RmlUi/Core/Factory.h>
#include <RmlUi/Lua/Utilities.h>
#include <doctest.h>

using namespace Rml;
using namespace Rml::Lua;

class PlainLuaElement : public Element {
public:
	using Element::Element;
};
class RttiLuaElement : public Element {
public:
	using Element::Element;
	RMLUI_RTTI_DeclareWithParent(RttiLuaElement, Element)
};
RMLUI_RTTI_Define(RttiLuaElement)

namespace Rml { namespace Lua {
template <> const char* GetTClassName<PlainLuaElement>() { return "PlainLuaElement"; }
template <> const char* GetTClassName<RttiLuaElement>() { return "RttiLuaElement"; }
} }

class PlainLuaInstancer : public ElementInstancer {
public:
	ElementPtr InstanceElement(Element*, const String& tag, const XMLAttributes&) override { return ElementPtr(new PlainLuaElement(tag)); }
	void ReleaseElement(Element* element) override { delete element; }
};

TEST_CASE("Lua element casts check control types")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);
	ElementDocument* document = context->LoadDocumentFromMemory(
		"<rml><body><div id='div'/><input id='input'/><select id='select'/><textarea id='area'/></body></rml>");
	REQUIRE(document);
	lua_State* L = luaL_newstate();
	REQUIRE(L);
	luaL_openlibs(L);
	LuaType<Element>::Register(L);
	LuaType<ElementFormControl>::Register(L);
	LuaType<ElementFormControlInput>::Register(L);
	LuaType<ElementFormControlSelect>::Register(L);
	LuaType<ElementFormControlTextArea>::Register(L);
	for (const char* name : {"PlainLuaElement", "RttiLuaElement"})
	{
		luaL_newmetatable(L, name);
		lua_pop(L, 1);
	}
	int top = lua_gettop(L);
	AddTypeToElementAsTable<PlainLuaElement>(L);
	AddTypeToElementAsTable<RttiLuaElement>(L);
	CHECK(lua_gettop(L) == top);
	PlainLuaInstancer instancer;
	Factory::RegisterElementInstancer("custom-lua-cast", &instancer);
	ElementDocument* custom_document = context->LoadDocumentFromMemory("<rml><body><custom-lua-cast id='custom'/></body></rml>");
	REQUIRE(custom_document);
	Element* custom = custom_document->GetElementById("custom");
	REQUIRE(custom);
	auto rtti_element = MakeUnique<RttiLuaElement>("rtti");
	LuaType<Element>::push(L, custom, false);
	lua_setglobal(L, "custom");
	LuaType<Element>::push(L, rtti_element.get(), false);
	lua_setglobal(L, "rtti_element");
	for (const char* id : {"div", "input", "select", "area"})
	{
		Element* element = document->GetElementById(id);
		REQUIRE(element);
		LuaType<Element>::push(L, element, false);
		lua_setglobal(L, id);
	}
	const char* script = R"(
		local kinds = {'ElementFormControl', 'ElementFormControlInput', 'ElementFormControlSelect', 'ElementFormControlTextArea'}
		local elements = {div, input, select, area}
		for i, element in ipairs(elements) do
			for j, kind in ipairs(kinds) do
				local value = Element.As[kind](element)
				local valid = i > 1 and (j == 1 or j == i)
				assert((value ~= nil) == valid, i .. ':' .. j)
				if valid then value.name = 'test'; assert(value.name == 'test') end
			end
		end
		assert(Element.As.ElementFormControl(nil) == nil)
		assert(Element.As.RttiLuaElement(rtti_element) ~= nil)
		assert(Element.As.RttiLuaElement(custom) == nil)
		assert(Element.As.PlainLuaElement(rtti_element) == nil)
		assert(Element.As.PlainLuaElement(nil) == nil)
		collectgarbage('collect')
		assert(Element.As.ElementFormControlInput(input).name == 'test')
	)";
	int result = luaL_dostring(L, script);
	if (result != 0) INFO(lua_tostring(L, -1));
	CHECK(result == 0);
#ifdef RMLUI_HAS_RTTI
	CHECK(luaL_dostring(L, "assert(Element.As.PlainLuaElement(custom) ~= nil); collectgarbage('collect'); assert(Element.As.PlainLuaElement(custom) ~= nil)") == 0);
#else
	CHECK(luaL_dostring(L, "assert(Element.As.PlainLuaElement(custom) == nil)") == 0);
#endif
	lua_close(L);
	rtti_element.reset();
	custom_document->Close();
	document->Close();
	TestsShell::ShutdownShell();
}
