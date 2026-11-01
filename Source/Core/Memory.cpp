#include "Memory.h"
#include <cstring>
#include <limits>
#include <memory>
#include <new>
#include <stdint.h>
#include <stdlib.h>

namespace Rml {

namespace Detail {

	inline void* rmlui_align(size_t alignment, size_t size, void*& ptr, size_t& space)
	{
#if defined(_MSC_VER)
		return std::align(alignment, size, ptr, space);
#else
		// std::align replacement to support compilers missing this feature.
		// From https://gcc.gnu.org/bugzilla/show_bug.cgi?id=57350

		uintptr_t pn = reinterpret_cast<uintptr_t>(ptr);
		uintptr_t aligned = (pn + alignment - 1) & -alignment;
		size_t padding = aligned - pn;
		if (space < size + padding)
			return nullptr;
		space -= padding;
		return ptr = reinterpret_cast<void*>(aligned);
#endif
	}

	BasicStackAllocator::BasicStackAllocator(size_t N) : N(N), data((byte*)malloc(N ? N : 1)), p(data)
	{
		if (!data)
			AllocationFailure();
	}

	BasicStackAllocator::~BasicStackAllocator() noexcept
	{
		RMLUI_ASSERT(p == data);
		free(data);
	}

	void* BasicStackAllocator::allocate(size_t alignment, size_t byte_size)
	{
		// Each allocation stores its previous stack position or heap base just before
		// the aligned result. This also restores padding on stack deallocation.
		const size_t header_size = sizeof(void*);
		byte_size = byte_size ? byte_size : 1;
		if (!alignment || (alignment & (alignment - 1)) ||
			alignment - 1 > std::numeric_limits<size_t>::max() - header_size ||
			byte_size > std::numeric_limits<size_t>::max() - header_size - (alignment - 1))
			AllocationFailure();
		size_t available_space = N - ((byte*)p - data);

		if (available_space >= header_size)
		{
			void* result = (byte*)p + header_size;
			available_space -= header_size;
			if (rmlui_align(alignment, byte_size, result, available_space))
			{
				std::memcpy((byte*)result - header_size, &p, header_size);
				p = (byte*)result + byte_size;
				return result;
			}
		}

		size_t size = header_size + (alignment - 1) + byte_size;
		void* base = malloc(size);
		if (!base)
			AllocationFailure();
		void* result = (byte*)base + header_size;
		size_t space = size - header_size;
		rmlui_align(alignment, byte_size, result, space);
		std::memcpy((byte*)result - header_size, &base, header_size);
		return result;
	}

	void BasicStackAllocator::deallocate(void* obj) noexcept
	{
		void* previous;
		std::memcpy(&previous, (byte*)obj - sizeof(void*), sizeof(void*));
		const uintptr_t address = reinterpret_cast<uintptr_t>(obj);
		const uintptr_t begin = reinterpret_cast<uintptr_t>(data);
		if (address < begin || address - begin >= N)
		{
			free(previous);
			return;
		}
		p = previous;
	}

	BasicStackAllocator& GetGlobalBasicStackAllocator()
	{
		static BasicStackAllocator stack_allocator(10 * 1024);
		return stack_allocator;
	}

} // namespace Detail

} // namespace Rml
