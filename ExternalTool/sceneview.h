#ifndef SCENEVIEW_H
#define SCENEVIEW_H

#include <QVulkanWindow>
#include <QVulkanFunctions>
#include <QVulkanInstance>
#include <cstdint>
#include <QFile>
#include <ctime>
#include <QImage>
#include "datacontext.h"
#include "model.h"
#include "helperMacro.h"

#define MAX_HILL_VSIZE (800 * 800 * 4)
#define MAX_PLANT_VSIZE (4096 * 4)
#define MAX_HILL_I_SIZE (800 * 800 * 5 + 1)
#define MAX_PLANT_I_SIZE (4096 * 5 + 1)
#define VDFT nullptr
#define TODO nullptr
#define PLANT_TYPE_NUM 8
#define AREA_TYPE_NUM 8
#define MAX_IMAGE_SIZE (1024 * 1024)
#define TEXEL_SIZE (sizeof(uint8_t) * 4)
#define MAX_TMP_BUFFER_SIZE ((800 * 800 * 5 + 1) * VertexSize)
#define MAX_TMP_IMAGE_SIZE (1024 * 1024 * sizeof(uint8_t) * 4)

class Render: public QVulkanWindowRenderer
{
public:
    Render(QVulkanWindow *w);

    void initResources() override;
    // void initSwapChainResources() override;
    // void releaseSwapChainResources() override;
    void releaseResources() override;
    void logicalDeviceLost() override;
    void physicalDeviceLost() override;

    void startNextFrame() override;

    void pushHill(const Vertexs& vs, const Indexs& index);

    void setPlant(const Vertexs& vs, const Indexs& index, uint32_t id);

private:
    void createBuffer(uint32_t size, void*& data, VkBuffer& buffer, VkBufferUsageFlags usage,
                      VkDeviceMemory& memory, VkMemoryRequirements& memoryRequirement, bool local);
    void createImage(VkExtent2D extent, VkFormat format, void*& data, VkImage& image, VkImageUsageFlags usage,
                     VkDeviceMemory& memory, VkMemoryRequirements& memoryRequirement, bool local);
    void fillBuffer(uint32_t size, const void* data, VkBuffer buffer, uint32_t offset);
    void fillBufferInit();
    void fillImage(uint32_t size, VkExtent2D extent, const void* data, VkImage image, VkOffset2D offset);
    void fillImageInit();
    void buildPipeline(uint32_t width, uint32_t height);
    void destroyPipeline();
    void loadShader();
    void createBasicRenderPass();
    void createSampler();
    void createImageView(VkImage image, VkImageAspectFlags aspect, VkFormat format, VkImageView& view);

private:
    QVulkanWindow* window;
    QVulkanDeviceFunctions* devFuncs;
    QVulkanFunctions* functions;
    float green = 0;

    VkBuffer hillVertexBuffer;
    uint32_t hillVsCount = 0;
    VkBuffer plantsVertexBuffers[PLANT_TYPE_NUM];
    uint32_t plantsVsCount[PLANT_TYPE_NUM] = { 0 };
    // Vertexs buffers and sizes;

    VkBuffer hillIndexBuffer;
    uint32_t hillIndexCount;
    VkBuffer plantsIndexBuffers[PLANT_TYPE_NUM];
    uint32_t plantIndexCounts[PLANT_TYPE_NUM];

    VkDeviceMemory hillMemory;
    VkMemoryRequirements hillMemoryRequirement;
    void* hillVData;
    VkDeviceMemory plantsMemories[PLANT_TYPE_NUM];
    VkMemoryRequirements plantsMemoriesRequirements[PLANT_TYPE_NUM];
    void* plantsVData[PLANT_TYPE_NUM];
    // Device memories and memory mappings;

    VkDeviceMemory hillIndexMemory;
    VkMemoryRequirements hillIndexMemoryRequirement;
    void* hillIndexData;
    VkDeviceMemory plantsIndexMemories[PLANT_TYPE_NUM];
    VkMemoryRequirements plantsIndexMemoryRequirements[PLANT_TYPE_NUM];
    void* plantsIndexDatas[PLANT_TYPE_NUM];

    VkPipeline sceneViewPipeline = VK_NULL_HANDLE;
    VkPipelineLayout sceneViewPipelineLayout = VK_NULL_HANDLE;
    // Graphics pipelines;

    VkRenderPass basicPass;

    VkShaderModule standardVertexShader;
    VkShaderModule standardFragmentShader;
    // Shaders;

    Vertexs monkey;
    Indexs monkeyIndex;

    VkBuffer uniform;
    VkDeviceMemory uniformMemory;
    VkMemoryRequirements unigormMemoryRequirement;
    void* uniformData;
    VkDescriptorSetLayout uniformSetLayout;
    VkDescriptorPool uniformPool;
    VkDescriptorSet* uniformSet = nullptr;
    uint32_t uniformSize = 0;

    VkDescriptorPool texturePool;
    VkDescriptorSet* texturesSet = nullptr;
    uint32_t texturesSize;

    VkImage hillTextures[AREA_TYPE_NUM];
    VkImage hillType0;
    VkImage hillType1;
    VkImage plantType0;
    VkImage plantType1;
    VkImage plantsTextures[PLANT_TYPE_NUM];
    VkImage testImage;
    VkImageView testImageView;
    VkSampler standSampler;
    VkDeviceMemory hillTMemory[AREA_TYPE_NUM];
    VkDeviceMemory hillType0Memory;
    VkDeviceMemory hillType1Memory;
    VkDeviceMemory plantType0Memory;
    VkDeviceMemory plantType1Memory;
    VkDeviceMemory plantsTexturesMemory[PLANT_TYPE_NUM];
    VkDeviceMemory testIMemory;
    VkMemoryRequirements hillTMemoryRequirements[AREA_TYPE_NUM];
    VkMemoryRequirements hillType0MemoryRequirement;
    VkMemoryRequirements hillType1MemoryRequirement;
    VkMemoryRequirements plantType0MemoryRequirement;
    VkMemoryRequirements plantType1MemoryRequirement;
    VkMemoryRequirements plantsTexturesMemoryRequirements[PLANT_TYPE_NUM];
    VkMemoryRequirements testIMemoryRequirement;
    void* hillTDatas[AREA_TYPE_NUM];
    void* hillType0Data;
    void* hillType1Data;
    void* plantType0Data;
    void* plantType1Data;
    void* plantsTexturesDatas[PLANT_TYPE_NUM];
    void* testImageData;

    VkBuffer tmpBuffer;
    VkDeviceMemory tmpBufferMemory;
    VkMemoryRequirements tmpBufferRequirement;
    VkBuffer tmpImage;
    VkDeviceMemory tmpImageMemory;
    VkMemoryRequirements tmpImageRequirement;
};

struct UniformTime {
    float time;
    alignas(16) glm::vec4 resolution;
};

class VulkanWindow : public QVulkanWindow
{
public:
    QVulkanWindowRenderer *createRenderer() override;
};

class SceneView : public QVulkanWindow
{
    Q_OBJECT
public:
    explicit SceneView(QVulkanWindow *parent = nullptr, const DataContext* dataContext = nullptr,
                       QVulkanInstance* instance = nullptr, const bool topView = false);
    QVulkanWindowRenderer *createRenderer() override;

signals:

private:
    const DataContext* dataContext;
    QVulkanInstance* instance;
    const bool topView;
};

#endif // SCENEVIEW_H
