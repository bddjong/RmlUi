// Standalone: compile with ../Memory.cpp and run under address/undefined sanitizers.
#include "../Memory.h"
#include <cassert>
#include <cstdint>
#include <limits>
#include <new>

namespace Rml {
bool Assert(const char*, const char*, int) { assert(false); return false; }
}

int main(int argc, char**)
{
	Rml::Detail::BasicStackAllocator stack(512);
	// In an exception-free build this mode must terminate on overflow.
	if (argc > 1)
		stack.allocate(128, std::numeric_limits<size_t>::max());
	void* first = stack.allocate(1, 1);
	void* aligned = stack.allocate(128, 32);
	assert(reinterpret_cast<uintptr_t>(aligned) % 128 == 0);
	stack.deallocate(aligned);
	void* reused = stack.allocate(128, 32);
	assert(reused == aligned);
	void* fallback = stack.allocate(256, 1024);
	assert(reinterpret_cast<uintptr_t>(fallback) % 256 == 0);
	stack.deallocate(fallback);
	stack.deallocate(reused);
	stack.deallocate(first);

	Rml::Detail::BasicStackAllocator empty(0);
	void* zero = empty.allocate(64, 0);
	assert(reinterpret_cast<uintptr_t>(zero) % 64 == 0);
	empty.deallocate(zero);

#if defined(__cpp_exceptions) || defined(__EXCEPTIONS) || defined(_CPPUNWIND)
	bool threw = false;
	try { stack.allocate(128, std::numeric_limits<size_t>::max()); }
	catch (const std::bad_alloc&) { threw = true; }
	assert(threw);
	threw = false;
	try { Rml::GlobalStackAllocator<uint64_t>().allocate(std::numeric_limits<size_t>::max()); }
	catch (const std::bad_alloc&) { threw = true; }
	assert(threw);
#endif
}
