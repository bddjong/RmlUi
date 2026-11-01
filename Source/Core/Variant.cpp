#include "../../Include/RmlUi/Core/Variant.h"
#include "../../Include/RmlUi/Core/DecorationTypes.h"
#include <new>

namespace Rml {

Variant::Variant()
{
	// Reject unsupported custom types at compile time.
#define CHECK_VARIANT_STORAGE(T) \
	static_assert(sizeof(T) <= LOCAL_DATA_SIZE && alignof(Variant) % alignof(T) == 0, "Variant storage cannot hold " #T); \
	static_assert(std::is_nothrow_move_constructible_v<T> && std::is_nothrow_move_assignable_v<T> && \
		std::is_nothrow_destructible_v<T>, "Variant requires noexcept move and destruction for " #T)
	CHECK_VARIANT_STORAGE(bool);
	CHECK_VARIANT_STORAGE(byte);
	CHECK_VARIANT_STORAGE(char);
	CHECK_VARIANT_STORAGE(float);
	CHECK_VARIANT_STORAGE(double);
	CHECK_VARIANT_STORAGE(int);
	CHECK_VARIANT_STORAGE(int64_t);
	CHECK_VARIANT_STORAGE(unsigned int);
	CHECK_VARIANT_STORAGE(uint64_t);
	CHECK_VARIANT_STORAGE(String);
	CHECK_VARIANT_STORAGE(Vector2f);
	CHECK_VARIANT_STORAGE(Vector3f);
	CHECK_VARIANT_STORAGE(Vector4f);
	CHECK_VARIANT_STORAGE(Colourf);
	CHECK_VARIANT_STORAGE(Colourb);
	CHECK_VARIANT_STORAGE(ScriptInterface*);
	CHECK_VARIANT_STORAGE(void*);
	CHECK_VARIANT_STORAGE(TransformPtr);
	CHECK_VARIANT_STORAGE(TransitionList);
	CHECK_VARIANT_STORAGE(AnimationList);
	CHECK_VARIANT_STORAGE(DecoratorsPtr);
	CHECK_VARIANT_STORAGE(FiltersPtr);
	CHECK_VARIANT_STORAGE(FontEffectsPtr);
	CHECK_VARIANT_STORAGE(ColorStopList);
	CHECK_VARIANT_STORAGE(BoxShadowList);
#undef CHECK_VARIANT_STORAGE
}

Variant::Variant(const Variant& copy)
{
	Set(copy);
}

Variant::Variant(Variant&& other) noexcept
{
	Set(std::move(other));
}

Variant::~Variant()
{
	Clear();
}

void Variant::Clear()
{
	// Free any allocated types.
	switch (type)
	{
	case STRING:
	{
		// Clean up the string.
		String* string = std::launder(reinterpret_cast<String*>(data));
		string->~String();
	}
	break;
	case TRANSFORMPTR:
	{
		// Clean up the transform.
		TransformPtr* transform = std::launder(reinterpret_cast<TransformPtr*>(data));
		transform->~TransformPtr();
	}
	break;
	case TRANSITIONLIST:
	{
		// Clean up the transition list.
		TransitionList* transition_list = std::launder(reinterpret_cast<TransitionList*>(data));
		transition_list->~TransitionList();
	}
	break;
	case ANIMATIONLIST:
	{
		// Clean up the transition list.
		AnimationList* animation_list = std::launder(reinterpret_cast<AnimationList*>(data));
		animation_list->~AnimationList();
	}
	break;
	case DECORATORSPTR:
	{
		DecoratorsPtr* decorators = std::launder(reinterpret_cast<DecoratorsPtr*>(data));
		decorators->~DecoratorsPtr();
	}
	break;
	case FILTERSPTR:
	{
		FiltersPtr* decorators = std::launder(reinterpret_cast<FiltersPtr*>(data));
		decorators->~FiltersPtr();
	}
	break;
	case FONTEFFECTSPTR:
	{
		FontEffectsPtr* font_effects = std::launder(reinterpret_cast<FontEffectsPtr*>(data));
		font_effects->~FontEffectsPtr();
	}
	break;
	case COLORSTOPLIST:
	{
		ColorStopList* value = std::launder(reinterpret_cast<ColorStopList*>(data));
		value->~ColorStopList();
	}
	break;
	case BOXSHADOWLIST:
	{
		BoxShadowList* value = std::launder(reinterpret_cast<BoxShadowList*>(data));
		value->~BoxShadowList();
	}
	break;
	default: break;
	}
	type = NONE;
}

#define SET_VARIANT(T) *std::launder(reinterpret_cast<T*>(data)) = value;
#define SET_SCALAR(T, tag) \
	if (type == tag) SET_VARIANT(T) \
	else { new (data) T(value); type = tag; }

void Variant::Set(const Variant& copy)
{
	switch (copy.type)
	{
	case STRING: Set(*std::launder(reinterpret_cast<const String*>(copy.data))); break;
	case TRANSFORMPTR: Set(*std::launder(reinterpret_cast<const TransformPtr*>(copy.data))); break;
	case TRANSITIONLIST: Set(*std::launder(reinterpret_cast<const TransitionList*>(copy.data))); break;
	case ANIMATIONLIST: Set(*std::launder(reinterpret_cast<const AnimationList*>(copy.data))); break;
	case DECORATORSPTR: Set(*std::launder(reinterpret_cast<const DecoratorsPtr*>(copy.data))); break;
	case FILTERSPTR: Set(*std::launder(reinterpret_cast<const FiltersPtr*>(copy.data))); break;
	case FONTEFFECTSPTR: Set(*std::launder(reinterpret_cast<const FontEffectsPtr*>(copy.data))); break;
	case COLORSTOPLIST: Set(*std::launder(reinterpret_cast<const ColorStopList*>(copy.data))); break;
	case BOXSHADOWLIST: Set(*std::launder(reinterpret_cast<const BoxShadowList*>(copy.data))); break;
	case BOOL: Set(*std::launder(reinterpret_cast<bool const*>(copy.data))); break;
	case BYTE: Set(*std::launder(reinterpret_cast<byte const*>(copy.data))); break;
	case CHAR: Set(*std::launder(reinterpret_cast<char const*>(copy.data))); break;
	case FLOAT: Set(*std::launder(reinterpret_cast<float const*>(copy.data))); break;
	case DOUBLE: Set(*std::launder(reinterpret_cast<double const*>(copy.data))); break;
	case INT: Set(*std::launder(reinterpret_cast<int const*>(copy.data))); break;
	case INT64: Set(*std::launder(reinterpret_cast<int64_t const*>(copy.data))); break;
	case UINT: Set(*std::launder(reinterpret_cast<unsigned int const*>(copy.data))); break;
	case UINT64: Set(*std::launder(reinterpret_cast<uint64_t const*>(copy.data))); break;
	case VECTOR2: Set(*std::launder(reinterpret_cast<Vector2f const*>(copy.data))); break;
	case VECTOR3: Set(*std::launder(reinterpret_cast<Vector3f const*>(copy.data))); break;
	case VECTOR4: Set(*std::launder(reinterpret_cast<Vector4f const*>(copy.data))); break;
	case COLOURF: Set(*std::launder(reinterpret_cast<Colourf const*>(copy.data))); break;
	case COLOURB: Set(*std::launder(reinterpret_cast<Colourb const*>(copy.data))); break;
	case SCRIPTINTERFACE: Set(*std::launder(reinterpret_cast<ScriptInterface* const*>(copy.data))); break;
	case VOIDPTR: Set(*std::launder(reinterpret_cast<void* const*>(copy.data))); break;
	case NONE: break;
	}
	RMLUI_ASSERT(type == copy.type);
}

void Variant::Set(Variant&& other)
{
	switch (other.type)
	{
	case STRING: Set(std::move(*std::launder(reinterpret_cast<String*>(other.data)))); break;
	case TRANSFORMPTR: Set(std::move(*std::launder(reinterpret_cast<TransformPtr*>(other.data)))); break;
	case TRANSITIONLIST: Set(std::move(*std::launder(reinterpret_cast<TransitionList*>(other.data)))); break;
	case ANIMATIONLIST: Set(std::move(*std::launder(reinterpret_cast<AnimationList*>(other.data)))); break;
	case DECORATORSPTR: Set(std::move(*std::launder(reinterpret_cast<DecoratorsPtr*>(other.data)))); break;
	case FILTERSPTR: Set(std::move(*std::launder(reinterpret_cast<FiltersPtr*>(other.data)))); break;
	case FONTEFFECTSPTR: Set(std::move(*std::launder(reinterpret_cast<FontEffectsPtr*>(other.data)))); break;
	case COLORSTOPLIST: Set(std::move(*std::launder(reinterpret_cast<ColorStopList*>(other.data)))); break;
	case BOXSHADOWLIST: Set(std::move(*std::launder(reinterpret_cast<BoxShadowList*>(other.data)))); break;
	case BOOL: Set(*std::launder(reinterpret_cast<bool const*>(other.data))); break;
	case BYTE: Set(*std::launder(reinterpret_cast<byte const*>(other.data))); break;
	case CHAR: Set(*std::launder(reinterpret_cast<char const*>(other.data))); break;
	case FLOAT: Set(*std::launder(reinterpret_cast<float const*>(other.data))); break;
	case DOUBLE: Set(*std::launder(reinterpret_cast<double const*>(other.data))); break;
	case INT: Set(*std::launder(reinterpret_cast<int const*>(other.data))); break;
	case INT64: Set(*std::launder(reinterpret_cast<int64_t const*>(other.data))); break;
	case UINT: Set(*std::launder(reinterpret_cast<unsigned int const*>(other.data))); break;
	case UINT64: Set(*std::launder(reinterpret_cast<uint64_t const*>(other.data))); break;
	case VECTOR2: Set(*std::launder(reinterpret_cast<Vector2f const*>(other.data))); break;
	case VECTOR3: Set(*std::launder(reinterpret_cast<Vector3f const*>(other.data))); break;
	case VECTOR4: Set(*std::launder(reinterpret_cast<Vector4f const*>(other.data))); break;
	case COLOURF: Set(*std::launder(reinterpret_cast<Colourf const*>(other.data))); break;
	case COLOURB: Set(*std::launder(reinterpret_cast<Colourb const*>(other.data))); break;
	case SCRIPTINTERFACE: Set(*std::launder(reinterpret_cast<ScriptInterface* const*>(other.data))); break;
	case VOIDPTR: Set(*std::launder(reinterpret_cast<void* const*>(other.data))); break;
	case NONE: break;
	}
	RMLUI_ASSERT(type == other.type);
}

void Variant::Set(const bool value)
{
	SET_SCALAR(bool, BOOL);
}

void Variant::Set(const byte value)
{
	SET_SCALAR(byte, BYTE);
}

void Variant::Set(const char value)
{
	SET_SCALAR(char, CHAR);
}

void Variant::Set(const float value)
{
	SET_SCALAR(float, FLOAT);
}

void Variant::Set(const double value)
{
	SET_SCALAR(double, DOUBLE);
}

void Variant::Set(const int value)
{
	SET_SCALAR(int, INT);
}

void Variant::Set(const int64_t value)
{
	SET_SCALAR(int64_t, INT64);
}

void Variant::Set(const unsigned int value)
{
	SET_SCALAR(unsigned int, UINT);
}

void Variant::Set(const uint64_t value)
{
	SET_SCALAR(uint64_t, UINT64);
}

void Variant::Set(const char* value)
{
	Set(String(value));
}

void Variant::Set(void* value)
{
	using Pointer = void*;
	SET_SCALAR(Pointer, VOIDPTR);
}

void Variant::Set(const Vector2f value)
{
	SET_SCALAR(Vector2f, VECTOR2);
}

void Variant::Set(const Vector3f value)
{
	SET_SCALAR(Vector3f, VECTOR3);
}

void Variant::Set(const Vector4f value)
{
	SET_SCALAR(Vector4f, VECTOR4);
}

void Variant::Set(const Colourf value)
{
	SET_SCALAR(Colourf, COLOURF);
}

void Variant::Set(const Colourb value)
{
	SET_SCALAR(Colourb, COLOURB);
}

void Variant::Set(ScriptInterface* value)
{
	using Pointer = ScriptInterface*;
	SET_SCALAR(Pointer, SCRIPTINTERFACE);
}

void Variant::Set(const String& value)
{
	if (type == STRING)
	{
		(*std::launder(reinterpret_cast<String*>(data))) = value;
	}
	else
	{
		new (data) String(value);
		type = STRING;
	}
}
void Variant::Set(String&& value)
{
	if (type == STRING)
	{
		(*std::launder(reinterpret_cast<String*>(data))) = std::move(value);
	}
	else
	{
		new (data) String(std::move(value));
		type = STRING;
	}
}

void Variant::Set(const TransformPtr& value)
{
	if (type == TRANSFORMPTR)
	{
		SET_VARIANT(TransformPtr);
	}
	else
	{
		new (data) TransformPtr(value);
		type = TRANSFORMPTR;
	}
}
void Variant::Set(TransformPtr&& value)
{
	if (type == TRANSFORMPTR)
	{
		(*std::launder(reinterpret_cast<TransformPtr*>(data))) = std::move(value);
	}
	else
	{
		new (data) TransformPtr(std::move(value));
		type = TRANSFORMPTR;
	}
}

void Variant::Set(const TransitionList& value)
{
	if (type == TRANSITIONLIST)
	{
		*std::launder(reinterpret_cast<TransitionList*>(data)) = value;
	}
	else
	{
		new (data) TransitionList(value);
		type = TRANSITIONLIST;
	}
}
void Variant::Set(TransitionList&& value)
{
	if (type == TRANSITIONLIST)
	{
		(*std::launder(reinterpret_cast<TransitionList*>(data))) = std::move(value);
	}
	else
	{
		new (data) TransitionList(std::move(value));
		type = TRANSITIONLIST;
	}
}

void Variant::Set(const AnimationList& value)
{
	if (type == ANIMATIONLIST)
	{
		*std::launder(reinterpret_cast<AnimationList*>(data)) = value;
	}
	else
	{
		new (data) AnimationList(value);
		type = ANIMATIONLIST;
	}
}
void Variant::Set(AnimationList&& value)
{
	if (type == ANIMATIONLIST)
	{
		(*std::launder(reinterpret_cast<AnimationList*>(data))) = std::move(value);
	}
	else
	{
		new (data) AnimationList(std::move(value));
		type = ANIMATIONLIST;
	}
}

void Variant::Set(const DecoratorsPtr& value)
{
	if (type == DECORATORSPTR)
	{
		*std::launder(reinterpret_cast<DecoratorsPtr*>(data)) = value;
	}
	else
	{
		new (data) DecoratorsPtr(value);
		type = DECORATORSPTR;
	}
}
void Variant::Set(DecoratorsPtr&& value)
{
	if (type == DECORATORSPTR)
	{
		(*std::launder(reinterpret_cast<DecoratorsPtr*>(data))) = std::move(value);
	}
	else
	{
		new (data) DecoratorsPtr(std::move(value));
		type = DECORATORSPTR;
	}
}
void Variant::Set(const FiltersPtr& value)
{
	if (type == FILTERSPTR)
	{
		*std::launder(reinterpret_cast<FiltersPtr*>(data)) = value;
	}
	else
	{
		new (data) FiltersPtr(value);
		type = FILTERSPTR;
	}
}
void Variant::Set(FiltersPtr&& value)
{
	if (type == FILTERSPTR)
	{
		(*std::launder(reinterpret_cast<FiltersPtr*>(data))) = std::move(value);
	}
	else
	{
		new (data) FiltersPtr(std::move(value));
		type = FILTERSPTR;
	}
}
void Variant::Set(const FontEffectsPtr& value)
{
	if (type == FONTEFFECTSPTR)
	{
		*std::launder(reinterpret_cast<FontEffectsPtr*>(data)) = value;
	}
	else
	{
		new (data) FontEffectsPtr(value);
		type = FONTEFFECTSPTR;
	}
}
void Variant::Set(FontEffectsPtr&& value)
{
	if (type == FONTEFFECTSPTR)
	{
		(*std::launder(reinterpret_cast<FontEffectsPtr*>(data))) = std::move(value);
	}
	else
	{
		new (data) FontEffectsPtr(std::move(value));
		type = FONTEFFECTSPTR;
	}
}
void Variant::Set(const ColorStopList& value)
{
	if (type == COLORSTOPLIST)
	{
		*std::launder(reinterpret_cast<ColorStopList*>(data)) = value;
	}
	else
	{
		new (data) ColorStopList(value);
		type = COLORSTOPLIST;
	}
}
void Variant::Set(ColorStopList&& value)
{
	if (type == COLORSTOPLIST)
	{
		(*std::launder(reinterpret_cast<ColorStopList*>(data))) = std::move(value);
	}
	else
	{
		new (data) ColorStopList(std::move(value));
		type = COLORSTOPLIST;
	}
}
void Variant::Set(const BoxShadowList& value)
{
	if (type == BOXSHADOWLIST)
	{
		*std::launder(reinterpret_cast<BoxShadowList*>(data)) = value;
	}
	else
	{
		new (data) BoxShadowList(value);
		type = BOXSHADOWLIST;
	}
}
void Variant::Set(BoxShadowList&& value)
{
	if (type == BOXSHADOWLIST)
	{
		(*std::launder(reinterpret_cast<BoxShadowList*>(data))) = std::move(value);
	}
	else
	{
		new (data) BoxShadowList(std::move(value));
		type = BOXSHADOWLIST;
	}
}

Variant& Variant::operator=(const Variant& copy)
{
	if (this == &copy)
		return *this;
	if (copy.type != type)
		Clear();
	Set(copy);
	return *this;
}

Variant& Variant::operator=(Variant&& other) noexcept
{
	if (this == &other)
		return *this;
	if (other.type != type)
		Clear();
	Set(std::move(other));
	return *this;
}

#define DEFAULT_VARIANT_COMPARE(TYPE) (GetReference<TYPE>() == other.GetReference<TYPE>())

bool Variant::operator==(const Variant& other) const
{
	if (type != other.type)
		return false;

	switch (type)
	{
	case BOOL: return DEFAULT_VARIANT_COMPARE(bool);
	case BYTE: return DEFAULT_VARIANT_COMPARE(byte);
	case CHAR: return DEFAULT_VARIANT_COMPARE(char);
	case FLOAT: return DEFAULT_VARIANT_COMPARE(float);
	case DOUBLE: return DEFAULT_VARIANT_COMPARE(double);
	case INT: return DEFAULT_VARIANT_COMPARE(int);
	case INT64: return DEFAULT_VARIANT_COMPARE(int64_t);
	case UINT: return DEFAULT_VARIANT_COMPARE(unsigned int);
	case UINT64: return DEFAULT_VARIANT_COMPARE(uint64_t);
	case STRING: return DEFAULT_VARIANT_COMPARE(String);
	case VECTOR2: return DEFAULT_VARIANT_COMPARE(Vector2f);
	case VECTOR3: return DEFAULT_VARIANT_COMPARE(Vector3f);
	case VECTOR4: return DEFAULT_VARIANT_COMPARE(Vector4f);
	case COLOURF: return DEFAULT_VARIANT_COMPARE(Colourf);
	case COLOURB: return DEFAULT_VARIANT_COMPARE(Colourb);
	case SCRIPTINTERFACE: return DEFAULT_VARIANT_COMPARE(ScriptInterface*);
	case VOIDPTR: return DEFAULT_VARIANT_COMPARE(void*);
	case TRANSFORMPTR: return DEFAULT_VARIANT_COMPARE(TransformPtr);
	case TRANSITIONLIST: return DEFAULT_VARIANT_COMPARE(TransitionList);
	case ANIMATIONLIST: return DEFAULT_VARIANT_COMPARE(AnimationList);
	case DECORATORSPTR: return DEFAULT_VARIANT_COMPARE(DecoratorsPtr);
	case FILTERSPTR: return DEFAULT_VARIANT_COMPARE(FiltersPtr);
	case FONTEFFECTSPTR: return DEFAULT_VARIANT_COMPARE(FontEffectsPtr);
	case COLORSTOPLIST: return DEFAULT_VARIANT_COMPARE(ColorStopList);
	case BOXSHADOWLIST: return DEFAULT_VARIANT_COMPARE(BoxShadowList);
	case NONE: return true;
	}
	RMLUI_ERRORMSG("Variant comparison not implemented for this type.");
	return false;
}

} // namespace Rml
