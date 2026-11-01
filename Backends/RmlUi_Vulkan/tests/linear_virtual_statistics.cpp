// Standalone regression: compile with Vulkan headers and run without a GPU.
#ifdef NDEBUG
#error This regression test requires assertions enabled.
#endif
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0
#define VMA_IMPLEMENTATION
#include "../vk_mem_alloc.h"
#include <cassert>
#include <cstring>
#include <string>

int main()
{
    VmaStringBuilder builder(nullptr);
    {
        VmaJsonWriter writer(nullptr, builder);
        writer.WriteString("\xC3\xA9\x01");
    }
    assert(std::string(builder.GetData(), builder.GetLength()) == "\"\xC3\xA9\\u0001\"");

    VmaVirtualBlockCreateInfo blockInfo = {};
    blockInfo.size = 96;
    blockInfo.flags = VMA_VIRTUAL_BLOCK_CREATE_LINEAR_ALGORITHM_BIT;
    VmaVirtualBlock block;
    assert(vmaCreateVirtualBlock(&blockInfo, &block) == VK_SUCCESS);
    VmaVirtualAllocationCreateInfo info = {};
    info.size = 32; // pUserData is deliberately null.
    VmaVirtualAllocation a, b, c, wrapped;
    assert(vmaVirtualAllocate(block, &info, &a, nullptr) == VK_SUCCESS);
    assert(vmaVirtualAllocate(block, &info, &b, nullptr) == VK_SUCCESS);
    assert(vmaVirtualAllocate(block, &info, &c, nullptr) == VK_SUCCESS);
    vmaVirtualFree(block, a);
    info.size = 16;
    assert(vmaVirtualAllocate(block, &info, &wrapped, nullptr) == VK_SUCCESS);
    VmaStatistics stats = {};
    VmaDetailedStatistics detailed = {};
    vmaGetVirtualBlockStatistics(block, &stats);
    vmaCalculateVirtualBlockStatistics(block, &detailed);
    assert(stats.allocationCount == 3 && stats.allocationBytes == 80);
    assert(detailed.statistics.allocationCount == 3 && detailed.statistics.allocationBytes == 80);
    char* json = nullptr;
    vmaBuildVirtualBlockStatsString(block, &json, VK_TRUE);
    assert(json && std::strstr(json, "\"Allocations\": 3"));
    vmaFreeVirtualBlockStatsString(block, json);
    vmaVirtualFree(block, b);
    vmaVirtualFree(block, c);
    vmaVirtualFree(block, wrapped);
    vmaDestroyVirtualBlock(block);
}
