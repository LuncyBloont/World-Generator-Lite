#ifndef SCENEVIEW_H
#define SCENEVIEW_H

#include <QVulkanWindow>
#include <QVulkanFunctions>
#include <QVulkanInstance>
#include <cstdint>
#include <QFile>
#include <ctime>
#include <QImage>
#include <glm/glm/gtc/random.hpp>
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
#define MAX_TMP_IMAGE_SIZE (1024 * 1024 * sizeof(uint8_t) * 4 * 2)
#define MAX_PLANT_COUNT (1024 * 1024)

struct SimplePBR {
    float roughness;    // 粗糙度
    float specular;     // 高光强度
    float metallic;     // 金属度
    float contrast;     // 反射率对比度
};

struct RenderDescription {
    SimplePBR tallArborMaterial;        // 高大乔木材质
    SimplePBR arborMaterial;            // 乔木材质
    SimplePBR largeShrubMaterial;       // 大型灌木材质
    SimplePBR shrubMaterial;            // 灌木材质
    SimplePBR flowerMaterial;           // 开花植物材质
    SimplePBR grassMaterial;            // 草材质
    SimplePBR creepingMaterial;         // 匍匐植物材质
    SimplePBR stoneMaterial;            // 石头材质
    float fog;                          // 雾浓度
    float fogAttenuation;               // 雾衰减
    glm::vec3 sunColor;                 // 日光颜色
    float sunForce;                     // 日光强度
    float skyForce;                     // 天光强度
};

class Render: public QVulkanWindowRenderer {
    // 基于Vulkan的渲染器

public:
    // 对接窗体系统的重载函数
    Render(QVulkanWindow *w, const bool topView);

    void initResources() override;
    void initSwapChainResources() override;
    void releaseSwapChainResources() override;
    void releaseResources() override;
    void logicalDeviceLost() override;
    void physicalDeviceLost() override;

    void startNextFrame() override;

public:
    // 面向WGL的接口

    void pushHill(const Vertexs& vs, const Indexs& index);                  // 设置地形数据

    void setPlant(const Vertexs& vs, const Indexs& index, uint32_t id);     // 设置指定id的植被数据

    void setHillTexture(const QImage& image);                               // 设置地形纹理

    void setPlantTexture(const QImage& image, uint32_t id);                 // 设置指定植被的纹理

    void setShaderDescription(const RenderDescription* dsecription);        // 设置渲染相关配置

private:
    void createBuffer(uint32_t size, void*& data, VkBuffer& buffer, VkBufferUsageFlags usage,
                      VkDeviceMemory& memory, VkMemoryRequirements& memoryRequirement, bool local);
    void createImage(VkExtent2D extent, uint32_t mipsLevel, VkFormat format, void*& data, VkImage& image, VkImageUsageFlags usage,
                     VkDeviceMemory& memory, VkMemoryRequirements& memoryRequirement, bool local);
    void fillBuffer(uint32_t size, const void* data, VkBuffer buffer, uint32_t offset);
    void fillBufferInit();
    void fillImage(uint32_t size, VkExtent2D extent, uint32_t mipsLevel, const void* data, VkImage image, VkOffset2D offset);
    void fillImageInit();
    void buildPipeline(uint32_t width, uint32_t height);
    void destroyPipeline();
    void loadShader();
    void createBasicRenderPass();
    void createSampler();
    void createImageView(VkImage image, VkImageAspectFlags aspect, VkFormat format, uint32_t mipsLevel, VkImageView& view);
    void genMipmaps(VkImage image, VkExtent2D size, uint32_t mipsLevel);

private:
    const bool topView;
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

    VkBuffer testInstanceBuffer;
    VkDeviceMemory testInstanceMemory;
    VkMemoryRequirements testInstanceMemoryRequirement;
    void* testInstanceData;
    uint32_t testInstanceCount;

};

struct Uniform {
    alignas(4) float time;
    alignas(4) float skyForce;
    alignas(8) glm::vec2 fog;
    alignas(16) glm::vec4 resolution;
    alignas(16) glm::vec4 sun;
    alignas(16) glm::mat4 mvp;
    alignas(16) glm::vec4 bprRSMC;
};

class VulkanWindow : public QVulkanWindow {
    // 渲染视图组件

public:
    QVulkanWindowRenderer *createRenderer() override;
};

class SceneView : public QVulkanWindow {
    Q_OBJECT
public:
    // 面向Qt的UI接口

    explicit SceneView(QVulkanWindow *parent = nullptr, const DataContext* dataContext = nullptr,
                       QVulkanInstance* instance = nullptr, const bool topView = false);

    QVulkanWindowRenderer *createRenderer() override;

public:
    // 面向WGL的接口

    const Render* render() const; // 获取渲染器

signals:

private:
    const DataContext* dataContext;
    QVulkanInstance* instance;
    const bool topView;
};

#endif // SCENEVIEW_H
