#include <RmlUi/Core/Variant.h>
#include <RmlUi/Core/DecorationTypes.h>
#include <cassert>
#include <cstdlib>
#include <new>
#include <memory>

// Single-threaded, isolated executable: count live allocations and fail one
// allocation at a time, including allocations inside nested list elements.
static long fail_after = -1;
static long live = 0;
void* operator new(std::size_t size)
{
	if (fail_after == 0) throw std::bad_alloc();
	if (fail_after > 0) --fail_after;
	if (void* p = std::malloc(size ? size : 1)) { ++live; return p; }
	throw std::bad_alloc();
}
void operator delete(void* p) noexcept { if (p) { --live; std::free(p); } }
void* operator new[](std::size_t n) { return ::operator new(n); }
void operator delete[](void* p) noexcept { ::operator delete(p); }
void operator delete(void* p, std::size_t) noexcept { ::operator delete(p); }
void operator delete[](void* p, std::size_t) noexcept { ::operator delete(p); }

using namespace Rml;

template <typename T> void exercise(const T& input)
{
	Variant source(input);
	const auto tag = source.GetType();
	for (int operation = 0; operation != 4; ++operation)
	{
		bool failed = false, completed = false;
		for (long point = 0; point != 100 && !completed; ++point)
		{
			const long baseline = live;
			{
				Variant destination = operation == 0 ? Variant(T{}) : (operation == 1 ? Variant(42) : Variant(String(200, 'o')));
				// Include destination in the baseline so constructor failures can be checked too.
				const long before = live;
				fail_after = point;
				try {
					if (operation < 2) destination = source;
					else if (operation == 2) destination = input;
					else { Variant copy(source); assert(copy.GetReference<T>() == input); }
					fail_after = -1;
					completed = true;
					if (operation != 3) assert(destination.GetReference<T>() == input);
				}
				catch (const std::bad_alloc&) {
					fail_after = -1;
					failed = true;
					assert(source.GetReference<T>() == input);
					if (operation == 0) assert(destination.GetType() == tag);
					if (operation == 1) assert(destination.GetType() == Variant::NONE);
					if (operation >= 2) assert(destination.GetReference<String>() == String(200, 'o'));
					if (operation == 3) assert(live == before);
					// A failed object remains usable and can be overwritten or destroyed.
					destination = source;
					assert(destination.GetReference<T>() == input);
				}
			}
			assert(live == baseline);
		}
		assert(failed && completed);
	}
	// Direct value construction has a separate entry point from copy construction.
	bool failed = false, completed = false;
	for (long point = 0; point != 100 && !completed; ++point) {
		long baseline = live;
		fail_after = point;
		try { Variant copy(input); fail_after = -1; assert(copy.GetReference<T>() == input); completed = true; }
		catch (const std::bad_alloc&) { fail_after = -1; failed = true; }
		assert(live == baseline);
	}
	assert(failed && completed);
}

template <typename P> void check_ownership()
{
	auto owner = std::make_shared<int>(7);
	std::weak_ptr<int> weak = owner;
	P pointer(owner, nullptr);
	Variant value(pointer);
	pointer.reset();
	owner.reset();
	assert(!weak.expired());
	Variant copy(value);
	Variant moved(std::move(copy));
	value.Clear();
	assert(!weak.expired());
	Variant source(String(256, 'x'));
	fail_after = 0;
	try { moved = source; assert(false); }
	catch (const std::bad_alloc&) { fail_after = -1; }
	assert(moved.GetType() == Variant::NONE);
	assert(weak.expired());
	moved = source;
	assert(moved.GetReference<String>() == source.GetReference<String>());
}

int main()
{
	const long initial_live = live;
	{
		exercise(String(256, 'x'));
		// The C-string constructor allocates before its String is installed.
		String text(256, 'c');
		long baseline = live;
		fail_after = 0;
		try { Variant value(text.c_str()); assert(false); }
		catch (const std::bad_alloc&) { fail_after = -1; }
		assert(live == baseline);
		Variant value(text.c_str());
		assert(value.GetReference<String>() == text);
		TransitionList transitions; transitions.transitions.resize(8); exercise(transitions);
		AnimationList animations(4); for (auto& a : animations) a.name = String(256, 'a'); exercise(animations);
		// Reuse vector capacity so failure occurs while assigning an element,
		// after earlier elements may already have changed.
		AnimationList small(4);
		Variant same(small), source(animations);
		long before = live;
		fail_after = 1;
		try { same = source; assert(false); }
		catch (const std::bad_alloc&) { fail_after = -1; }
		assert(same.GetType() == Variant::ANIMATIONLIST);
		assert(source.GetReference<AnimationList>() == animations);
		same.Clear();
		assert(live <= before);
		same = source;
		assert(same.GetReference<AnimationList>() == animations);
		exercise(ColorStopList(8));
		exercise(BoxShadowList(8));
		check_ownership<TransformPtr>();
		check_ownership<DecoratorsPtr>();
		check_ownership<FiltersPtr>();
		check_ownership<FontEffectsPtr>();
	}
	assert(live == initial_live);
}
