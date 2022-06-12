#include "sceneview.h"
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm/glm.hpp>
#include <glm/glm/gtc/matrix_transform.hpp>

SceneView::SceneView(QVulkanWindow *parent, const DataContext* dataContext,
                     QVulkanInstance* instance, const bool topView)
    : QVulkanWindow{parent}, dataContext(dataContext), instance(instance), topView(topView),
      thread(this, 20) {
    setVulkanInstance(instance);
    setSampleCount(2);
    setCursor(Qt::CrossCursor);
    connect(this, &SceneView::updateSignal, this, &SceneView::updateSLot);
}

SceneView::~SceneView() {
    thread.runable = false;
    thread.wait();
}

QVulkanWindowRenderer* SceneView::createRenderer() {
    render = new Render(this, topView);
    vcamera = render->camera;
    thread.start();
    return render;
}

void SceneView::mouseMoveEvent(QMouseEvent* mouseEvent) {
    static int oldx, oldy;
    if (mouseEvent->buttons() & Qt::RightButton) {
        vcamera.turn(0.003f * glm::vec2(mouseEvent->pos().x() - oldx, -mouseEvent->pos().y() + oldy));
    }
    if (mouseEvent->buttons() & Qt::LeftButton) {
        vcamera.move(0.01f * glm::vec3(mouseEvent->pos().x() - oldx, -mouseEvent->pos().y() + oldy, 0.0f));
    }
    if (mouseEvent->buttons() & Qt::MiddleButton) {
        vcamera.move(0.01f * glm::vec3(mouseEvent->pos().x() - oldx, 0.0f, -mouseEvent->pos().y() + oldy));
    }
    oldx = mouseEvent->pos().x();
    oldy = mouseEvent->pos().y();
}

void SceneView::wheelEvent(QWheelEvent* wheelEvent) {
    vcamera.position += 0.01f * vcamera.front * static_cast<float>(wheelEvent->angleDelta().y());
}

void SceneView::updateSLot() {
    render->camera.lerp(vcamera, 0.3f);
}

void Render::textureInit() {
    QImage testImageFile(":/assets/textures/grass.png");
    QImage postImageFile = testImageFile.convertToFormat(QImage::Format_RGBA8888);
    VkExtent2D testISize = { static_cast<uint32_t>(postImageFile.width()), static_cast<uint32_t>(postImageFile.height()) };
    uint32_t testMipLevel = glm::floor(glm::log(glm::max(testISize.width, testISize.height)) / glm::log(2) + 0.5f);
    createImage(testISize, testMipLevel, VK_FORMAT_R8G8B8A8_SRGB, testImageData, testImage,
                VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                testIMemory, testIMemoryRequirement, 0, 1, false);
    createImageView(testImage, VK_IMAGE_ASPECT_COLOR_BIT, VK_FORMAT_R8G8B8A8_SRGB, testMipLevel, testImageView, 0, 1);
    fillImage(postImageFile.sizeInBytes(), testISize, testMipLevel, postImageFile.bits(), testImage, { 0, 0 }, 0, false);
    genMipmaps(testImage, testISize, testMipLevel, 0);

    QImage r_skyX(":/assets/textures/env/test/px.png");
    QImage r_sky_X(":/assets/textures/env/test/nx.png");
    QImage r_skyY(":/assets/textures/env/test/py.png");
    QImage r_sky_Y(":/assets/textures/env/test/ny.png");
    QImage r_skyZ(":/assets/textures/env/test/pz.png");
    QImage r_sky_Z(":/assets/textures/env/test/nz.png");
    QImage skyX = r_skyX.convertToFormat(QImage::Format_RGBA8888);
    QImage sky_X = r_sky_X.convertToFormat(QImage::Format_RGBA8888);
    QImage skyY = r_skyY.convertToFormat(QImage::Format_RGBA8888);
    QImage sky_Y = r_sky_Y.convertToFormat(QImage::Format_RGBA8888);
    QImage skyZ = r_skyZ.convertToFormat(QImage::Format_RGBA8888);
    QImage sky_Z = r_sky_Z.convertToFormat(QImage::Format_RGBA8888);
    VkExtent2D skySize = { static_cast<uint32_t>(skyX.width()), static_cast<uint32_t>(skyX.height()) };
    uint32_t skyMipLevel = glm::floor(glm::log(glm::max(skySize.width, skySize.height)) / glm::log(2) + 0.5f);
    createImage(skySize, skyMipLevel, VK_FORMAT_R8G8B8A8_SRGB, notuse, skyImage,
                VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT, skyMemory, skyMemoryRequirement,
                VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT, 6, false);
    createImageView(skyImage, VK_IMAGE_ASPECT_COLOR_BIT, VK_FORMAT_R8G8B8A8_SRGB, skyMipLevel, skyImageView, 0, 6);
    fillImage(skyX.sizeInBytes(), skySize, skyMipLevel, skyX.bits(), skyImage, { 0, 0 }, 0, false);
    fillImage(sky_X.sizeInBytes(), skySize, skyMipLevel, sky_X.bits(), skyImage, { 0, 0 }, 1, true);
    fillImage(skyY.sizeInBytes(), skySize, skyMipLevel, skyY.bits(), skyImage, { 0, 0 }, 2, true);
    fillImage(sky_Y.sizeInBytes(), skySize, skyMipLevel, sky_Y.bits(), skyImage, { 0, 0 }, 3, true);
    fillImage(skyZ.sizeInBytes(), skySize, skyMipLevel, skyZ.bits(), skyImage, { 0, 0 }, 4, true);
    fillImage(sky_Z.sizeInBytes(), skySize, skyMipLevel, sky_Z.bits(), skyImage, { 0, 0 }, 5, true);
    genMipmaps(skyImage, skySize, skyMipLevel, 0);
    genMipmaps(skyImage, skySize, skyMipLevel, 1);
    genMipmaps(skyImage, skySize, skyMipLevel, 2);
    genMipmaps(skyImage, skySize, skyMipLevel, 3);
    genMipmaps(skyImage, skySize, skyMipLevel, 4);
    genMipmaps(skyImage, skySize, skyMipLevel, 5);
}

void Render::vertexInit() {
    createBuffer(MAX_HILL_VSIZE * VertexSize, hillVData,
                 hillVertexBuffer, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                 hillMemory, hillMemoryRequirement, false);

    createBuffer(MAX_HILL_I_SIZE * IndexSize, hillIndexData,
                 hillIndexBuffer, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                 hillIndexMemory, hillIndexMemoryRequirement, false);

    for (uint32_t i = 0; i < PLANT_TYPE_NUM; i++) {
        createBuffer(MAX_PLANT_VSIZE * VertexSize, plantsVData[i],
                     plantsVertexBuffers[i], VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                     plantsMemories[i], plantsMemoriesRequirements[i], false);

        createBuffer(MAX_PLANT_VSIZE * IndexSize, plantsIndexDatas[i],
                     plantsIndexBuffers[i], VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                     plantsIndexMemories[i], plantsIndexMemoryRequirements[i], false);
    }

    pushHill(monkey, monkeyIndex);

    createBuffer(sizeof(glm::vec2) * 4, skyVertexData, skyVertexBuffer,
                 VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, skyMemory,
                 skyMemoryRequirement, false);
    glm::vec2 skyPoints[4] = {
        { -1.0f, -1.0f }, { -1.0f, 1.0f }, { 1.0f, 1.0f }, { 1.0f, -1.0f }
    };
    skyVertexCount = 4;
    fillBuffer(4 * sizeof(glm::vec2), skyPoints, skyVertexBuffer, 0);
}

void Render::uniformInit() {
    createBuffer(uniformSize * window->concurrentFrameCount(),
                 uniformData, uniform, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                 uniformMemory, unigormMemoryRequirement, true);
}

void Render::instanceInit() {
    createBuffer(MAX_PLANT_COUNT * InstanceSize, testInstanceData, testInstanceBuffer, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                 testInstanceMemory, testInstanceMemoryRequirement, true);

    testInstanceCount = 128;
    std::vector<MMat> idata;
    for (uint32_t i = 0; i < testInstanceCount; i++) {
        Transform t{};
        t.position = glm::vec3(glm::sphericalRand(3.5f));
        t.rotation = glm::vec3(glm::linearRand(0.0, 6.28), glm::linearRand(0.0, 6.28), glm::linearRand(0.0, 6.28));
        t.scale = glm::vec3(0.5f);
        idata.push_back(Model::packTransform(t));
    }

    memcpy(testInstanceData, idata.data(), idata.size() * InstanceSize);
}

Render::Render(QVulkanWindow* w, const bool topView): topView(topView), window(w) {

    camera.position = glm::vec3(5.0, 5.0, 3.0f);
    camera.lookAt(glm::vec3(0.0f, 0.0f, 0.0f));

    Model::loadFromOBJFile(":/assets/models/smoothMonkey.obj", monkey, monkeyIndex, false);

}

void Render::initResources() {
    devFuncs = window->vulkanInstance()->deviceFunctions(window->device());
    functions = window->vulkanInstance()->functions();

    uniformSet = new VkDescriptorSet[window->concurrentFrameCount()];
    // allocate uniform descriptor sets pointers.

    const VkPhysicalDeviceLimits *pdevLimits = &window->physicalDeviceProperties()->limits;
    const VkDeviceSize uniAlign = pdevLimits->minUniformBufferOffsetAlignment;
    uniformSize = (sizeof(Uniform) / uniAlign + 1) * uniAlign;
    printf("Allcate %u uniform description set.\n", window->concurrentFrameCount());
    // get uniform create information.

    createMaxImage(); // to get max image memory size
    fillBufferInit(); // create stage buffer
    fillImageInit();  // create stage image buffer
    createSampler();  // create image standard sampler

    vertexInit();

    uniformInit();

    textureInit();

    loadShader();

    instanceInit();

    buildPipeline();
    buildSkyPipeline();
}

void Render::initSwapChainResources() {
    QVulkanWindowRenderer::initSwapChainResources();
}

void Render::releaseSwapChainResources() {
    QVulkanWindowRenderer::releaseSwapChainResources();
}

void Render::releaseResources() {
    VkDevice dev = window->device();
    devFuncs->vkFreeMemory(dev, hillMemory, VDFT);
    devFuncs->vkFreeMemory(dev, hillIndexMemory, VDFT);
    devFuncs->vkDestroyBuffer(dev, hillVertexBuffer, VDFT);
    devFuncs->vkDestroyBuffer(dev, hillIndexBuffer, VDFT);

    for (uint32_t i = 0; i < PLANT_TYPE_NUM; i++) {
        devFuncs->vkFreeMemory(dev, plantsMemories[i], VDFT);
        devFuncs->vkFreeMemory(dev, plantsIndexMemories[i], VDFT);
        devFuncs->vkDestroyBuffer(dev, plantsVertexBuffers[i], VDFT);
        devFuncs->vkDestroyBuffer(dev, plantsIndexBuffers[i], VDFT);
    }

    devFuncs->vkFreeMemory(dev, uniformMemory, VDFT);
    devFuncs->vkDestroyBuffer(dev, uniform, VDFT);
    devFuncs->vkDestroyDescriptorSetLayout(dev, uniformSetLayout, VDFT);
    for (int i = 0; i < window->concurrentFrameCount(); i++) {
        devFuncs->vkFreeDescriptorSets(dev, uniformPool, 1, &uniformSet[i]);
    }
    devFuncs->vkDestroyDescriptorPool(dev, uniformPool, VDFT);
    delete[] uniformSet;

    devFuncs->vkFreeMemory(dev, tmpBufferMemory, VDFT);
    devFuncs->vkFreeMemory(dev, tmpImageMemory, VDFT);
    devFuncs->vkDestroyBuffer(dev, tmpBuffer, VDFT);
    devFuncs->vkDestroyBuffer(dev, tmpImage, VDFT);

    devFuncs->vkDestroyShaderModule(dev, standardVertexShader, VDFT);
    devFuncs->vkDestroyShaderModule(dev, standardFragmentShader, VDFT);
    devFuncs->vkDestroyShaderModule(dev, skyVertexShader, VDFT);
    devFuncs->vkDestroyShaderModule(dev, skyFragmentShader, VDFT);

    devFuncs->vkDestroyPipelineLayout(dev, standardPipelineLayout, VDFT);
    devFuncs->vkDestroyPipeline(dev, standardPipeline, VDFT);

    devFuncs->vkDestroyImageView(dev, testImageView, VDFT);
    devFuncs->vkDestroyImage(dev, testImage, VDFT);
    devFuncs->vkFreeMemory(dev, testIMemory, VDFT);

    devFuncs->vkDestroySampler(dev, standSampler, VDFT);

    devFuncs->vkFreeMemory(dev, testInstanceMemory, VDFT);
    devFuncs->vkDestroyBuffer(dev, testInstanceBuffer, VDFT);

    devFuncs->vkFreeMemory(dev, skyVertexMemory, VDFT);
    devFuncs->vkDestroyBuffer(dev, skyVertexBuffer, VDFT);

    devFuncs->vkDestroyImageView(dev, skyImageView, VDFT);
    devFuncs->vkDestroyImage(dev, skyImage, VDFT);
    devFuncs->vkFreeMemory(dev, skyMemory, VDFT);

}

void Render::logicalDeviceLost() {
    perror("Device lost!!\n");
    QVulkanWindowRenderer::logicalDeviceLost();
}

void Render::physicalDeviceLost() {
    perror("Physical device lost!!\n");
    QVulkanWindowRenderer::physicalDeviceLost();
}

void Render::startNextFrame() {
    green += 0.005f;
    if (green > 1.0f)
        green = 0.0f;

    VkClearColorValue clearColor = {{ green, green, 0.0f, 1.0f }};
    VkClearDepthStencilValue clearDS = { 1.0f, 0 };
    VkClearValue clearValues[3] = {};
    clearValues[0].color = clearColor;
    clearValues[1].depthStencil = clearDS;
    glm::vec3 env = glm::vec3(0.25f, 0.4f, 0.65f);
    env = glm::pow(env, glm::vec3(1.0f / 2.2f));
    clearValues[2].color = {{ env.r, env.g, env.b }};

    VkRenderPassBeginInfo rpBeginInfo{};
    rpBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpBeginInfo.renderPass = window->defaultRenderPass();
    rpBeginInfo.framebuffer = window->currentFramebuffer();
    const QSize sz = window->swapChainImageSize();
    rpBeginInfo.renderArea.extent.width = sz.width();
    rpBeginInfo.renderArea.extent.height = sz.height();
    rpBeginInfo.clearValueCount = window->sampleCountFlagBits() > VK_SAMPLE_COUNT_1_BIT ? 3 : 2;
    rpBeginInfo.pClearValues = clearValues;
    VkCommandBuffer cmdBuf = window->currentCommandBuffer();

    VkViewport viewport{};
    viewport.width = window->swapChainImageSize().width();
    viewport.height = window->swapChainImageSize().height();
    viewport.minDepth = 0.0;
    viewport.maxDepth = 1.0;
    viewport.x = 0.0;
    viewport.y = 0.0;

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = VkExtent2D {
        static_cast<uint32_t>(window->swapChainImageSize().width()),
        static_cast<uint32_t>(window->swapChainImageSize().height())
    };

    Uniform unif{};
    unif.time = static_cast<float>(clock() / 1000.0f);

    unif.resolution = glm::vec4(static_cast<float>(scissor.extent.width), static_cast<float>(scissor.extent.height), 1.0f, 1.0f);
    unif.baseColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    unif.bprRSMC = glm::vec4(testPBR.roughness, testPBR.specular, testPBR.metallic, testPBR.contrast);
    unif.etc.x = renderType;
    unif.m = glm::mat4(1.0f, 0.0f, 0.0f, 0.0f,
                       0.0f, 1.0f, 0.0f, 0.0f,
                       0.0f, 0.0f, 1.0f, 0.0f,
                       0.0f, 0.0f, 0.0f, 1.0f);
    unif.v = camera.getV();
    unif.p = camera.getP(glm::vec2(static_cast<float>(scissor.extent.width), static_cast<float>(scissor.extent.height)));
    unif.mvp = unif.p * unif.v * unif.m;
    unif.sun = glm::vec4(1.0f, 0.97f, 0.9f, 0.6f);
    unif.sunDir = glm::normalize(glm::vec3(-0.1, 0.9, -0.8));
    memcpy(static_cast<uint8_t*>(uniformData) + window->currentFrame() * uniformSize, &unif, sizeof(Uniform));

    VkDeviceSize offsets[] = { 0 };
    VkDeviceSize instanceOffsets[] = { 0 };

    devFuncs->vkCmdBeginRenderPass(cmdBuf, &rpBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    devFuncs->vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, skyBoxPipeline);

    devFuncs->vkCmdSetViewport(cmdBuf, 0, 1, &viewport);
    devFuncs->vkCmdSetScissor(cmdBuf, 0, 1, &scissor);

    devFuncs->vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, skyBoxPipelineLayout, 0, 1,
                                      &uniformSet[window->currentFrame()], 0, nullptr);

    offsets[0] = 0;
    devFuncs->vkCmdBindVertexBuffers(cmdBuf, 0, 1, &skyVertexBuffer, offsets);

    devFuncs->vkCmdDraw(cmdBuf, skyVertexCount, 1, 0, 0);

    devFuncs->vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, standardPipeline);

    devFuncs->vkCmdSetViewport(cmdBuf, 0, 1, &viewport);
    devFuncs->vkCmdSetScissor(cmdBuf, 0, 1, &scissor);

    devFuncs->vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, standardPipelineLayout, 0, 1,
                                      &uniformSet[window->currentFrame()], 0, nullptr);

    offsets[0] = 0;
    instanceOffsets[0] = 0;
    devFuncs->vkCmdBindVertexBuffers(cmdBuf, 0, 1, &hillVertexBuffer, offsets);
    devFuncs->vkCmdBindIndexBuffer(cmdBuf, hillIndexBuffer, 0, IndexType);

    devFuncs->vkCmdBindVertexBuffers(cmdBuf, 1, 1, &testInstanceBuffer, instanceOffsets);
    devFuncs->vkCmdDrawIndexed(cmdBuf, hillIndexCount, testInstanceCount, 0, 0, 0);

    devFuncs->vkCmdEndRenderPass(cmdBuf);

    window->frameReady();
    window->requestUpdate(); // render continuously, throttled by the presentation rate
}

void Render::pushHill(const Vertexs& vs, const Indexs& index) {
    fillBuffer(vs.size() * VertexSize, vs.data(), hillVertexBuffer, 0);
    hillVsCount = vs.size();
    fillBuffer(index.size() * IndexSize, index.data(), hillIndexBuffer, 0);
    hillIndexCount = index.size();
}

void Render::createBuffer(uint32_t size, void*& data, VkBuffer& buffer, VkBufferUsageFlags usage,
                          VkDeviceMemory& memory, VkMemoryRequirements& memoryRequirement, bool local) {
    VkBufferCreateInfo bufferCreateInfo{};
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    bufferCreateInfo.size = size;
    bufferCreateInfo.usage = usage;
    VK(devFuncs->vkCreateBuffer(window->device(), &bufferCreateInfo, VDFT, &buffer));

    devFuncs->vkGetBufferMemoryRequirements(window->device(), buffer, &memoryRequirement);
    VkMemoryAllocateInfo memoryAllocateInfo{};
    memoryAllocateInfo.allocationSize = memoryRequirement.size;
    memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    VkPhysicalDeviceMemoryProperties memoryProperties;
    functions->vkGetPhysicalDeviceMemoryProperties(window->physicalDevice(), &memoryProperties);
    memoryAllocateInfo.memoryTypeIndex = 1;
    for (size_t i = 0; i < memoryProperties.memoryTypeCount; i++) {
        VkMemoryPropertyFlags flags = local ?
                    (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) :
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        if (memoryProperties.memoryTypes[i].propertyFlags & flags) {
            if (memoryRequirement.memoryTypeBits & (1 << i)) {
                memoryAllocateInfo.memoryTypeIndex = i; break;
            }
        }
    }
    VK(devFuncs->vkAllocateMemory(window->device(), &memoryAllocateInfo, VDFT, &memory));
    VK(devFuncs->vkBindBufferMemory(window->device(), buffer, memory, 0));


    if (local) { VK(devFuncs->vkMapMemory(window->device(), memory, 0, memoryRequirement.size, 0, &data)); }
}

void Render::createImage(VkExtent2D extent, uint32_t mipsLevel, VkFormat format, void*& data, VkImage& image, VkImageUsageFlags usage,
                         VkDeviceMemory& memory, VkMemoryRequirements& memoryRequirement, VkImageCreateFlags flags,
                         uint32_t layerCount, bool local) {
    VkImageCreateInfo imageCreateInfo{};
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.extent.width = extent.width;
    imageCreateInfo.extent.height = extent.height;
    imageCreateInfo.extent.depth = 1;
    imageCreateInfo.arrayLayers = layerCount;
    imageCreateInfo.format = format;
    imageCreateInfo.initialLayout = local ? VK_IMAGE_LAYOUT_PREINITIALIZED : VK_IMAGE_LAYOUT_UNDEFINED;
    imageCreateInfo.mipLevels = mipsLevel;
    imageCreateInfo.tiling = local ? VK_IMAGE_TILING_LINEAR : VK_IMAGE_TILING_OPTIMAL;
    imageCreateInfo.usage = usage;
    imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageCreateInfo.flags = flags;
    VK(devFuncs->vkCreateImage(window->device(), &imageCreateInfo, VDFT, &image));

    devFuncs->vkGetImageMemoryRequirements(window->device(), layerCount > 1 ? maxCubeTexture : maxTexture, &memoryRequirement);

    VkMemoryAllocateInfo memoryAllocateInfo{};
    memoryAllocateInfo.memoryTypeIndex = 1;
    VkPhysicalDeviceMemoryProperties memoryProperties;
    functions->vkGetPhysicalDeviceMemoryProperties(window->physicalDevice(), &memoryProperties);
    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++) {
        VkMemoryPropertyFlags flag = local ?
                    (VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) :
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        if (memoryProperties.memoryTypes[i].propertyFlags & flag) {
            if (memoryRequirement.memoryTypeBits & (1 << i)) {
                memoryAllocateInfo.memoryTypeIndex = i;
            }
        }
    }
    memoryAllocateInfo.allocationSize = memoryRequirement.size;
    memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;

    VK(devFuncs->vkAllocateMemory(window->device(), &memoryAllocateInfo, VDFT, &memory));
    VK(devFuncs->vkBindImageMemory(window->device(), image, memory, 0));

    if (local) { devFuncs->vkMapMemory(window->device(), memory, 0, memoryRequirement.size, 0, &data); }
}

void Render::createMaxImage()
{
    VkImageCreateInfo imageCreateInfo{};
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.extent.width = MAX_IMAGE_WIDTH;
    imageCreateInfo.extent.height = MAX_IMAGE_HEIGHT;
    imageCreateInfo.extent.depth = 1;
    imageCreateInfo.arrayLayers = 1;
    imageCreateInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageCreateInfo.mipLevels = glm::floor(glm::log(glm::max(MAX_IMAGE_WIDTH, MAX_IMAGE_HEIGHT)) / glm::log(2) + 0.5f);
    imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCreateInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VK(devFuncs->vkCreateImage(window->device(), &imageCreateInfo, VDFT, &maxTexture));

    VkImageCreateInfo cubeCreateInfo{};
    cubeCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    cubeCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    cubeCreateInfo.extent.width = MAX_IMAGE_WIDTH;
    cubeCreateInfo.extent.height = MAX_IMAGE_HEIGHT;
    cubeCreateInfo.extent.depth = 1;
    cubeCreateInfo.arrayLayers = 6;
    cubeCreateInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    cubeCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    cubeCreateInfo.mipLevels = glm::floor(glm::log(glm::max(MAX_IMAGE_WIDTH, MAX_IMAGE_HEIGHT)) / glm::log(2) + 0.5f);
    cubeCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    cubeCreateInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    cubeCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    cubeCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    cubeCreateInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    VK(devFuncs->vkCreateImage(window->device(), &cubeCreateInfo, VDFT, &maxCubeTexture));
}

void Render::fillBuffer(uint32_t size, const void* data, VkBuffer buffer, uint32_t offset) {
    void* GPUData;
    devFuncs->vkMapMemory(window->device(), tmpBufferMemory, 0, tmpBufferRequirement.size, 0, &GPUData);
    memcpy(GPUData, data, size);
    devFuncs->vkUnmapMemory(window->device(), tmpBufferMemory);

    VkCommandBufferAllocateInfo cmdAllocateInfo{};
    cmdAllocateInfo.commandBufferCount = 1;
    cmdAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdAllocateInfo.commandPool = window->graphicsCommandPool();

    VkCommandBuffer cmdBuf;
    VK(devFuncs->vkAllocateCommandBuffers(window->device(), &cmdAllocateInfo, &cmdBuf));

    VkCommandBufferBeginInfo cmdBeginInfo{};
    cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    devFuncs->vkBeginCommandBuffer(cmdBuf, &cmdBeginInfo);

    VkBufferCopy copy{};
    copy.dstOffset = offset;
    copy.srcOffset = 0;
    copy.size = size;
    devFuncs->vkCmdCopyBuffer(cmdBuf, tmpBuffer, buffer, 1, &copy);

    devFuncs->vkEndCommandBuffer(cmdBuf);

    VkSubmitInfo submit{};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &cmdBuf;
    devFuncs->vkQueueSubmit(window->graphicsQueue(), 1, &submit, VK_NULL_HANDLE);
    devFuncs->vkQueueWaitIdle(window->graphicsQueue());

    devFuncs->vkFreeCommandBuffers(window->device(), window->graphicsCommandPool(), 1, &cmdBuf);
}

void Render::fillBufferInit() {
    VkBufferCreateInfo tmpBufCreateInfo{};
    tmpBufCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    tmpBufCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    tmpBufCreateInfo.size = MAX_TMP_BUFFER_SIZE;
    tmpBufCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;;
    VK(devFuncs->vkCreateBuffer(window->device(), &tmpBufCreateInfo, VDFT, &tmpBuffer));

    devFuncs->vkGetBufferMemoryRequirements(window->device(), tmpBuffer, &tmpBufferRequirement);
    VkMemoryAllocateInfo tmpAllocateInfo{};
    tmpAllocateInfo.memoryTypeIndex = 1;
    VkPhysicalDeviceMemoryProperties memoryProperties;
    functions->vkGetPhysicalDeviceMemoryProperties(window->physicalDevice(), &memoryProperties);
    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++) {
        if (memoryProperties.memoryTypes[i].propertyFlags & (VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)) {
            if (tmpBufferRequirement.memoryTypeBits & (1 << i)) {
                tmpAllocateInfo.memoryTypeIndex = i;
                break;
            }
        }
    }
    tmpAllocateInfo.allocationSize = tmpBufferRequirement.size;
    tmpAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    VK(devFuncs->vkAllocateMemory(window->device(), &tmpAllocateInfo, VDFT, &tmpBufferMemory));
    VK(devFuncs->vkBindBufferMemory(window->device(), tmpBuffer, tmpBufferMemory, 0));
}

void Render::fillImage(uint32_t size, VkExtent2D extent, uint32_t mipsLevel, const void* data, VkImage image,
                       VkOffset2D offset, uint32_t layer, bool protectOld) {
    void* GPUData;
    devFuncs->vkMapMemory(window->device(), tmpImageMemory, 0, tmpImageRequirement.size, 0, &GPUData);
    memcpy(GPUData, data, size);
    devFuncs->vkUnmapMemory(window->device(), tmpImageMemory);

    VkCommandBuffer cmdBuf;
    VkCommandBufferAllocateInfo cmdAllcoateInfo{};
    cmdAllcoateInfo.commandBufferCount = 1;
    cmdAllcoateInfo.commandPool = window->graphicsCommandPool();
    cmdAllcoateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdAllcoateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    VK(devFuncs->vkAllocateCommandBuffers(window->device(), &cmdAllcoateInfo, &cmdBuf));

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    devFuncs->vkBeginCommandBuffer(cmdBuf, &beginInfo);

    VkImageMemoryBarrier makeImageDstBarrier{};
    makeImageDstBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    makeImageDstBarrier.image = image;
    makeImageDstBarrier.oldLayout = protectOld ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_UNDEFINED;
    makeImageDstBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    makeImageDstBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    makeImageDstBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    makeImageDstBarrier.srcAccessMask = protectOld ? VK_ACCESS_SHADER_READ_BIT : VK_ACCESS_NONE;
    makeImageDstBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    makeImageDstBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    makeImageDstBarrier.subresourceRange.baseArrayLayer = layer;
    makeImageDstBarrier.subresourceRange.baseMipLevel = 0;
    makeImageDstBarrier.subresourceRange.layerCount = 1;
    makeImageDstBarrier.subresourceRange.levelCount = mipsLevel;
    devFuncs->vkCmdPipelineBarrier(cmdBuf, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                   0,
                                   0, nullptr,
                                   0, nullptr,
                                   1, &makeImageDstBarrier);

    VkBufferImageCopy copy{};
    copy.bufferImageHeight = 0;
    copy.bufferOffset = 0;
    copy.bufferRowLength = 0;
    copy.imageOffset = { offset.x, offset.y, 0 };
    copy.imageExtent = { extent.width, extent.height, 1 };
    copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copy.imageSubresource.baseArrayLayer = layer;
    copy.imageSubresource.layerCount = 1;
    copy.imageSubresource.mipLevel = 0;
    devFuncs->vkCmdCopyBufferToImage(cmdBuf, tmpImage, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);

    VkImageMemoryBarrier makeImageShaderUseBarrier{};
    makeImageShaderUseBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    makeImageShaderUseBarrier.image = image;
    makeImageShaderUseBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    makeImageShaderUseBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    makeImageShaderUseBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    makeImageShaderUseBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    makeImageShaderUseBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    makeImageShaderUseBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    makeImageShaderUseBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    makeImageShaderUseBarrier.subresourceRange.baseArrayLayer = layer;
    makeImageShaderUseBarrier.subresourceRange.baseMipLevel = 0;
    makeImageShaderUseBarrier.subresourceRange.layerCount = 1;
    makeImageShaderUseBarrier.subresourceRange.levelCount = mipsLevel;
    devFuncs->vkCmdPipelineBarrier(cmdBuf, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
                                   0,
                                   0, nullptr,
                                   0, nullptr,
                                   1, &makeImageShaderUseBarrier);

    devFuncs->vkEndCommandBuffer(cmdBuf);

    VkSubmitInfo submitInfo{};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmdBuf;
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    devFuncs->vkQueueSubmit(window->graphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
    devFuncs->vkQueueWaitIdle(window->graphicsQueue());

    devFuncs->vkFreeCommandBuffers(window->device(), window->graphicsCommandPool(), 1, &cmdBuf);
}

void Render::fillImageInit() {
    VkBufferCreateInfo tmpCreateInfo{};
    tmpCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    tmpCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    tmpCreateInfo.size = MAX_TMP_IMAGE_SIZE;
    tmpCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    VK(devFuncs->vkCreateBuffer(window->device(), &tmpCreateInfo, VDFT, &tmpImage));

    devFuncs->vkGetBufferMemoryRequirements(window->device(), tmpImage, &tmpImageRequirement);

    VkMemoryAllocateInfo tmpMemoryAllocateInfo{};
    tmpMemoryAllocateInfo.memoryTypeIndex = 1;
    VkPhysicalDeviceMemoryProperties memoryProperties;
    functions->vkGetPhysicalDeviceMemoryProperties(window->physicalDevice(), &memoryProperties);
    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++) {
        if (memoryProperties.memoryTypes[i].propertyFlags & (VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)) {
            if (tmpImageRequirement.memoryTypeBits & (1 << i)) {
                tmpMemoryAllocateInfo.memoryTypeIndex = i;
                break;
            }
        }
    }
    tmpMemoryAllocateInfo.allocationSize = tmpImageRequirement.size;
    tmpMemoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    VK(devFuncs->vkAllocateMemory(window->device(), &tmpMemoryAllocateInfo, VDFT, &tmpImageMemory));
    VK(devFuncs->vkBindBufferMemory(window->device(), tmpImage, tmpImageMemory, 0));
}

void Render::buildPipeline() {
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.setLayoutCount = 1;

    VkDescriptorSetLayoutCreateInfo setLayoutCreateInfo{};
    setLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

    VkDescriptorSetLayoutBinding uniBinding{};
    uniBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uniBinding.binding = 0;
    uniBinding.descriptorCount = 1;
    uniBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    VkDescriptorSetLayoutBinding texBinding0{};  // 反射率贴图
    texBinding0.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    texBinding0.binding = 1;
    texBinding0.descriptorCount = 1;
    texBinding0.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    VkDescriptorSetLayoutBinding pbrBinding{};  // PBR纹理
    pbrBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    pbrBinding.binding = 2;
    pbrBinding.descriptorCount = 1;
    pbrBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT;
    VkDescriptorSetLayoutBinding skyBinding{};
    skyBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    skyBinding.binding = 3;
    skyBinding.descriptorCount = 1;
    skyBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutBinding bindings[] = { uniBinding, texBinding0, pbrBinding, skyBinding };
    setLayoutCreateInfo.bindingCount = 4;
    setLayoutCreateInfo.pBindings = bindings;
    VK(devFuncs->vkCreateDescriptorSetLayout(window->device(), &setLayoutCreateInfo, VDFT, &uniformSetLayout));

    pipelineLayoutCreateInfo.pSetLayouts = &uniformSetLayout;
    VK(devFuncs->vkCreatePipelineLayout(window->device(), &pipelineLayoutCreateInfo, VDFT, &standardPipelineLayout));

    VkDescriptorPoolCreateInfo poolCreateInfo{};

    VkDescriptorPoolSize uniPoolSize{};
    uniPoolSize.descriptorCount = window->concurrentFrameCount();
    uniPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    VkDescriptorPoolSize texPoolSize0{};
    texPoolSize0.descriptorCount = window->concurrentFrameCount();
    texPoolSize0.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    VkDescriptorPoolSize pbrPoolSize{};
    pbrPoolSize.descriptorCount = window->concurrentFrameCount();
    pbrPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    VkDescriptorPoolSize skyPoolSize{};
    skyPoolSize.descriptorCount = window->concurrentFrameCount();
    skyPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    VkDescriptorPoolSize poolSizes[] = { uniPoolSize, texPoolSize0, pbrPoolSize, skyPoolSize };

    poolCreateInfo.poolSizeCount = 4;
    poolCreateInfo.pPoolSizes = poolSizes;
    poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolCreateInfo.maxSets = window->concurrentFrameCount();
    VK(devFuncs->vkCreateDescriptorPool(window->device(), &poolCreateInfo, VDFT, &uniformPool));

    for (int i = 0; i < window->concurrentFrameCount(); i++) {
        VkDescriptorSetAllocateInfo uniformAllocateInfo{};
        uniformAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        uniformAllocateInfo.descriptorPool = uniformPool;
        uniformAllocateInfo.descriptorSetCount = 1;
        uniformAllocateInfo.pSetLayouts = &uniformSetLayout;
        VK(devFuncs->vkAllocateDescriptorSets(window->device(), &uniformAllocateInfo, &uniformSet[i]));
    }

    VkPipelineColorBlendStateCreateInfo colorInfo{};
    colorInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorInfo.attachmentCount = 1;
    VkPipelineColorBlendAttachmentState colorBlendState{};
    colorBlendState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendState.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendState.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendState.alphaBlendOp = VK_BLEND_OP_ADD;
    colorBlendState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                     VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendState.blendEnable = VK_FALSE;
    colorInfo.pAttachments = &colorBlendState;

    VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo{};
    graphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    graphicsPipelineCreateInfo.layout = standardPipelineLayout; // TODO
    graphicsPipelineCreateInfo.pColorBlendState = &colorInfo;
    VkPipelineMultisampleStateCreateInfo AACreateInfo{};
    AACreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    AACreateInfo.rasterizationSamples = window->sampleCountFlagBits();
    VkSampleMask sampleMask = ~0x0;
    AACreateInfo.pSampleMask = &sampleMask;
    AACreateInfo.alphaToCoverageEnable = VK_TRUE;
    graphicsPipelineCreateInfo.pMultisampleState = &AACreateInfo;
    VkPipelineShaderStageCreateInfo vertexStageCreateInfo{};
    vertexStageCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertexStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertexStageCreateInfo.module = standardVertexShader;
    vertexStageCreateInfo.pName = "main";
    VkPipelineShaderStageCreateInfo fragmentStageCreateInfo{};
    fragmentStageCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragmentStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragmentStageCreateInfo.module = standardFragmentShader;
    fragmentStageCreateInfo.pName = "main";
    VkPipelineShaderStageCreateInfo shaderStages[] = { vertexStageCreateInfo, fragmentStageCreateInfo };
    graphicsPipelineCreateInfo.pStages = shaderStages;
    graphicsPipelineCreateInfo.stageCount = 2;
    graphicsPipelineCreateInfo.renderPass = window->defaultRenderPass();
    graphicsPipelineCreateInfo.subpass = 0;

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    VkVertexInputAttributeDescription descriptions[Model::VERTEX_INPUT_COUNT + Model::INSTANCE_INPUT_COUNT];
    Model::vertexInputAttributeDescription(descriptions);
    Model::instanceInputAttributeDescription(descriptions + Model::VERTEX_INPUT_COUNT);
    vertexInputInfo.vertexAttributeDescriptionCount = Model::VERTEX_INPUT_COUNT + Model::INSTANCE_INPUT_COUNT;
    vertexInputInfo.pVertexAttributeDescriptions = descriptions;

    vertexInputInfo.vertexBindingDescriptionCount = 2;
    VkVertexInputBindingDescription inputBindingDescription{};
    Model::vertexInputBindingDescription(inputBindingDescription);
    VkVertexInputBindingDescription inputInstanceBindingDescription{};
    Model::instanceInputBindingDescription(inputInstanceBindingDescription);
    VkVertexInputBindingDescription inputBindings[] = { inputBindingDescription, inputInstanceBindingDescription };
    vertexInputInfo.pVertexBindingDescriptions = inputBindings;

    graphicsPipelineCreateInfo.pVertexInputState = &vertexInputInfo;
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyState{};
    inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyState.primitiveRestartEnable = VK_TRUE;
    inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
    graphicsPipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.lineWidth = 1.0f;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthClampEnable = VK_FALSE;
    graphicsPipelineCreateInfo.pRasterizationState = &rasterizer;
    VkPipelineDepthStencilStateCreateInfo depthTest{};
    depthTest.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthTest.depthTestEnable = VK_TRUE;
    depthTest.depthWriteEnable = VK_TRUE;
    depthTest.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    graphicsPipelineCreateInfo.pDepthStencilState = &depthTest;
    VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo{};
    VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    dynamicStateCreateInfo.dynamicStateCount = 2;
    dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicStateCreateInfo.pDynamicStates = dynamicStates;
    graphicsPipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;

    VK(devFuncs->vkCreateGraphicsPipelines(window->device(), VDFT, 1, &graphicsPipelineCreateInfo, VDFT, &standardPipeline));


    for (int i = 0; i < window->concurrentFrameCount(); i++) {
        VkWriteDescriptorSet uniformWrite{};
        uniformWrite.descriptorCount = 1;
        uniformWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uniformWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        uniformWrite.dstArrayElement = 0;
        uniformWrite.dstSet = uniformSet[i];
        uniformWrite.dstBinding = 0;
        VkDescriptorBufferInfo uniformBufferInfo{};
        uniformBufferInfo.buffer = uniform;
        uniformBufferInfo.offset = i * uniformSize;
        uniformBufferInfo.range = uniformSize;
        uniformWrite.pBufferInfo = &uniformBufferInfo;
        devFuncs->vkUpdateDescriptorSets(window->device(), 1, &uniformWrite, 0, nullptr);

        VkWriteDescriptorSet textureWrite0{};
        textureWrite0.descriptorCount = 1;
        textureWrite0.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        textureWrite0.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        textureWrite0.dstArrayElement = 0;
        textureWrite0.dstSet = uniformSet[i];
        textureWrite0.dstBinding = 1;
        VkDescriptorImageInfo textureImage0Info{};
        textureImage0Info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        textureImage0Info.imageView = testImageView;
        textureImage0Info.sampler = standSampler;
        textureWrite0.pImageInfo = &textureImage0Info;
        devFuncs->vkUpdateDescriptorSets(window->device(), 1, &textureWrite0, 0, nullptr);

        VkWriteDescriptorSet pbrTexWrite0{};
        pbrTexWrite0.descriptorCount = 1;
        pbrTexWrite0.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        pbrTexWrite0.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        pbrTexWrite0.dstArrayElement = 0;
        pbrTexWrite0.dstSet = uniformSet[i];
        pbrTexWrite0.dstBinding = 2;
        VkDescriptorImageInfo pbrTexImage0Info{};
        pbrTexImage0Info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        pbrTexImage0Info.imageView = testImageView;
        pbrTexImage0Info.sampler = standSampler;
        pbrTexWrite0.pImageInfo = &pbrTexImage0Info;
        devFuncs->vkUpdateDescriptorSets(window->device(), 1, &pbrTexWrite0, 0, nullptr);

        VkWriteDescriptorSet skyTexWrite0{};
        skyTexWrite0.descriptorCount = 1;
        skyTexWrite0.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        skyTexWrite0.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        skyTexWrite0.dstArrayElement = 0;
        skyTexWrite0.dstSet = uniformSet[i];
        skyTexWrite0.dstBinding = 3;
        VkDescriptorImageInfo skyTexImage0Info{};
        skyTexImage0Info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        skyTexImage0Info.imageView = skyImageView;
        skyTexImage0Info.sampler = standSampler;
        skyTexWrite0.pImageInfo = &skyTexImage0Info;
        devFuncs->vkUpdateDescriptorSets(window->device(), 1, &skyTexWrite0, 0, nullptr);
    }
}

void Render::buildSkyPipeline() {
    VkPipelineLayoutCreateInfo layoutCreateInfo{};
    layoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutCreateInfo.setLayoutCount = 1;

    VkDescriptorSetLayoutCreateInfo setLayoutInfo{};
    setLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    setLayoutInfo.bindingCount = 4;
    VkDescriptorSetLayoutBinding uniBinding{};
    uniBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uniBinding.binding = 0;
    uniBinding.descriptorCount = 1;
    uniBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    VkDescriptorSetLayoutBinding texBinding0{};  // 反射率贴图
    texBinding0.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    texBinding0.binding = 1;
    texBinding0.descriptorCount = 1;
    texBinding0.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    VkDescriptorSetLayoutBinding pbrBinding{};  // PBR纹理
    pbrBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    pbrBinding.binding = 2;
    pbrBinding.descriptorCount = 1;
    pbrBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT;
    VkDescriptorSetLayoutBinding skyBinding{};
    skyBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    skyBinding.binding = 3;
    skyBinding.descriptorCount = 1;
    skyBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutBinding bindings[] = { uniBinding, texBinding0, pbrBinding, skyBinding };

    setLayoutInfo.pBindings = bindings;
    VkDescriptorSetLayout setLayout;
    VK(devFuncs->vkCreateDescriptorSetLayout(window->device(), &setLayoutInfo, VDFT, &setLayout));

    layoutCreateInfo.pSetLayouts = &setLayout;
    VK(devFuncs->vkCreatePipelineLayout(window->device(), &layoutCreateInfo, VDFT, &skyBoxPipelineLayout));

    VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.layout = skyBoxPipelineLayout;
    pipelineCreateInfo.subpass = 0;
    pipelineCreateInfo.renderPass = window->defaultRenderPass();

    VkPipelineColorBlendStateCreateInfo colorBlend{};
    colorBlend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlend.attachmentCount = 1;

    VkPipelineColorBlendAttachmentState colorAttachment{};
    colorAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    colorAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorAttachment.blendEnable = VK_FALSE;
    colorAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                     VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    colorBlend.pAttachments = &colorAttachment;
    pipelineCreateInfo.pColorBlendState = &colorBlend;

    VkPipelineDepthStencilStateCreateInfo depthInfo{};
    depthInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    depthInfo.depthTestEnable = VK_FALSE;
    depthInfo.depthWriteEnable = VK_FALSE;
    depthInfo.depthBoundsTestEnable = VK_FALSE;
    depthInfo.maxDepthBounds = 1.0f;
    depthInfo.minDepthBounds = 0.0f;
    depthInfo.stencilTestEnable = VK_FALSE;
    pipelineCreateInfo.pDepthStencilState = &depthInfo;

    VkPipelineDynamicStateCreateInfo dynamicInfo{};
    dynamicInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicInfo.dynamicStateCount = 2;
    VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    dynamicInfo.pDynamicStates = dynamicStates;
    pipelineCreateInfo.pDynamicState = &dynamicInfo;

    VkPipelineInputAssemblyStateCreateInfo inputInfo{};
    inputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputInfo.primitiveRestartEnable = VK_FALSE;
    inputInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
    pipelineCreateInfo.pInputAssemblyState = &inputInfo;

    VkPipelineMultisampleStateCreateInfo aaInfo{};
    aaInfo.alphaToCoverageEnable = VK_TRUE;
    VkSampleMask aaMasks = ~0x0;
    aaInfo.pSampleMask = &aaMasks;
    aaInfo.rasterizationSamples = window->sampleCountFlagBits();
    aaInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    pipelineCreateInfo.pMultisampleState = &aaInfo;

    VkPipelineRasterizationStateCreateInfo rasterization{};
    rasterization.cullMode = VK_CULL_MODE_NONE;
    rasterization.polygonMode = VK_POLYGON_MODE_FILL;
    rasterization.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    pipelineCreateInfo.pRasterizationState = &rasterization;

    VkPipelineVertexInputStateCreateInfo vertexInfo{};
    vertexInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInfo.vertexAttributeDescriptionCount = 1;
    VkVertexInputAttributeDescription attrDescription{};
    attrDescription.binding = 0;
    attrDescription.format = VK_FORMAT_R32G32_SFLOAT;
    attrDescription.location = 0;
    attrDescription.offset = 0;
    vertexInfo.pVertexAttributeDescriptions = &attrDescription;
    vertexInfo.vertexBindingDescriptionCount = 1;
    VkVertexInputBindingDescription vertexBindingDescription{};
    vertexBindingDescription.binding = 0;
    vertexBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    vertexBindingDescription.stride = sizeof(glm::vec2);
    vertexInfo.pVertexBindingDescriptions = &vertexBindingDescription;
    pipelineCreateInfo.pVertexInputState = &vertexInfo;

    VkPipelineShaderStageCreateInfo shaderStage[2]{};
    shaderStage[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStage[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStage[0].module = skyVertexShader;
    shaderStage[0].pName = "main";
    shaderStage[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStage[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStage[1].module = skyFragmentShader;
    shaderStage[1].pName = "main";

    pipelineCreateInfo.pStages = shaderStage;
    pipelineCreateInfo.stageCount = 2;

    VK(devFuncs->vkCreateGraphicsPipelines(window->device(), VK_NULL_HANDLE, 1, &pipelineCreateInfo, VDFT, &skyBoxPipeline));
}

void Render::loadShader() {
    QFile vertFile0(":/assets/shaders/standard.vert.spv");
    vertFile0.open(QFile::ReadOnly);
    VkShaderModuleCreateInfo vertexShaderModuleCreateInfo{};
    vertexShaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    QByteArray vertCode = vertFile0.readAll();
    vertexShaderModuleCreateInfo.codeSize = vertCode.size();
    vertexShaderModuleCreateInfo.pCode = reinterpret_cast<uint32_t*>(vertCode.data());
    VK(devFuncs->vkCreateShaderModule(window->device(), &vertexShaderModuleCreateInfo, VDFT, &standardVertexShader));

    QFile fragFile0(":/assets/shaders/standard.frag.spv");
    fragFile0.open(QFile::ReadOnly);
    VkShaderModuleCreateInfo fragmentShaderModuleCreateInfo{};
    fragmentShaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    QByteArray fragCode = fragFile0.readAll();
    fragmentShaderModuleCreateInfo.codeSize = fragCode.size();
    fragmentShaderModuleCreateInfo.pCode = reinterpret_cast<uint32_t*>(fragCode.data());
    VK(devFuncs->vkCreateShaderModule(window->device(), &fragmentShaderModuleCreateInfo, VDFT, &standardFragmentShader));

    QFile envVF(":/assets/shaders/env.vert.spv");
    envVF.open(QFile::ReadOnly);
    VkShaderModuleCreateInfo envVertexCreateInfo{};
    envVertexCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    QByteArray envVCode = envVF.readAll();
    envVertexCreateInfo.codeSize = envVCode.size();
    envVertexCreateInfo.pCode = reinterpret_cast<uint32_t*>(envVCode.data());
    VK(devFuncs->vkCreateShaderModule(window->device(), &envVertexCreateInfo, VDFT, &skyVertexShader));

    QFile envFF(":/assets/shaders/env.frag.spv");
    envFF.open(QFile::ReadOnly);
    VkShaderModuleCreateInfo envFragmentCreateInfo{};
    envFragmentCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    QByteArray envFCode = envFF.readAll();
    envFragmentCreateInfo.codeSize = envFCode.size();
    envFragmentCreateInfo.pCode = reinterpret_cast<uint32_t*>(envFCode.data());
    VK(devFuncs->vkCreateShaderModule(window->device(), &envFragmentCreateInfo, VDFT, &skyFragmentShader));
}

void Render::createBasicRenderPass() {
    VkRenderPassCreateInfo passCreateInfo{};
    passCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    VkAttachmentDescription attachmentDescription{};
    attachmentDescription.format = window->colorFormat();
    attachmentDescription.samples = VK_SAMPLE_COUNT_4_BIT;
    attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    passCreateInfo.attachmentCount = 1;
    passCreateInfo.pAttachments = &attachmentDescription;
    VkSubpassDescription basicSubpassDescription{};
    basicSubpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    VkAttachmentReference colorReference{};
    colorReference.attachment = 0;
    colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    basicSubpassDescription.colorAttachmentCount = 1;
    basicSubpassDescription.pColorAttachments = &colorReference;
    passCreateInfo.subpassCount = 1;
    passCreateInfo.pSubpasses = &basicSubpassDescription;
    passCreateInfo.dependencyCount = 1; // For multi subpass
    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    passCreateInfo.pDependencies = &dependency; // For multi subpass

    VK(devFuncs->vkCreateRenderPass(window->device(), &passCreateInfo, VDFT, &basicPass));
}

void Render::createSampler() {
    VkSamplerCreateInfo createInfo{};
    createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    createInfo.anisotropyEnable = VK_TRUE;
    VkPhysicalDeviceFeatures deviceFeatures;
    functions->vkGetPhysicalDeviceFeatures(window->physicalDevice(), &deviceFeatures);
    createInfo.maxAnisotropy = deviceFeatures.samplerAnisotropy;
    createInfo.compareEnable = VK_FALSE;
    createInfo.magFilter = VK_FILTER_LINEAR;
    createInfo.minFilter = VK_FILTER_LINEAR;
    createInfo.maxLod = 20.0f;
    createInfo.minLod = 0.0;
    createInfo.mipLodBias = 0.0f;
    createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

    VK(devFuncs->vkCreateSampler(window->device(), &createInfo, VDFT, &standSampler));
}

void Render::createImageView(VkImage image, VkImageAspectFlags aspect, VkFormat format, uint32_t mipsLevel, VkImageView& view,
                             uint32_t layer, uint32_t layerCount) {
    VkImageViewCreateInfo viewInfo{};
    viewInfo.image = image;
    viewInfo.viewType = layerCount > 1 ? VK_IMAGE_VIEW_TYPE_CUBE : VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = aspect;
    viewInfo.subresourceRange.baseArrayLayer = layer;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.layerCount = layerCount;
    viewInfo.subresourceRange.levelCount = mipsLevel;
    VK(devFuncs->vkCreateImageView(window->device(), &viewInfo, VDFT, &view));
}

void Render::genMipmaps(VkImage image, VkExtent2D size, uint32_t mipsLevel, uint32_t layer) {
    VkCommandBuffer cmdBuf;
    VkCommandBufferAllocateInfo cmdBufAllocateInfo{};
    cmdBufAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdBufAllocateInfo.commandBufferCount = 1;
    cmdBufAllocateInfo.commandPool = window->graphicsCommandPool();
    cmdBufAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    VK(devFuncs->vkAllocateCommandBuffers(window->device(), &cmdBufAllocateInfo, &cmdBuf));

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    devFuncs->vkBeginCommandBuffer(cmdBuf, &beginInfo);

    VkImageMemoryBarrier makeImageAllLevelToDst{};
    makeImageAllLevelToDst.image = image;
    makeImageAllLevelToDst.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    makeImageAllLevelToDst.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    makeImageAllLevelToDst.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    makeImageAllLevelToDst.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    makeImageAllLevelToDst.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    makeImageAllLevelToDst.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    makeImageAllLevelToDst.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    makeImageAllLevelToDst.subresourceRange.baseMipLevel = 0;
    makeImageAllLevelToDst.subresourceRange.baseArrayLayer = layer;
    makeImageAllLevelToDst.subresourceRange.levelCount = mipsLevel;
    makeImageAllLevelToDst.subresourceRange.layerCount = 1;
    makeImageAllLevelToDst.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    devFuncs->vkCmdPipelineBarrier(cmdBuf, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr,
                                   1, &makeImageAllLevelToDst);

    VkOffset2D mipSize = { static_cast<int32_t>(size.width), static_cast<int32_t>(size.height) };
    for (uint32_t i = 1; i <= mipsLevel; i++) {
        uint32_t level = i < mipsLevel ? i : mipsLevel - 1;
        VkImageMemoryBarrier makeImageIndexIm1ToSrc{};
        makeImageIndexIm1ToSrc.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        makeImageIndexIm1ToSrc.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        makeImageIndexIm1ToSrc.image = image;
        makeImageIndexIm1ToSrc.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        makeImageIndexIm1ToSrc.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        makeImageIndexIm1ToSrc.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        makeImageIndexIm1ToSrc.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        makeImageIndexIm1ToSrc.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        makeImageIndexIm1ToSrc.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        makeImageIndexIm1ToSrc.subresourceRange.baseArrayLayer = layer;
        makeImageIndexIm1ToSrc.subresourceRange.baseMipLevel = level - 1;
        makeImageIndexIm1ToSrc.subresourceRange.layerCount = 1;
        makeImageIndexIm1ToSrc.subresourceRange.levelCount = 1;
        devFuncs->vkCmdPipelineBarrier(cmdBuf, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr,
                                       1, &makeImageIndexIm1ToSrc);

        VkImageBlit blit{};
        blit.srcOffsets[0] = { 0, 0, 0 };
        blit.srcOffsets[1] = { mipSize.x, mipSize.y, 1 };
        mipSize = { mipSize.x / 2 > 0 ? mipSize.x / 2 : 1, mipSize.y / 2 > 0 ? mipSize.y / 2 : 1 };
        blit.dstOffsets[0] = { 0, 0, 0 };
        blit.dstOffsets[1] = { mipSize.x, mipSize.y, 1 };
        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.baseArrayLayer = layer;
        blit.srcSubresource.layerCount = 1;
        blit.srcSubresource.mipLevel = level - 1;
        blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.baseArrayLayer = layer;
        blit.dstSubresource.layerCount = 1;
        blit.dstSubresource.mipLevel = level;
        devFuncs->vkCmdBlitImage(cmdBuf, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                 1, &blit, VK_FILTER_LINEAR);

        VkImageMemoryBarrier makeImageIndexIm1ToShader{};
        makeImageIndexIm1ToShader.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        makeImageIndexIm1ToShader.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        makeImageIndexIm1ToShader.image = image;
        makeImageIndexIm1ToShader.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        makeImageIndexIm1ToShader.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        makeImageIndexIm1ToShader.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        makeImageIndexIm1ToShader.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        makeImageIndexIm1ToShader.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        makeImageIndexIm1ToShader.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        makeImageIndexIm1ToShader.subresourceRange.baseArrayLayer = layer;
        makeImageIndexIm1ToShader.subresourceRange.baseMipLevel = level - 1;
        makeImageIndexIm1ToShader.subresourceRange.levelCount = 1;
        makeImageIndexIm1ToShader.subresourceRange.layerCount = 1;
        devFuncs->vkCmdPipelineBarrier(cmdBuf, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, 0, 0, nullptr, 0, nullptr,
                                       1, &makeImageIndexIm1ToShader);
    }

    devFuncs->vkEndCommandBuffer(cmdBuf);

    VkSubmitInfo submitInfo{};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmdBuf;
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    devFuncs->vkQueueSubmit(window->graphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
    devFuncs->vkQueueWaitIdle(window->graphicsQueue());

    devFuncs->vkFreeCommandBuffers(window->device(), window->graphicsCommandPool(), 1, &cmdBuf);
}

ViewThread::ViewThread(SceneView* area, uint32_t delta): area(area), delta(delta) {
    runable = true;
}

void ViewThread::run() {
    while (runable) {
        emit area->updateSignal();
        msleep(delta);
    }
}
