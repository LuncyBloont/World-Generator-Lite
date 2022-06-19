#ifndef RENDERSTRUCT_H
#define RENDERSTRUCT_H
#include <vulkan/vulkan.h>
#include <cstdint>

struct Texture2D {
    VkImage image = VK_NULL_HANDLE;
    VkImageView view = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkMemoryRequirements requirement{};
    void* data;
};

struct IBuffer {
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkMemoryRequirements requirement{};
    void* data;
    uint32_t count = 0; // optional
};

#endif // RENDERSTRUCT_H
