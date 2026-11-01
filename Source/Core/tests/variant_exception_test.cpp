// Standalone exception regression. Build with exceptions enabled; do not link into the unit test runner.
#include <RmlUi/Core/Variant.h>
#include <RmlUi/Core/DecorationTypes.h>
#include <cassert>
#include <cstdlib>
#include <new>

namespace Rml { bool Assert(const char*, const char*, int) { std::abort(); } }

static bool fail_allocation = false;
void* operator new(std::size_t size)
{
	if (fail_allocation)
		throw std::bad_alloc();
	if (void* p = std::malloc(size ? size : 1))
		return p;
	throw std::bad_alloc();
}
void operator delete(void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }

template <typename T>
void CheckLifetime(const T& input)
{
	Rml::Variant value(input);
	assert(reinterpret_cast<uintptr_t>(&value.GetReference<T>()) % alignof(T) == 0);
	Rml::Variant copy(value);
	Rml::Variant moved(std::move(copy));
	copy = value;
	copy = 42;
	copy = std::move(moved);
	copy.Clear();
	assert(copy.GetType() == Rml::Variant::NONE);
}

int main()
{
	using namespace Rml;
	CheckLifetime(bool(true)); CheckLifetime(byte(1)); CheckLifetime(char('x'));
	CheckLifetime(float(1)); CheckLifetime(double(2)); CheckLifetime(int(3));
	CheckLifetime(int64_t(4)); CheckLifetime(unsigned(5)); CheckLifetime(uint64_t(6));
	CheckLifetime(String(128, 'x'));
	CheckLifetime(Vector2f(1, 2)); CheckLifetime(Vector3f(1, 2, 3)); CheckLifetime(Vector4f(1, 2, 3, 4));
	CheckLifetime(Colourf(1, 0, 1, 1)); CheckLifetime(Colourb(1, 2, 3, 4));
	CheckLifetime(static_cast<ScriptInterface*>(nullptr)); CheckLifetime(static_cast<void*>(nullptr));
	CheckLifetime(TransformPtr{}); CheckLifetime(TransitionList{}); CheckLifetime(AnimationList{});
	CheckLifetime(DecoratorsPtr{}); CheckLifetime(FiltersPtr{}); CheckLifetime(FontEffectsPtr{});
	CheckLifetime(ColorStopList{}); CheckLifetime(BoxShadowList{});
	Variant source(String(4096, 's'));
	Variant different(42);
	Variant same(String(32, 't'));
	fail_allocation = true;
	try { different = source; assert(false); } catch (const std::bad_alloc&) {}
	assert(different.GetType() == Variant::NONE);
	try { same = source; assert(false); } catch (const std::bad_alloc&) {}
	assert(same.GetType() == Variant::STRING);
	try { Variant copy(source); assert(false); } catch (const std::bad_alloc&) {}
	fail_allocation = false;
	different = source;
	same = source;
	assert(different.GetReference<String>() == source.GetReference<String>());
	assert(same.GetReference<String>() == source.GetReference<String>());
}
