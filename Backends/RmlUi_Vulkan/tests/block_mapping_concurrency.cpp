// Standalone mock Vulkan regression. No GPU required.
#ifdef NDEBUG
#error This regression requires assertions enabled.
#endif
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0
#define VMA_IMPLEMENTATION
#include "../vk_mem_alloc.h"
#include <atomic>
#include <cassert>
#include <thread>

static std::atomic<int> failures{0}, maps{0}, unmaps{0}, active{0};
static char memory[64];
static VKAPI_ATTR void VKAPI_CALL unused() {}
static VKAPI_ATTR void VKAPI_CALL properties(VkPhysicalDevice, VkPhysicalDeviceProperties* p) {
    *p = {}; p->limits.bufferImageGranularity = 1; p->limits.nonCoherentAtomSize = 1;
}
static VKAPI_ATTR void VKAPI_CALL memoryProperties(VkPhysicalDevice, VkPhysicalDeviceMemoryProperties* p) { *p = {}; p->memoryTypeCount = 1; p->memoryTypes[0].propertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT; p->memoryHeapCount = 1; p->memoryHeaps[0].size = 1024 * 1024; }
static VKAPI_ATTR VkResult VKAPI_CALL allocateMemory(VkDevice, const VkMemoryAllocateInfo*, const VkAllocationCallbacks*, VkDeviceMemory* p) { *p = (VkDeviceMemory)1; return VK_SUCCESS; }
static VKAPI_ATTR void VKAPI_CALL freeMemory(VkDevice, VkDeviceMemory, const VkAllocationCallbacks*) {}
static VKAPI_ATTR VkResult VKAPI_CALL mapMemory(VkDevice, VkDeviceMemory, VkDeviceSize, VkDeviceSize, VkMemoryMapFlags, void** p) {
    if (failures.load() > 0) { --failures; *p = reinterpret_cast<void*>(1); return VK_ERROR_MEMORY_MAP_FAILED; }
    assert(active.fetch_add(1) == 0); ++maps; *p = memory; return VK_SUCCESS;
}
static VKAPI_ATTR void VKAPI_CALL unmapMemory(VkDevice, VkDeviceMemory) {
    assert(active.fetch_sub(1) == 1); ++unmaps;
}

int main() {
    VmaVulkanFunctions f = {};
#define STUB(name) f.name = reinterpret_cast<decltype(f.name)>(unused)
    STUB(vkFlushMappedMemoryRanges);
    STUB(vkInvalidateMappedMemoryRanges); STUB(vkBindBufferMemory); STUB(vkBindImageMemory);
    STUB(vkGetBufferMemoryRequirements); STUB(vkGetImageMemoryRequirements);
    STUB(vkCreateBuffer); STUB(vkDestroyBuffer); STUB(vkCreateImage); STUB(vkDestroyImage); STUB(vkCmdCopyBuffer);
#undef STUB
    f.vkGetPhysicalDeviceProperties = properties; f.vkGetPhysicalDeviceMemoryProperties = memoryProperties;
    f.vkAllocateMemory = allocateMemory; f.vkFreeMemory = freeMemory;
    f.vkMapMemory = mapMemory; f.vkUnmapMemory = unmapMemory;
    VmaAllocatorCreateInfo info = {};
    info.instance = reinterpret_cast<VkInstance>(1); info.physicalDevice = reinterpret_cast<VkPhysicalDevice>(1);
    info.device = reinterpret_cast<VkDevice>(1); info.pVulkanFunctions = &f;
    VmaAllocator_T allocator(&info);
    VmaDeviceMemoryBlock block(&allocator);
    failures = 20;
    for (int i = 0; i < 20; ++i) {
        void* p = nullptr;
        assert(block.Map(&allocator, 1, &p) == VK_ERROR_MEMORY_MAP_FAILED);
        assert(p == nullptr && block.GetMappedData() == nullptr && block.GetMapRefCount() == 0);
    }
    void* pinned = nullptr;
    assert(block.Map(&allocator, 1, &pinned) == VK_SUCCESS && pinned == memory);
    auto mapping = [&] { for (int i = 0; i < 20000; ++i) {
        void* p = nullptr; assert(block.Map(&allocator, 1, &p) == VK_SUCCESS && p == memory);
        block.Unmap(&allocator, 1);
    }};
    auto allocations = [&] { for (int i = 0; i < 20000; ++i) {
        block.PostAlloc(); block.PostFree(&allocator);
        assert(block.GetMappedData() == memory); (void)block.GetMapRefCount();
    }};
    std::thread a(mapping), b(mapping), c(allocations), d(allocations);
    a.join(); b.join(); c.join(); d.join();
    block.Unmap(&allocator, 1);
    auto churn = [&] { for (int i = 0; i < 20000; ++i) {
        block.PostAlloc(); block.PostFree(&allocator); (void)block.GetMappedData();
    }};
    std::thread e(mapping), f2(mapping), g(churn);
    e.join(); f2.join(); g.join();
    for (int i = 0; i < 100; ++i) block.PostFree(&allocator);
    assert(block.GetMappedData() == nullptr && active == 0 && maps == unmaps);

    // Exercise failure during allocation commit through the public API. The
    // failed request must leave the existing block usable and accounting intact.
    VmaAllocator publicAllocator = nullptr;
    assert(vmaCreateAllocator(&info, &publicAllocator) == VK_SUCCESS);
    VmaPoolCreateInfo poolInfo = {};
    poolInfo.memoryTypeIndex = 0; poolInfo.blockSize = 64; poolInfo.maxBlockCount = 1;
    VmaPool pool = nullptr;
    assert(vmaCreatePool(publicAllocator, &poolInfo, &pool) == VK_SUCCESS);
    VkMemoryRequirements req = {}; req.size = 16; req.alignment = 1; req.memoryTypeBits = 1;
    VmaAllocationCreateInfo allocInfo = {}; allocInfo.pool = pool;
    allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
    VmaAllocation first = nullptr;
    assert(vmaAllocateMemory(publicAllocator, &req, &allocInfo, &first, nullptr) == VK_SUCCESS);
    void* publicMapped = nullptr;
    failures = 1;
    assert(vmaMapMemory(publicAllocator, first, &publicMapped) == VK_ERROR_MEMORY_MAP_FAILED);
    assert(publicMapped == nullptr);
    assert(vmaMapMemory(publicAllocator, first, &publicMapped) == VK_SUCCESS && publicMapped == memory);
    vmaUnmapMemory(publicAllocator, first);
    VmaStatistics before = {}; vmaGetPoolStatistics(publicAllocator, pool, &before);
    allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT | VMA_ALLOCATION_CREATE_NEVER_ALLOCATE_BIT;
    failures = 10;
    VmaAllocation second = nullptr;
    assert(vmaAllocateMemory(publicAllocator, &req, &allocInfo, &second, nullptr) != VK_SUCCESS);
    VmaStatistics after = {}; vmaGetPoolStatistics(publicAllocator, pool, &after);
    assert(after.allocationCount == before.allocationCount && after.allocationBytes == before.allocationBytes);
    failures = 0;
    assert(vmaAllocateMemory(publicAllocator, &req, &allocInfo, &second, nullptr) == VK_SUCCESS);
    vmaFreeMemory(publicAllocator, second);
    vmaFreeMemory(publicAllocator, first);
    vmaDestroyPool(publicAllocator, pool);
    vmaDestroyAllocator(publicAllocator);
    assert(active == 0 && maps == unmaps);
}
