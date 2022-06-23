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
#include <mutex>
#include <glm/glm/gtc/random.hpp>
#include "datacontext.h"
#include "model.h"
#include "helperMacro.h"
#include "camera.h"
#include "renderstruct.h"

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
#define MAX_TMP_IMAGE_SIZE (2048 * 2048 * sizeof(uint8_t) * 4 * 2)
#define MAX_PLANT_COUNT (1024 * 1024)
#define U32(d) (static_cast<uint32_t>(d))
#define MIP(w, h) (glm::floor(glm::log(glm::max(w, h)) / glm::log(2) + 0.5f))
#define SRGB true
#define LINEAR false
#define UNI(type, data, offset, countInOne, sizefunc, id) (reinterpret_cast<type*>\
    (reinterpret_cast<uint8_t*>(data) + (countInOne * offset + id) * sizefunc(sizeof(type))))

#define SHADOW_SIZE 1024
#define SHADOW_FORMAT VK_FORMAT_D16_UNORM

#define OBJECTS_MAX_COUNT 256

extern bool resetup;

extern std::mutex renderLoopLock;

struct SimplePBR {
    float roughness = 1.0;    // 粗糙度
    float specular = 0.35;     // 高光强度
    float metallic = 1.0;     // 金属度
    float contrast = 1.0;     // 反射率对比度
    float ss = 1.0f;
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
    Render(QVulkanWindow *w, const bool topView, std::mutex* lock);

    void initResources() override;
    void initSwapChainResources() override;
    void releaseSwapChainResources() override;
    void releaseResources() override;
    void logicalDeviceLost() override;
    void physicalDeviceLost() override;
    void preInitResources() override;

    void startNextFrame() override;

    uint32_t uniformSize(uint32_t realSize);

public:
    // 面向WGL的接口

    void pushHill(const Vertexs& vs, const Indexs& index);                  // 设置地形数据

    void setPlant(const Vertexs& vs, const Indexs& index, uint32_t id);     // 设置指定id的植被数据

private:
    void createBuffer(uint32_t size, void*& data, VkBuffer& buffer, VkBufferUsageFlags usage,
                      VkDeviceMemory& memory, VkMemoryRequirements& memoryRequirement, bool local);

    void createImage(VkExtent2D extent, uint32_t mipsLevel, VkFormat format, void*& data, VkImage& image,
                     VkImageUsageFlags usage,
                     VkDeviceMemory& memory, VkMemoryRequirements& memoryRequirement, VkImageCreateFlags flags,
                     uint32_t layerCount, bool local, VkExtent2D size = { 0, 0 });

    void createImageOnly(VkExtent2D extent, uint32_t mipsLevel, VkFormat format, Texture2D& tex,
                        VkImageUsageFlags usage, VkImageCreateFlags flags, uint32_t layerCount, bool local);

    void createMaxImage();

    void makeDepthBuffer(VkImage image);

    void fillBufferInit();
    void fillImageInit();
    void buildPipeline();
    void buildSkyPipeline();
    void buildShadowPipeline();
    void loadShader();
    void createBasicRenderPass();
    void createSampler();
    void createImageView(VkImage image, VkImageAspectFlags aspect, VkFormat format, uint32_t mipsLevel,
                         VkImageView& view,
                         uint32_t layer, uint32_t layerCount, bool array);

    void updateUniforms(bool all, int id);

    void drawObjects(VkCommandBuffer& cmdBuf, uint32_t* dynamicBinding);

    void fillUnifromLayoutBindings(VkDescriptorSetLayoutBinding* bindings);

public:
    VkImageView albedoView = VK_NULL_HANDLE;
    VkImageView PBRView = VK_NULL_HANDLE;
    VkImageView normalView = VK_NULL_HANDLE;
    VkImageView skyView = VK_NULL_HANDLE;

    void loadImage(Texture2D& texture, const char* path, bool srgb, VkExtent2D csize = { 0, 0 });
    void loadCubemap(Texture2D& texture, const char* path[6], bool srgb);
    void loadVertex(IBuffer& buffer, uint32_t size);
    void loadIndex(IBuffer& buffer, uint32_t size);
    void loadInstance(IBuffer& buffer, uint32_t size);
    void loadUniform(IBuffer& buffer, uint32_t size, bool dynamic);
    void fillLocalData(IBuffer& buffer, const void* data, uint32_t size, VkDeviceSize offset);
    void fillBuffer(uint32_t size, const void* data, IBuffer buffer, VkDeviceSize offset);
    void fillImage(uint32_t size, VkExtent2D extent, uint32_t mipsLevel, const void* data,
                   Texture2D image, VkOffset2D offset,
                   uint32_t layer, bool protectOld);
    void genMipmaps(VkImage image, VkExtent2D size, uint32_t mipsLevel, uint32_t layer);

    void destroyBuffer(IBuffer buffer);
    void destroyImage(Texture2D texture);

    void updateImage(const QImage& img, Texture2D& texture, bool srgb, bool array);
    void updateCubeMap(const QImage* img, Texture2D& texture);

    bool ready() const;

private:
    const bool topView;
    QVulkanWindow* window;
    QVulkanDeviceFunctions* devFuncs;
    QVulkanFunctions* functions;
    float green = 0;
    void* notuse;

    bool readyToRender = false;

public:

    IBuffer terrainVertex;
    IBuffer terrainIndex;

    IBuffer plantsVertex[PLANT_TYPE_NUM];
    IBuffer plantsIndex[PLANT_TYPE_NUM];
    // Vertexs and index buffers;

    IBuffer terrainInstance;
    IBuffer plantsInstance[PLANT_TYPE_NUM];

    IBuffer testInstance;

    IBuffer skyVertex;

    IBuffer frameUniform;
    IBuffer objectsUniform;
    IBuffer shadowUniform;

    Texture2D terrainAlbedo;
    Texture2D terrainPBR;
    Texture2D terrainNormal;
    Texture2D plantsAlbedo[PLANT_TYPE_NUM];
    Texture2D plantsNormal[PLANT_TYPE_NUM];
    Texture2D plantsPBR[PLANT_TYPE_NUM];

    Texture2D testAlbedo;
    Texture2D testPBR;
    Texture2D testNormal;

    Texture2D skyCubemap;

private:

    constexpr static uint32_t UNI_FRAME_BD = 0;
    constexpr static uint32_t UNI_OBJ_BD = 1;
    constexpr static uint32_t UNI_SHADOW_BD = 2;
    constexpr static uint32_t UNI_MAX_BD = 3;
    constexpr static uint32_t UNI_BINDING_COUNT = 9;

    Texture2D shadowMap;
    VkFramebuffer shadowFrameBuffer[4];

    VkPipeline standardPipeline = VK_NULL_HANDLE;
    VkPipelineLayout standardPipelineLayout = VK_NULL_HANDLE;

    VkPipeline skyBoxPipeline = VK_NULL_HANDLE;
    VkPipelineLayout skyBoxPipelineLayout = VK_NULL_HANDLE;

    VkPipeline shadowPipeline = VK_NULL_HANDLE;
    VkPipelineLayout shadowPipelineLayout = VK_NULL_HANDLE;
    // Graphics pipelines;

    VkRenderPass shadowPass;
    // Render Pass;

    VkShaderModule standardVertexShader;
    VkShaderModule standardFragmentShader;
    VkShaderModule skyVertexShader;
    VkShaderModule skyFragmentShader;
    VkShaderModule shadowVertexShader;
    VkShaderModule shadowFragmentShader;
    // Shaders;

    Vertexs monkeyModel;
    Indexs monkeyModelIndex;
    Vertexs plantsModel[PLANT_TYPE_NUM];
    Indexs plantsModelIndex[PLANT_TYPE_NUM];

    VkDescriptorSetLayout uniformSetLayout;
    VkDescriptorPool uniformPool;
    VkDescriptorSet* uniformSet = nullptr;

    VkImage maxTexture;
    VkImage maxCubeTexture;

    VkSampler standSampler;
    VkSampler shadowSampler;

    VkBuffer tmpBuffer;
    VkDeviceMemory tmpBufferMemory;
    VkMemoryRequirements tmpBufferRequirement;
    VkBuffer tmpImage;
    VkDeviceMemory tmpImageMemory;
    VkMemoryRequirements tmpImageRequirement;

    std::mutex* lock;

public:
    SimplePBR testPBRInfo;
    SimplePBR testPBRBase = { 0.0f, 0.0f, 0.0f, 0.0f };
    float renderType = 0.0f;

    Camera camera;

};

struct TheOldUniform {
    alignas(4) float time;
    alignas(4) float skyForce;
    alignas(4) float subsurface;
    alignas(4) float ssbase;
    alignas(8) glm::vec2 fog;
    alignas(16) glm::vec3 fogColor;
    alignas(16) glm::vec4 resolution;
    alignas(16) glm::vec4 sun;
    alignas(16) glm::mat4 mvp;
    alignas(16) glm::vec4 pbrRSMC;
    alignas(16) glm::vec3 sunDir;
    alignas(16) glm::vec4 baseColor;
    alignas(16) glm::vec4 etc;
    alignas(16) glm::mat4 m;
    alignas(16) glm::mat4 v;
    alignas(16) glm::mat4 p;
    alignas(16) glm::vec4 scale;
    alignas(16) glm::vec4 pbrBase;
    alignas(16) glm::mat4 shadowV;
    alignas(8) glm::vec2 shadowBias;
};

struct UniFrame {
    alignas(4) float time;
    alignas(4) float skyForce;
    alignas(8) glm::vec2 fog;
    alignas(16) glm::vec3 fogColor;
    alignas(16) glm::vec4 resolution;
    alignas(16) glm::vec4 sun;
    alignas(16) glm::vec3 sunDir;
    alignas(16) glm::mat4 v;
    alignas(16) glm::mat4 p;
    alignas(16) glm::vec4 etc;
    alignas(8) glm::vec2 shadowBias;
};

struct UniObject {
    alignas(4) float subsurface;
    alignas(4) float ssbase;
    alignas(4) float clip;
    alignas(16) glm::mat4 mvp;
    alignas(16) glm::vec4 pbrRSMC;
    alignas(16) glm::vec4 baseColor;
    alignas(16) glm::mat4 m;
    alignas(16) glm::vec4 scale;
    alignas(16) glm::vec4 pbrBase;
};

struct UniShadow {
    alignas(16) glm::mat4 shadowV;
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

    explicit SceneView(std::mutex* lock, QVulkanWindow *parent = nullptr, const DataContext* dataContext = nullptr,
                       QVulkanInstance* instance = nullptr, const bool topView = false);
    ~SceneView();

    QVulkanWindowRenderer *createRenderer() override;
    void mouseMoveEvent(QMouseEvent* mouseEvent) override;
    void wheelEvent(QWheelEvent *wheelEvent) override;

public:
    // 面向WGL的接口

    Render* render;
    Camera vcamera; // 虚拟相机，插值用
    ViewThread thread;

signals:
    void updateSignal();

public slots:
    void updateSLot();

private:
    const DataContext* dataContext;
    QVulkanInstance* instance;
    const bool topView;

    std::mutex* lock;
};

#endif // SCENEVIEW_H
