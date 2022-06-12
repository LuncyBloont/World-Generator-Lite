#ifndef SCENEVIEW_H
#define SCENEVIEW_H

#include <QVulkanWindow>
#include <QVulkanFunctions>
#include <QVulkanInstance>
#include <cstdint>
#include <QFile>
#include <ctime>
#include <QImage>
#include <QThread>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QCursor>
#include <glm/glm/gtc/random.hpp>
#include "datacontext.h"
#include "model.h"
#include "helperMacro.h"
#include "camera.h"

#define MAX_HILL_VSIZE (800 * 800 * 4)
#define MAX_PLANT_VSIZE (4096 * 4)
#define MAX_HILL_I_SIZE (800 * 800 * 5 + 1)
#define MAX_PLANT_I_SIZE (4096 * 5 + 1)
#define VDFT nullptr
#define TODO nullptr
#define PLANT_TYPE_NUM 8
#define AREA_TYPE_NUM 8
#define MAX_IMAGE_SIZE (1024 * 1024)
#define MAX_IMAGE_WIDTH 1024
#define MAX_IMAGE_HEIGHT 1024
#define TEXEL_SIZE (sizeof(uint8_t) * 4)
#define MAX_TMP_BUFFER_SIZE ((800 * 800 * 5 + 1) * VertexSize)
#define MAX_TMP_IMAGE_SIZE (1024 * 1024 * sizeof(uint8_t) * 4 * 2)
#define MAX_PLANT_COUNT (1024 * 1024)

struct SimplePBR {
    float roughness = 0.1;    // 粗糙度
    float specular = 0.5;     // 高光强度
    float metallic = 0.05;     // 金属度
    float contrast = 0.5;     // 反射率对比度
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
    glm::vec3 sunDir;                   // 日光方向
};

class Render: public QVulkanWindowRenderer {
    // 基于Vulkan的渲染器

private:
    void textureInit();
    void vertexInit();
    void uniformInit();
    void instanceInit();

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

    void setPlantTransform(const std::vector<Transform>& transforms, uint32_t id); // 设置指定id的植被分布

    void setHillsType(const QImage& d0, const QImage& d1);                  // 设置地形类型

    void setPlantsType(const QImage& d0, const QImage& d1);                 // 设置植被分布数据

private:
    void createBuffer(uint32_t size, void*& data, VkBuffer& buffer, VkBufferUsageFlags usage,
                      VkDeviceMemory& memory, VkMemoryRequirements& memoryRequirement, bool local);

    void createImage(VkExtent2D extent, uint32_t mipsLevel, VkFormat format, void*& data, VkImage& image, VkImageUsageFlags usage,
                     VkDeviceMemory& memory, VkMemoryRequirements& memoryRequirement, VkImageCreateFlags flags,
                     uint32_t layerCount, bool local);

    void createMaxImage();

    void fillBuffer(uint32_t size, const void* data, VkBuffer buffer, uint32_t offset);
    void fillBufferInit();
    void fillImage(uint32_t size, VkExtent2D extent, uint32_t mipsLevel, const void* data, VkImage image, VkOffset2D offset,
                   uint32_t layer, bool protectOld);
    void fillImageInit();
    void buildPipeline();
    void buildSkyPipeline();
    void loadShader();
    void createBasicRenderPass();
    void createSampler();
    void createImageView(VkImage image, VkImageAspectFlags aspect, VkFormat format, uint32_t mipsLevel, VkImageView& view,
                         uint32_t layer, uint32_t layerCount);
    void genMipmaps(VkImage image, VkExtent2D size, uint32_t mipsLevel, uint32_t layer);

private:
    const bool topView;
    QVulkanWindow* window;
    QVulkanDeviceFunctions* devFuncs;
    QVulkanFunctions* functions;
    float green = 0;
    void* notuse;

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

    VkBuffer skyVertexBuffer;
    VkDeviceMemory skyVertexMemory;
    VkMemoryRequirements skyVertexMemoryRequirement;
    void* skyVertexData;
    uint32_t skyVertexCount;

    VkPipeline standardPipeline = VK_NULL_HANDLE;
    VkPipelineLayout standardPipelineLayout = VK_NULL_HANDLE;

    VkPipeline skyBoxPipeline = VK_NULL_HANDLE;
    VkPipelineLayout skyBoxPipelineLayout = VK_NULL_HANDLE;

    VkPipeline shadowPipeline = VK_NULL_HANDLE;
    VkPipelineLayout shadowPipelineLayout = VK_NULL_HANDLE;
    // Graphics pipelines;

    VkRenderPass shadowPass;

    VkRenderPass basicPass; // unuse
    // Render Pass;

    VkShaderModule standardVertexShader;
    VkShaderModule standardFragmentShader;
    VkShaderModule skyVertexShader;
    VkShaderModule skyFragmentShader;
    VkShaderModule shadowVertexShader;
    VkShaderModule shadowFragmentShader;
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

    VkImage maxTexture;
    VkImage maxCubeTexture;

    VkImage hillTexture;
    VkImage hillNormal;
    VkImage hillPBR;
    VkImage plantsTextures[PLANT_TYPE_NUM];
    VkImage plantsNormal[PLANT_TYPE_NUM];
    VkImage plantsPBR[PLANT_TYPE_NUM];

    VkImage testImage;
    VkImageView testImageView;
    VkSampler standSampler;
    VkDeviceMemory testIMemory;
    VkMemoryRequirements testIMemoryRequirement;
    void* testImageData;

    VkDeviceMemory hillTMemory;
    VkDeviceMemory hillNormalMemory;
    VkDeviceMemory hillPBRMemory;

    VkDeviceMemory plantsTexturesMemory[PLANT_TYPE_NUM];
    VkDeviceMemory plantsNormalMemory[PLANT_TYPE_NUM];
    VkDeviceMemory plantsPBRMemory[PLANT_TYPE_NUM];

    VkMemoryRequirements hillTMemoryRequirement;
    VkMemoryRequirements hillNormalMemoryRequirement;
    VkMemoryRequirements hillPBRMemoryRequirement;

    VkMemoryRequirements plantsTexturesMemoryRequirements[PLANT_TYPE_NUM];
    VkMemoryRequirements plantsNormalMemoryRequirement[PLANT_TYPE_NUM];
    VkMemoryRequirements plantsPBRMemoryRequirement[PLANT_TYPE_NUM];

    VkImage skyImage;
    VkDeviceMemory skyMemory;
    VkMemoryRequirements skyMemoryRequirement;
    VkImageView skyImageView;

    void* hillTData;
    void* hillNormalData;
    void* hillPBRData;

    void* plantsTexturesDatas[PLANT_TYPE_NUM];
    void* plantsNormalData[PLANT_TYPE_NUM];
    void* plantsPBRData[PLANT_TYPE_NUM];

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

    VkBuffer plantsInstanceBuffer[MAX_PLANT_COUNT];
    VkDeviceMemory plantsInstanceMemory[MAX_PLANT_COUNT];
    VkMemoryRequirements plantsInstanceMemoryRequirement[MAX_PLANT_COUNT];
    void* plantsInstanceData[MAX_PLANT_COUNT];
    uint32_t plantsInstanceCount[MAX_PLANT_COUNT];

public:
    SimplePBR testPBR;
    float renderType = 0.0f;

    Camera camera;

};

struct Uniform {
    alignas(4) float time;
    alignas(4) float skyForce;
    alignas(8) glm::vec2 fog;
    alignas(16) glm::vec4 resolution;
    alignas(16) glm::vec4 sun;
    alignas(16) glm::mat4 mvp;
    alignas(16) glm::vec4 bprRSMC;
    alignas(16) glm::vec3 sunDir;
    alignas(16) glm::vec4 baseColor;
    alignas(16) glm::vec4 etc;
    alignas(16) glm::mat4 m;
    alignas(16) glm::mat4 v;
    alignas(16) glm::mat4 p;
    alignas(16) glm::vec4 scale;
};

class SceneView;

class ViewThread: public QThread {
private:
    SceneView* area;
    uint32_t delta;
public:
    ViewThread(SceneView* area, uint32_t delta);
    bool runable = true;
    void run() override;
};

class SceneView : public QVulkanWindow {
    Q_OBJECT
public:
    // 面向Qt的UI接口

    explicit SceneView(QVulkanWindow *parent = nullptr, const DataContext* dataContext = nullptr,
                       QVulkanInstance* instance = nullptr, const bool topView = false);
    ~SceneView();

    QVulkanWindowRenderer *createRenderer() override;
    void mouseMoveEvent(QMouseEvent* mouseEvent) override;
    void wheelEvent(QWheelEvent *wheelEvent) override;

public:
    // 面向WGL的接口

    Render* render;

signals:
    void updateSignal();

public slots:
    void updateSLot();

private:
    const DataContext* dataContext;
    QVulkanInstance* instance;
    const bool topView;
    ViewThread thread;

    Camera vcamera; // 虚拟相机，插值用
};

#endif // SCENEVIEW_H
