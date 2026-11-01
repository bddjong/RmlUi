#include "../../Include/RmlUi/Core/Traits.h"
#include <atomic>

namespace Rml {

int FamilyBase::GetNewId()
{
	static std::atomic<int> id{0};
	return id.fetch_add(1, std::memory_order_relaxed);
}

} // namespace Rml
