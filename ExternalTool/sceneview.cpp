#include "sceneview.h"
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm/glm.hpp>
#include <glm/glm/gtc/matrix_transform.hpp>

std::mutex renderLoopLock;

SceneView::SceneView(std::mutex* lock, QVulkanWindow *parent, const DataContext* dataContext,
                     QVulkanInstance* instance, const bool topView)
    : QVulkanWindow{parent},
      thread(this, 20), dataContext(dataContext), instance(instance), topView(topView), lock(lock) {
    setVulkanInstance(instance);
    setSampleCount(1);
    setCursor(Qt::CrossCursor);
    connect(this, &SceneView::updateSignal, this, &SceneView::updateSLot);
    setPreferredColorFormats({ VK_FORMAT_R8G8B8A8_UNORM });
}

SceneView::~SceneView() {
    thread.runable = false;
    thread.wait();
}

QVulkanWindowRenderer* SceneView::createRenderer() {
    render = new Render(this, topView, lock);
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
    renderLoopLock.lock();
    lock->lock();
    render->camera.lerp(vcamera, 0.3f);

    for (int i = 0; i < concurrentFrameCount() && render->ready(); i++) {
        auto funi = UNI(UniFrame, render->frameUniform.data, i, 1, render->uniformSize, 0);

        funi->time = static_cast<float>(clock() / 1000.0f);

        funi->resolution = glm::vec4(static_cast<float>(swapChainImageSize().width()),
                                     static_cast<float>(swapChainImageSize().height()),
                                     1.0f, 1.0f);

        funi->etc.x = render->renderType;

        funi->v = render->camera.getV();
        funi->p = render->camera.getP(glm::vec2(static_cast<float>(swapChainImageSize().width()),
                                       static_cast<float>(swapChainImageSize().height())));

        funi->sun = glm::vec4(1.0f, 0.93f, 0.8f, 1.59f);
        funi->sunDir = -glm::normalize(glm::vec3(1.0, 0.2, 1.0));

        funi->fog = { 0.4f, 0.2f };
        funi->fogColor = { 0.8f, 0.75f, 0.7f };

        funi->shadowBias = glm::vec2(0.005f, 1.0f / SHADOW_SIZE);

        for (int j = 0; j < OBJECTS_MAX_COUNT; j++) {
            auto objuni = UNI(UniObject, render->objectsUniform.data, i, OBJECTS_MAX_COUNT, render->uniformSize, j);
            objuni->baseColor = { 1.0f, 1.0f, 1.0f, 1.0f };
            objuni->pbrBase = glm::vec4(render->testPBRBase.roughness, render->testPBRBase.specular,
                                          render->testPBRBase.metallic, render->testPBRBase.contrast);
            objuni->ssbase = render->testPBRBase.ss;
            Transform t{};
            t.position = (j != 5 + 1) ? glm::vec3{ 0.0f, 0.0f, 0.0f } :
                                        glm::vec3{ glm::cos(funi->time), glm::sin(funi->time), 0.0f } * 0.001f;
            t.rotation = { 0.0f, 0.0f, 0.0f };
            t.scale = { 1.0f, 1.0f, 1.0f };
            objuni->m = Model::packTransform(t, 1.0f).offsetRotate;
            objuni->pbrRSMC = glm::vec4(render->testPBRInfo.roughness, render->testPBRInfo.specular,
                                          render->testPBRInfo.metallic, render->testPBRInfo.contrast);
            objuni->scale = Model::packTransform(t, 1.0f).scale;
            objuni->subsurface = render->testPBRInfo.ss;
            objuni->mvp = funi->p * funi->v * objuni->m;
            objuni->clip = 0.6;
        }

        for (int j = 0; j < 4; j++) {
            auto suni = UNI(UniShadow, render->shadowUniform.data, i, 4, render->uniformSize, j);
            suni->shadowV = render->camera.getShadowV(funi->sunDir, 4.0f, j, -4.0f, 4.0f);
        }
    }

    lock->unlock();
    renderLoopLock.unlock();
}

void Render::textureInit() {
    loadImage(testAlbedo, ":/assets/textures/vase.png", SRGB, { 2048, 2048 });
    loadImage(testPBR, ":/assets/textures/standardPBR.png", LINEAR, { 2048, 2048 });
    loadImage(testNormal, ":/assets/textures/standardNormal.bmp", LINEAR, { 2048, 2048 });

    const char* skyPath[6] = {
        ":/assets/textures/env/test/px.png",
        ":/assets/textures/env/test/nx.png",
        ":/assets/textures/env/test/py.png",
        ":/assets/textures/env/test/ny.png",
        ":/assets/textures/env/test/pz.png",
        ":/assets/textures/env/test/nz.png"
    };
    loadCubemap(skyCubemap, skyPath, SRGB);

    std::string plantsTexPath[] = {
        ":/r/plants/larborLeaf.png",
        ":/r/plants/arborLeaf.png",
        ":/r/plants/lshrub.png",
        ":/r/plants/shrubs.png",
        ":/r/plants/flowers.png",
        ":/r/plants/grassx.png",
        ":/r/plants/climber.png",
        ":/r/plants/stone.png"
    };

    std::string plantsNPath[] = {
        ":/r/plants/larborN.png",
        ":/r/plants/arborN.png",
        ":/r/plants/lshrubN.png",
        ":/r/plants/shrubN.png",
        ":/r/plants/flowersN.png",
        ":/r/plants/grassN.png",
        ":/r/plants/climberN.png",
        ":/r/plants/stoneN.png"
    };

    for (size_t i = 0; i < PLANT_TYPE_NUM; i++) {
        loadImage(plantsAlbedo[i], plantsTexPath[i].data(), SRGB);
        loadImage(plantsNormal[i], plantsNPath[i].data(), LINEAR);
        loadImage(plantsPBR[i], ":/assets/textures/standardPBR.png", LINEAR);
    }

    const VkFormat shadow_format = SHADOW_FORMAT;
    VkExtent2D shadowMapSize = { SHADOW_SIZE, SHADOW_SIZE };

    createImage(shadowMapSize, 1, shadow_format, notuse, shadowMap.image,
                VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, shadowMap.memory,
                shadowMap.requirement, 0, 4, false);
    makeDepthBuffer(shadowMap.image);
    createImageView(shadowMap.image, VK_IMAGE_ASPECT_DEPTH_BIT, shadow_format, 1,
                    shadowMap.view, 0, 4, true);
    for (int i = 0; i < 4; i++) {
        createImageView(shadowMap.image, VK_IMAGE_ASPECT_DEPTH_BIT, shadow_format, 1,
                        shadowMap.multiView[i], i, 1, true);
        VkFramebufferCreateInfo shadowFrameInfo{};
        shadowFrameInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        shadowFrameInfo.attachmentCount = 1;
        shadowFrameInfo.pAttachments = &shadowMap.multiView[i];
        shadowFrameInfo.width = SHADOW_SIZE;
        shadowFrameInfo.height = SHADOW_SIZE;
        shadowFrameInfo.layers = 1;
        shadowFrameInfo.renderPass = shadowPass;
        devFuncs->vkCreateFramebuffer(window->device(), &shadowFrameInfo, VDFT, &shadowFrameBuffer[i]);
    }

}

void Render::vertexInit() {
    loadVertex(terrainVertex, MAX_HILL_VSIZE * VertexSize);

    loadIndex(terrainIndex, MAX_HILL_I_SIZE * IndexSize);

    for (uint32_t i = 0; i < PLANT_TYPE_NUM; i++) {
        loadVertex(plantsVertex[i], MAX_PLANT_VSIZE * VertexSize);

        loadVertex(plantsIndex[i], MAX_PLANT_I_SIZE * IndexSize);
    }

    loadVertex(skyVertex, sizeof(glm::vec2) * 4);
    glm::vec2 skyPoints[4] = {
        { -1.0f, -1.0f }, { -1.0f, 1.0f }, { 1.0f, 1.0f }, { 1.0f, -1.0f }
    };
    skyVertex.count = 4;
    fillBuffer(4 * sizeof(glm::vec2), skyPoints, skyVertex, 0);
}

void Render::uniformInit() {
    loadUniform(frameUniform, uniformSize(sizeof(frameUniform)) * window->concurrentFrameCount(), false);
    loadUniform(objectsUniform, uniformSize(sizeof(objectsUniform)) * OBJECTS_MAX_COUNT *
                window->concurrentFrameCount(), true);
    loadUniform(shadowUniform, uniformSize(sizeof(shadowUniform)) * 4 * window->concurrentFrameCount(), true);

}

void Render::instanceInit() {

    loadInstance(testInstance, MAX_PLANT_COUNT * InstanceSize);

    for (uint32_t i = 0; i < PLANT_TYPE_NUM; i++) {
        loadInstance(plantsInstance[i], MAX_PLANT_COUNT * InstanceSize);
        plantsInstance[i].count = 32;
        std::vector<MMat> pidata;
        for (uint32_t ii = 0; ii < plantsInstance[i].count; ii++) {
            Transform t{};
            t.position = glm::vec3(glm::sphericalRand(3.5f));
            t.rotation = glm::vec3(glm::linearRand(0.0f, 0.0f),
                                   glm::linearRand(0.0f, 0.0f),
                                   glm::linearRand(0.0f, 6.28f));
            t.scale = glm::vec3(glm::linearRand(2.7f, 3.4f));
            pidata.push_back(Model::packTransform(t, 0.5f));
        }
        fillLocalData(plantsInstance[i], pidata.data(), pidata.size() * InstanceSize, 0);
    }

    testInstance.count = 1;
    std::vector<MMat> idata;
    for (uint32_t i = 0; i < testInstance.count; i++) {
        Transform t{};
        t.position = glm::vec3(0.0); // glm::vec3(glm::sphericalRand(3.5f));
        t.rotation = glm::vec3(0.0); // glm::vec3(glm::linearRand(0.0, 6.28), glm::linearRand(0.0, 6.28), glm::linearRand(0.0, 6.28));
        t.scale = glm::vec3(1.0f);
        idata.push_back(Model::packTransform(t, 1.0f));
    }

    fillLocalData(testInstance, idata.data(), idata.size() * InstanceSize, 0);
}

Render::Render(QVulkanWindow* w, const bool topView, std::mutex* lock): topView(topView), window(w), lock(lock) {
    camera.position = glm::vec3(5.0, 5.0, 3.0f);
    camera.lookAt(glm::vec3(0.0f, 0.0f, 0.0f));

    Model::loadFromOBJFile(":/assets/models/cube-repeat-tex.obj", monkeyModel, monkeyModelIndex, false);
    Model::loadFromOBJFile(":/r/plants/larbor.obj", plantsModel[0], plantsModelIndex[0]);
    Model::loadFromOBJFile(":/r/plants/arbor.obj", plantsModel[1], plantsModelIndex[1]);
    Model::loadFromOBJFile(":/r/plants/lshrubs.obj", plantsModel[2], plantsModelIndex[2]);
    Model::loadFromOBJFile(":/r/plants/shrubs.obj", plantsModel[3], plantsModelIndex[3]);
    Model::loadFromOBJFile(":/r/plants/flowers.obj", plantsModel[4], plantsModelIndex[4]);
    Model::loadFromOBJFile(":/r/plants/grass.obj", plantsModel[5], plantsModelIndex[5]);
    Model::loadFromOBJFile(":/r/plants/climber.obj", plantsModel[6], plantsModelIndex[6]);
    Model::loadFromOBJFile(":/r/plants/stone.obj", plantsModel[7], plantsModelIndex[7]);

}

void Render::initResources() {
    lock->lock();
    printf("Use color space %u\n", static_cast<uint32_t>(window->colorFormat()));

    devFuncs = window->vulkanInstance()->deviceFunctions(window->device());
    functions = window->vulkanInstance()->functions();

    createBasicRenderPass();

    createMaxImage(); // to get max image memory size
    fillBufferInit(); // create stage buffer
    fillImageInit();  // create stage image buffer
    createSampler();  // create image standard sampler

    uniformSet = new VkDescriptorSet[window->concurrentFrameCount()];
    // allocate uniform descriptor sets pointers.

    uniformInit();

    loadShader();

    vertexInit();

    textureInit();

    instanceInit();

    pushHill(monkeyModel, monkeyModelIndex);
    for (uint32_t i = 0; i < PLANT_TYPE_NUM; i++) {
        setPlant(plantsModel[i], plantsModelIndex[i], i);
    }

    buildPipeline();
    buildSkyPipeline();
    buildShadowPipeline();

    if (window->swapChainImageSize().width() > 0 && window->swapChainImageSize().height() > 0)
    {
        resetup = true;
    }

    readyToRender = true;
    lock->unlock();
}

void Render::initSwapChainResources() {
    QVulkanWindowRenderer::initSwapChainResources();
}

void Render::releaseSwapChainResources() {
    QVulkanWindowRenderer::releaseSwapChainResources();
}

void Render::releaseResources() {
    lock->lock();
    readyToRender = false;

    VkDevice dev = window->device();

    destroyBuffer(terrainVertex);
    destroyBuffer(terrainIndex);

    for (uint32_t i = 0; i < PLANT_TYPE_NUM; i++) {
        // devFuncs->vkFreeMemory(dev, plantsMemories[i], VDFT);
        // devFuncs->vkFreeMemory(dev, plantsIndexMemories[i], VDFT);
        // devFuncs->vkDestroyBuffer(dev, plantsVertexBuffers[i], VDFT);
        // devFuncs->vkDestroyBuffer(dev, plantsIndexBuffers[i], VDFT);
    }

    // devFuncs->vkFreeMemory(dev, uniformMemory, VDFT);
    // devFuncs->vkDestroyBuffer(dev, uniform, VDFT);
    destroyBuffer(frameUniform);
    destroyBuffer(objectsUniform);
    destroyBuffer(shadowUniform);

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

    destroyImage(testAlbedo);

    devFuncs->vkDestroySampler(dev, standSampler, VDFT);

    destroyBuffer(testInstance);

    destroyBuffer(skyVertex);

    destroyImage(skyCubemap);

    for (size_t i = 0; i < 8; i++) {
        destroyImage(plantsAlbedo[i]);
    }

    resetup = false;
    lock->unlock();
}

void Render::logicalDeviceLost() {
    perror("Device lost!!\n");
    QVulkanWindowRenderer::logicalDeviceLost();
}

void Render::physicalDeviceLost() {
    perror("Physical device lost!!\n");
    QVulkanWindowRenderer::physicalDeviceLost();
}

void Render::preInitResources() {

}

void Render::startNextFrame() {
    lock->lock();
    renderLoopLock.lock();

    green += 0.005f;
    if (green > 1.0f)
        green = 0.0f;

    VkClearDepthStencilValue clearDS = { 1.0f, 0 };

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
        U32(window->swapChainImageSize().width()),
        U32(window->swapChainImageSize().height())
    };

    VkDeviceSize offsets[] = { 0 };

    uint32_t dynamicBinding[2] = { 0, 0 };

    albedoView = testAlbedo.view; PBRView = testPBR.view; normalView = testNormal.view;
    skyView = skyCubemap.view;
    updateUniforms(false, window->currentFrame());

    // ########### shadow pass #########################
    for (int i = 0; i < 4; i++) {
        VkRenderPassBeginInfo sdBeginInfo{};
        sdBeginInfo.renderArea.extent.width = SHADOW_SIZE;
        sdBeginInfo.renderArea.extent.height = SHADOW_SIZE;
        sdBeginInfo.renderArea.offset = { 0, 0 };
        sdBeginInfo.renderPass = shadowPass;
        sdBeginInfo.clearValueCount = 1;
        sdBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        sdBeginInfo.framebuffer = shadowFrameBuffer[i];
        VkClearValue sdClear{};
        VkClearDepthStencilValue sdClearDS = { 1.0f, 0 };
        sdClear.depthStencil = sdClearDS;
        sdBeginInfo.pClearValues = &sdClear;

        VkViewport sdViewport{};
        sdViewport.height = SHADOW_SIZE;
        sdViewport.width = SHADOW_SIZE;
        sdViewport.x = 0;
        sdViewport.y = 0;
        sdViewport.minDepth = 0.0f;
        sdViewport.maxDepth = 1.0f;

        VkRect2D sdDcissors{};
        sdDcissors.offset = { 0, 0 };
        sdDcissors.extent = { SHADOW_SIZE, SHADOW_SIZE };

        devFuncs->vkCmdSetViewport(cmdBuf, 0, 1, &sdViewport);
        devFuncs->vkCmdSetScissor(cmdBuf, 0, 1, &sdDcissors);

        dynamicBinding[1] = i * uniformSize(sizeof(UniShadow));
        devFuncs->vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, shadowPipelineLayout, 0, 1,
                                          &uniformSet[window->currentFrame()], 2, dynamicBinding);

        devFuncs->vkCmdBeginRenderPass(cmdBuf, &sdBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

        devFuncs->vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, shadowPipeline);

        drawObjects(cmdBuf, dynamicBinding);

        devFuncs->vkCmdEndRenderPass(cmdBuf);
    }

    // ########### shadow pass #########################

    VkClearColorValue clearColor = {{ green, green, 0.0f, 1.0f }};

    VkClearValue clearValues[3] = {};
    clearValues[0].color = clearColor;
    clearValues[1].depthStencil = clearDS;
    glm::vec3 env = glm::vec3(0.25f, 0.4f, 0.65f);
    env = glm::pow(env, glm::vec3(1.0f / 2.2f));
    clearValues[2].color = {{ env.r, env.g, env.b }};

    dynamicBinding[1] = 0 * uniformSize(sizeof(UniShadow));

    VkRenderPassBeginInfo rpBeginInfo{};
    rpBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpBeginInfo.renderPass = window->defaultRenderPass();
    rpBeginInfo.framebuffer = window->currentFramebuffer();
    const QSize sz = window->swapChainImageSize();
    rpBeginInfo.renderArea.extent.width = sz.width();
    rpBeginInfo.renderArea.extent.height = sz.height();
    rpBeginInfo.clearValueCount = window->sampleCountFlagBits() > VK_SAMPLE_COUNT_1_BIT ? 3 : 2;
    rpBeginInfo.pClearValues = clearValues;

    devFuncs->vkCmdBeginRenderPass(cmdBuf, &rpBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    devFuncs->vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, skyBoxPipeline);

    devFuncs->vkCmdSetViewport(cmdBuf, 0, 1, &viewport);
    devFuncs->vkCmdSetScissor(cmdBuf, 0, 1, &scissor);

    albedoView = testAlbedo.view; PBRView = testPBR.view; normalView = testNormal.view;
    skyView = skyCubemap.view;
    updateUniforms(false, window->currentFrame());

    devFuncs->vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, skyBoxPipelineLayout, 0, 1,
                                      &uniformSet[window->currentFrame()], 2, dynamicBinding);

    offsets[0] = 0;
    devFuncs->vkCmdBindVertexBuffers(cmdBuf, 0, 1, &skyVertex.buffer, offsets);

    devFuncs->vkCmdDraw(cmdBuf, skyVertex.count, 1, 0, 0);

    devFuncs->vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, standardPipeline);

    devFuncs->vkCmdSetViewport(cmdBuf, 0, 1, &viewport);
    devFuncs->vkCmdSetScissor(cmdBuf, 0, 1, &scissor);

    drawObjects(cmdBuf, dynamicBinding);

    devFuncs->vkCmdEndRenderPass(cmdBuf);

    window->frameReady();
    window->requestUpdate(); // render continuously, throttled by the presentation rate

    renderLoopLock.unlock();
    lock->unlock();

}

void Render::pushHill(const Vertexs& vs, const Indexs& index) {
    fillBuffer(vs.size() * VertexSize, vs.data(), terrainVertex, 0);
    terrainVertex.count = vs.size();
    fillBuffer(index.size() * IndexSize, index.data(), terrainIndex, 0);
    terrainIndex.count = index.size();
}

void Render::setPlant(const Vertexs& vs, const Indexs& index, uint32_t id) {
    fillBuffer(vs.size() * VertexSize, vs.data(), plantsVertex[id], 0);
    plantsVertex[id].count = vs.size();
    fillBuffer(index.size() * IndexSize, index.data(), plantsIndex[id], 0);
    plantsIndex[id].count = index.size();
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

void Render::createImage(VkExtent2D extent, uint32_t mipsLevel, VkFormat format, void*& data, VkImage& image,
                         VkImageUsageFlags usage, VkDeviceMemory& memory, VkMemoryRequirements& memoryRequirement,
                         VkImageCreateFlags flags, uint32_t layerCount, bool local, VkExtent2D size) {
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

    if (size.width == 0 && size.height == 0) {
        devFuncs->vkGetImageMemoryRequirements(window->device(), layerCount > 1 ? maxCubeTexture : maxTexture,
                                               &memoryRequirement);
    } else {
        VkImage sizeTemp;
        VkImageCreateInfo imageCreateInfo{};
        imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
        imageCreateInfo.extent.width = size.width;
        imageCreateInfo.extent.height = size.height;
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
        VK(devFuncs->vkCreateImage(window->device(), &imageCreateInfo, VDFT, &sizeTemp));

        devFuncs->vkGetImageMemoryRequirements(window->device(), sizeTemp, &memoryRequirement);

        devFuncs->vkDestroyImage(window->device(), sizeTemp, VDFT);
    }

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

void Render::createImageOnly(VkExtent2D extent, uint32_t mipsLevel, VkFormat format, Texture2D& tex,
                             VkImageUsageFlags usage, VkImageCreateFlags flags, uint32_t layerCount, bool local) {
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
    VK(devFuncs->vkCreateImage(window->device(), &imageCreateInfo, VDFT, &tex.image));
    VK(devFuncs->vkBindImageMemory(window->device(), tex.image, tex.memory, 0));
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

void Render::makeDepthBuffer(VkImage image) {
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

    for (uint32_t i = 0; i < 4; i++) {
        VkImageMemoryBarrier makeImageDstBarrier{};
        makeImageDstBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        makeImageDstBarrier.image = image;
        makeImageDstBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        makeImageDstBarrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
        makeImageDstBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        makeImageDstBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        makeImageDstBarrier.srcAccessMask = VK_ACCESS_NONE;
        makeImageDstBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        makeImageDstBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        makeImageDstBarrier.subresourceRange.baseArrayLayer = i;
        makeImageDstBarrier.subresourceRange.baseMipLevel = 0;
        makeImageDstBarrier.subresourceRange.layerCount = 1;
        makeImageDstBarrier.subresourceRange.levelCount = 1;
        devFuncs->vkCmdPipelineBarrier(cmdBuf, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                       0,
                                       0, nullptr,
                                       0, nullptr,
                                       1, &makeImageDstBarrier);
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

void Render::fillBuffer(uint32_t size, const void* data, IBuffer buffer, VkDeviceSize offset) {
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
    devFuncs->vkCmdCopyBuffer(cmdBuf, tmpBuffer, buffer.buffer, 1, &copy);

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
        if (memoryProperties.memoryTypes[i].propertyFlags & (VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
                                                             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)) {
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

void Render::fillImage(uint32_t size, VkExtent2D extent, uint32_t mipsLevel, const void* data, Texture2D image,
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
    makeImageDstBarrier.image = image.image;
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
    devFuncs->vkCmdCopyBufferToImage(cmdBuf, tmpImage, image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);

    VkImageMemoryBarrier makeImageShaderUseBarrier{};
    makeImageShaderUseBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    makeImageShaderUseBarrier.image = image.image;
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
        if (memoryProperties.memoryTypes[i].propertyFlags & (VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
                                                             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)) {
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

    VkDescriptorSetLayoutBinding bindings[UNI_BINDING_COUNT];
    fillUnifromLayoutBindings(bindings);

    setLayoutCreateInfo.bindingCount = UNI_BINDING_COUNT;
    setLayoutCreateInfo.pBindings = bindings;
    VK(devFuncs->vkCreateDescriptorSetLayout(window->device(), &setLayoutCreateInfo, VDFT, &uniformSetLayout));

    pipelineLayoutCreateInfo.pSetLayouts = &uniformSetLayout;
    VK(devFuncs->vkCreatePipelineLayout(window->device(), &pipelineLayoutCreateInfo, VDFT, &standardPipelineLayout));

    VkDescriptorPoolCreateInfo poolCreateInfo{};

    VkDescriptorPoolSize uniFramePoolSize{};
    uniFramePoolSize.descriptorCount = window->concurrentFrameCount();
    uniFramePoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    VkDescriptorPoolSize uniObjPoolSize{};
    uniObjPoolSize.descriptorCount = window->concurrentFrameCount();
    uniObjPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    VkDescriptorPoolSize uniShadowPoolSize{};
    uniShadowPoolSize.descriptorCount = window->concurrentFrameCount();
    uniShadowPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    VkDescriptorPoolSize texPoolSize0{};
    texPoolSize0.descriptorCount = window->concurrentFrameCount();
    texPoolSize0.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    VkDescriptorPoolSize pbrPoolSize{};
    pbrPoolSize.descriptorCount = window->concurrentFrameCount();
    pbrPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    VkDescriptorPoolSize skyPoolSize{};
    skyPoolSize.descriptorCount = window->concurrentFrameCount();
    skyPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    VkDescriptorPoolSize normalPoolSize{};
    normalPoolSize.descriptorCount = window->concurrentFrameCount();
    normalPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    VkDescriptorPoolSize shadowPoolSize{};
    shadowPoolSize.descriptorCount = window->concurrentFrameCount();
    shadowPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    VkDescriptorPoolSize dynamicPoolSize{};
    dynamicPoolSize.descriptorCount = window->concurrentFrameCount();
    dynamicPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

    VkDescriptorPoolSize poolSizes[] = { uniFramePoolSize, uniObjPoolSize, uniShadowPoolSize,
                                         texPoolSize0, pbrPoolSize, skyPoolSize, normalPoolSize,
                                         shadowPoolSize, dynamicPoolSize };

    poolCreateInfo.poolSizeCount = 9;
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
    colorBlendState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendState.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendState.alphaBlendOp = VK_BLEND_OP_ADD;
    colorBlendState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                     VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendState.blendEnable = VK_FALSE;
    colorInfo.pAttachments = &colorBlendState;
    colorInfo.blendConstants[0] = 0.0f;
    colorInfo.blendConstants[1] = 0.0f;
    colorInfo.blendConstants[2] = 0.0f;
    colorInfo.blendConstants[3] = 0.0f;
    colorInfo.logicOpEnable = VK_FALSE;
    colorInfo.logicOp = VK_LOGIC_OP_COPY;

    VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo{};
    graphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    graphicsPipelineCreateInfo.layout = standardPipelineLayout;
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
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
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

    VK(devFuncs->vkCreateGraphicsPipelines(window->device(), VDFT, 1, &graphicsPipelineCreateInfo, VDFT,
                                           &standardPipeline));

    albedoView = testAlbedo.view;
    PBRView = testPBR.view;
    normalView = testNormal.view;
    skyView = skyCubemap.view;

    for (int i = 0; i < window->concurrentFrameCount(); i++) {
        updateUniforms(true, i);
    }
}

void Render::buildSkyPipeline() {
    VkPipelineLayoutCreateInfo layoutCreateInfo{};
    layoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutCreateInfo.setLayoutCount = 1;

    VkDescriptorSetLayoutCreateInfo setLayoutInfo{};
    setLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    setLayoutInfo.bindingCount = UNI_BINDING_COUNT;

    VkDescriptorSetLayoutBinding bindings[UNI_BINDING_COUNT];
    fillUnifromLayoutBindings(bindings);

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

    VK(devFuncs->vkCreateGraphicsPipelines(window->device(), VK_NULL_HANDLE, 1, &pipelineCreateInfo, VDFT,
                                           &skyBoxPipeline));
}

void Render::buildShadowPipeline() {
    VkPipelineLayoutCreateInfo layoutCreateInfo{};
    layoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutCreateInfo.setLayoutCount = 1;

    VkDescriptorSetLayoutCreateInfo setLayoutInfo{};
    setLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    setLayoutInfo.bindingCount = UNI_BINDING_COUNT;

    VkDescriptorSetLayoutBinding bindings[UNI_BINDING_COUNT];
    fillUnifromLayoutBindings(bindings);

    setLayoutInfo.pBindings = bindings;
    VkDescriptorSetLayout setLayout;
    VK(devFuncs->vkCreateDescriptorSetLayout(window->device(), &setLayoutInfo, VDFT, &setLayout));

    layoutCreateInfo.pSetLayouts = &setLayout;
    VK(devFuncs->vkCreatePipelineLayout(window->device(), &layoutCreateInfo, VDFT, &shadowPipelineLayout));

    VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.layout = shadowPipelineLayout;
    pipelineCreateInfo.subpass = 0;
    pipelineCreateInfo.renderPass = shadowPass;

    VkPipelineColorBlendStateCreateInfo colorBlend{};
    colorBlend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlend.attachmentCount = 0;

    /* VkPipelineColorBlendAttachmentState colorAttachment{};
    colorAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    colorAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorAttachment.blendEnable = VK_FALSE;
    colorAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                     VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    colorBlend.pAttachments = &colorAttachment; */
    pipelineCreateInfo.pColorBlendState = &colorBlend;

    VkPipelineDepthStencilStateCreateInfo depthInfo{};
    depthInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    depthInfo.depthTestEnable = VK_TRUE;
    depthInfo.depthWriteEnable = VK_TRUE;
    depthInfo.depthBoundsTestEnable = VK_FALSE;
    depthInfo.maxDepthBounds = 1.0f;
    depthInfo.minDepthBounds = 0.0f;
    depthInfo.stencilTestEnable = VK_FALSE;
    pipelineCreateInfo.pDepthStencilState = &depthInfo;

    VkPipelineDynamicStateCreateInfo dynamicInfo{};
    dynamicInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicInfo.dynamicStateCount = 1;
    VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    dynamicInfo.pDynamicStates = dynamicStates;
    pipelineCreateInfo.pDynamicState = &dynamicInfo;

    VkPipelineInputAssemblyStateCreateInfo inputInfo{};
    inputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputInfo.primitiveRestartEnable = VK_TRUE;
    inputInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
    pipelineCreateInfo.pInputAssemblyState = &inputInfo;

    VkPipelineMultisampleStateCreateInfo aaInfo{};
    VkSampleMask aaMasks = ~0x0;
    aaInfo.pSampleMask = &aaMasks;
    aaInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    aaInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    pipelineCreateInfo.pMultisampleState = &aaInfo;

    VkPipelineRasterizationStateCreateInfo rasterization{};
    rasterization.cullMode = VK_CULL_MODE_NONE;
    rasterization.polygonMode = VK_POLYGON_MODE_FILL;
    rasterization.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    pipelineCreateInfo.pRasterizationState = &rasterization;

    VkPipelineVertexInputStateCreateInfo vertexInfo{};
    vertexInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInfo.vertexAttributeDescriptionCount = Model::VERTEX_INPUT_COUNT + Model::INSTANCE_INPUT_COUNT;
    VkVertexInputAttributeDescription descriptions[Model::VERTEX_INPUT_COUNT + Model::INSTANCE_INPUT_COUNT];
    Model::vertexInputAttributeDescription(descriptions);
    Model::instanceInputAttributeDescription(descriptions + Model::VERTEX_INPUT_COUNT);
    vertexInfo.pVertexAttributeDescriptions = descriptions;

    VkVertexInputBindingDescription vertexBindingDescription{};
    Model::vertexInputBindingDescription(vertexBindingDescription);
    VkVertexInputBindingDescription instanceBindingDescription{};
    Model::instanceInputBindingDescription(instanceBindingDescription);
    VkVertexInputBindingDescription vBindings[] = { vertexBindingDescription, instanceBindingDescription };
    vertexInfo.pVertexBindingDescriptions = vBindings;
    vertexInfo.vertexBindingDescriptionCount = 2;
    pipelineCreateInfo.pVertexInputState = &vertexInfo;

    VkPipelineShaderStageCreateInfo shaderStage[2]{};
    shaderStage[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStage[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStage[0].module = shadowVertexShader;
    shaderStage[0].pName = "main";
    shaderStage[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStage[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStage[1].module = shadowFragmentShader;
    shaderStage[1].pName = "main";

    pipelineCreateInfo.pStages = shaderStage;
    pipelineCreateInfo.stageCount = 2;

    VkViewport viewport{};
    viewport.height = SHADOW_SIZE;
    viewport.width = SHADOW_SIZE;
    viewport.x = 0;
    viewport.y = 0;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissors{};
    scissors.offset = { 0, 0 };
    scissors.extent = { SHADOW_SIZE, SHADOW_SIZE };

    VkPipelineViewportStateCreateInfo viewportInfo{};
    viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportInfo.pViewports = &viewport;
    viewportInfo.viewportCount = 1;
    viewportInfo.pScissors = &scissors;
    viewportInfo.scissorCount = 1;

    pipelineCreateInfo.pViewportState = &viewportInfo;

    VK(devFuncs->vkCreateGraphicsPipelines(window->device(), VK_NULL_HANDLE, 1, &pipelineCreateInfo, VDFT,
                                           &shadowPipeline));
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
    VK(devFuncs->vkCreateShaderModule(window->device(), &fragmentShaderModuleCreateInfo, VDFT,
                                      &standardFragmentShader));

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

    QFile sdVF(":/assets/shaders/shadow.vert.spv");
    sdVF.open(QFile::ReadOnly);
    VkShaderModuleCreateInfo sdVertexCreateInfo{};
    sdVertexCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    QByteArray sdVCode = sdVF.readAll();
    sdVertexCreateInfo.codeSize = sdVCode.size();
    sdVertexCreateInfo.pCode = reinterpret_cast<uint32_t*>(sdVCode.data());
    VK(devFuncs->vkCreateShaderModule(window->device(), &sdVertexCreateInfo, VDFT, &shadowVertexShader));

    QFile sdFF(":/assets/shaders/shadow.frag.spv");
    sdFF.open(QFile::ReadOnly);
    VkShaderModuleCreateInfo sdFragmentCreateInfo{};
    sdFragmentCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    QByteArray sdFCode = sdFF.readAll();
    sdFragmentCreateInfo.codeSize = sdFCode.size();
    sdFragmentCreateInfo.pCode = reinterpret_cast<uint32_t*>(sdFCode.data());
    VK(devFuncs->vkCreateShaderModule(window->device(), &sdFragmentCreateInfo, VDFT, &shadowFragmentShader));
}

void Render::createBasicRenderPass() {
    VkRenderPassCreateInfo passCreateInfo{};
    passCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    VkAttachmentDescription attachmentDescription{};
    attachmentDescription.format = SHADOW_FORMAT;
    attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
    attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    passCreateInfo.attachmentCount = 1;
    passCreateInfo.pAttachments = &attachmentDescription;

    VkSubpassDescription basicSubpassDescription{};
    basicSubpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    basicSubpassDescription.colorAttachmentCount = 0;

    VkAttachmentReference depthReference{};
    depthReference.attachment = 0;
    depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

    basicSubpassDescription.pDepthStencilAttachment = &depthReference;

    VkSubpassDependency dependencies[2]{};
    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;
    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
    dependencies[1].srcSubpass = 0;
    dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[1].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    dependencies[1].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
    passCreateInfo.pDependencies = dependencies; // For multi subpass

    passCreateInfo.subpassCount = 1;
    passCreateInfo.pSubpasses = &basicSubpassDescription;
    passCreateInfo.dependencyCount = 2; // For multi subpass

    VK(devFuncs->vkCreateRenderPass(window->device(), &passCreateInfo, VDFT, &shadowPass));
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

    createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    createInfo.anisotropyEnable = VK_FALSE;
    createInfo.maxAnisotropy = 1.0f;
    createInfo.maxLod = 0.0f;
    createInfo.minLod = 0.0;
    createInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    createInfo.magFilter = VK_FILTER_NEAREST;
    createInfo.minFilter = VK_FILTER_NEAREST;
    createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    VK(devFuncs->vkCreateSampler(window->device(), &createInfo, VDFT, &shadowSampler));

}

void Render::createImageView(VkImage image, VkImageAspectFlags aspect, VkFormat format, uint32_t mipsLevel,
                             VkImageView& view, uint32_t layer, uint32_t layerCount, bool array) {
    VkImageViewCreateInfo viewInfo{};
    viewInfo.image = image;
    viewInfo.viewType = layerCount > 1 ? VK_IMAGE_VIEW_TYPE_CUBE : VK_IMAGE_VIEW_TYPE_2D;
    if (array) { viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY; }
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = aspect;
    viewInfo.subresourceRange.baseArrayLayer = layer;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.layerCount = layerCount;
    viewInfo.subresourceRange.levelCount = mipsLevel;
    VK(devFuncs->vkCreateImageView(window->device(), &viewInfo, VDFT, &view));
}

void Render::updateUniforms(bool all, int id) {
    for (int i = 0; i < window->concurrentFrameCount(); i++) {
        if (id != i) { continue; }

        VkWriteDescriptorSet frameUniformWrite{};
        frameUniformWrite.descriptorCount = 1;
        frameUniformWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        frameUniformWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        frameUniformWrite.dstArrayElement = 0;
        frameUniformWrite.dstSet = uniformSet[i];
        frameUniformWrite.dstBinding = UNI_FRAME_BD;
        VkDescriptorBufferInfo frameUniformBufferInfo{};
        frameUniformBufferInfo.buffer = frameUniform.buffer;
        frameUniformBufferInfo.offset = i * uniformSize(sizeof(UniFrame));
        frameUniformBufferInfo.range =  uniformSize(sizeof(UniFrame));
        frameUniformWrite.pBufferInfo = &frameUniformBufferInfo;

        VkWriteDescriptorSet objectsUniformWrite{};
        objectsUniformWrite.descriptorCount = 1;
        objectsUniformWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        objectsUniformWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        objectsUniformWrite.dstArrayElement = 0;
        objectsUniformWrite.dstSet = uniformSet[i];
        objectsUniformWrite.dstBinding = UNI_OBJ_BD;
        VkDescriptorBufferInfo objectsUniformBufferInfo{};
        objectsUniformBufferInfo.buffer = objectsUniform.buffer;
        objectsUniformBufferInfo.offset = i * uniformSize(sizeof(UniObject)) * OBJECTS_MAX_COUNT;
        objectsUniformBufferInfo.range =  uniformSize(sizeof(UniObject)) * OBJECTS_MAX_COUNT;
        objectsUniformWrite.pBufferInfo = &objectsUniformBufferInfo;

        VkWriteDescriptorSet shadowUniformWrite{};
        shadowUniformWrite.descriptorCount = 1;
        shadowUniformWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        shadowUniformWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        shadowUniformWrite.dstArrayElement = 0;
        shadowUniformWrite.dstSet = uniformSet[i];
        shadowUniformWrite.dstBinding = UNI_SHADOW_BD;
        VkDescriptorBufferInfo shadowUniformBufferInfo{};
        shadowUniformBufferInfo.buffer = shadowUniform.buffer;
        shadowUniformBufferInfo.offset = i * uniformSize(sizeof(UniShadow)) * 4;
        shadowUniformBufferInfo.range =  uniformSize(sizeof(UniShadow)) * 4;
        shadowUniformWrite.pBufferInfo = &shadowUniformBufferInfo;

        if (all) {
            devFuncs->vkUpdateDescriptorSets(window->device(), 1, &frameUniformWrite, 0, nullptr);
            devFuncs->vkUpdateDescriptorSets(window->device(), 1, &objectsUniformWrite, 0, nullptr);
            devFuncs->vkUpdateDescriptorSets(window->device(), 1, &shadowUniformWrite, 0, nullptr);
        }

        VkWriteDescriptorSet textureWrite0{};
        textureWrite0.descriptorCount = 1;
        textureWrite0.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        textureWrite0.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        textureWrite0.dstArrayElement = 0;
        textureWrite0.dstSet = uniformSet[i];
        textureWrite0.dstBinding = UNI_MAX_BD;
        VkDescriptorImageInfo textureImage0Info{};
        textureImage0Info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        textureImage0Info.imageView = albedoView;
        textureImage0Info.sampler = standSampler;
        textureWrite0.pImageInfo = &textureImage0Info;
        devFuncs->vkUpdateDescriptorSets(window->device(), 1, &textureWrite0, 0, nullptr);

        VkWriteDescriptorSet pbrTexWrite0{};
        pbrTexWrite0.descriptorCount = 1;
        pbrTexWrite0.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        pbrTexWrite0.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        pbrTexWrite0.dstArrayElement = 0;
        pbrTexWrite0.dstSet = uniformSet[i];
        pbrTexWrite0.dstBinding = UNI_MAX_BD + 1;
        VkDescriptorImageInfo pbrTexImage0Info{};
        pbrTexImage0Info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        pbrTexImage0Info.imageView = PBRView;
        pbrTexImage0Info.sampler = standSampler;
        pbrTexWrite0.pImageInfo = &pbrTexImage0Info;
        devFuncs->vkUpdateDescriptorSets(window->device(), 1, &pbrTexWrite0, 0, nullptr);

        VkWriteDescriptorSet skyTexWrite0{};
        skyTexWrite0.descriptorCount = 1;
        skyTexWrite0.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        skyTexWrite0.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        skyTexWrite0.dstArrayElement = 0;
        skyTexWrite0.dstSet = uniformSet[i];
        skyTexWrite0.dstBinding = UNI_MAX_BD + 2;
        VkDescriptorImageInfo skyTexImage0Info{};
        skyTexImage0Info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        skyTexImage0Info.imageView = skyView;
        skyTexImage0Info.sampler = standSampler;
        skyTexWrite0.pImageInfo = &skyTexImage0Info;
        devFuncs->vkUpdateDescriptorSets(window->device(), 1, &skyTexWrite0, 0, nullptr);

        VkWriteDescriptorSet normalTexWrite0{};
        normalTexWrite0.descriptorCount = 1;
        normalTexWrite0.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        normalTexWrite0.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        normalTexWrite0.dstArrayElement = 0;
        normalTexWrite0.dstSet = uniformSet[i];
        normalTexWrite0.dstBinding = UNI_MAX_BD + 3;
        VkDescriptorImageInfo normalTexImage0Info{};
        normalTexImage0Info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        normalTexImage0Info.imageView = normalView;
        normalTexImage0Info.sampler = standSampler;
        normalTexWrite0.pImageInfo = &normalTexImage0Info;
        devFuncs->vkUpdateDescriptorSets(window->device(), 1, &normalTexWrite0, 0, nullptr);

        VkWriteDescriptorSet shadowWrite0{};
        shadowWrite0.descriptorCount = 1;
        shadowWrite0.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        shadowWrite0.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        shadowWrite0.dstArrayElement = 0;
        shadowWrite0.dstSet = uniformSet[i];
        shadowWrite0.dstBinding = UNI_MAX_BD + 4;
        VkDescriptorImageInfo shadowImage0Info{};
        shadowImage0Info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        shadowImage0Info.imageView = shadowMap.view;
        shadowImage0Info.sampler = shadowSampler;
        shadowWrite0.pImageInfo = &shadowImage0Info;
        devFuncs->vkUpdateDescriptorSets(window->device(), 1, &shadowWrite0, 0, nullptr);

        VkWriteDescriptorSet vdynamicWrite0{};
        vdynamicWrite0.descriptorCount = 1;
        vdynamicWrite0.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        vdynamicWrite0.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        vdynamicWrite0.dstArrayElement = 0;
        vdynamicWrite0.dstSet = uniformSet[i];
        vdynamicWrite0.dstBinding = UNI_MAX_BD + 5;
        VkDescriptorImageInfo vdynamicImage0Info{};
        vdynamicImage0Info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        vdynamicImage0Info.imageView = normalView;
        vdynamicImage0Info.sampler = standSampler;
        vdynamicWrite0.pImageInfo = &vdynamicImage0Info;
        devFuncs->vkUpdateDescriptorSets(window->device(), 1, &vdynamicWrite0, 0, nullptr);
    }
}

void Render::drawObjects(VkCommandBuffer& cmdBuf, uint32_t* dynamicBinding) {
    VkDeviceSize offsets[] = { 0 };
    VkDeviceSize instanceOffsets[] = { 0 };

    // ################### draw objects #####################################################
    albedoView = testAlbedo.view; PBRView = testPBR.view; normalView = testNormal.view;
    // albedoView = shadowMap.multiView[1];
    updateUniforms(false, window->currentFrame());
    dynamicBinding[0] = 0 * uniformSize(sizeof(UniObject));
    devFuncs->vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, standardPipelineLayout, 0, 1,
                                      &uniformSet[window->currentFrame()], 2, dynamicBinding);

    devFuncs->vkCmdBindVertexBuffers(cmdBuf, 0, 1, &terrainVertex.buffer, offsets);
    devFuncs->vkCmdBindIndexBuffer(cmdBuf, terrainIndex.buffer, 0, IndexType);

    devFuncs->vkCmdBindVertexBuffers(cmdBuf, 1, 1, &testInstance.buffer, instanceOffsets);
    devFuncs->vkCmdDrawIndexed(cmdBuf, terrainIndex.count, testInstance.count, 0, 0, 0);

    for (uint32_t i = 0; i < PLANT_TYPE_NUM; i++) {
        albedoView = plantsAlbedo[i].view; PBRView = plantsPBR[i].view; normalView = plantsNormal[i].view;
        updateUniforms(false, window->currentFrame());
        dynamicBinding[0] = (1 + i) * uniformSize(sizeof(UniObject));
        devFuncs->vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, standardPipelineLayout, 0, 1,
                                          &uniformSet[window->currentFrame()], 2, dynamicBinding);

        devFuncs->vkCmdBindVertexBuffers(cmdBuf, 0, 1, &plantsVertex[i].buffer, offsets);
        devFuncs->vkCmdBindIndexBuffer(cmdBuf, plantsIndex[i].buffer, 0, IndexType);

        devFuncs->vkCmdBindVertexBuffers(cmdBuf, 1, 1, &plantsInstance[i].buffer, instanceOffsets);
        devFuncs->vkCmdDrawIndexed(cmdBuf, plantsIndex[i].count, plantsInstance[i].count, 0, 0, 0);
    }
}

uint32_t Render::uniformSize(uint32_t realSize) {
    const VkPhysicalDeviceLimits *pdevLimits = &window->physicalDeviceProperties()->limits;
    const VkDeviceSize uniAlign = pdevLimits->minUniformBufferOffsetAlignment;
    uint32_t uniformSize = (realSize / uniAlign + 1) * uniAlign;
    return uniformSize;
}

void Render::fillUnifromLayoutBindings(VkDescriptorSetLayoutBinding* bindings) {
    VkDescriptorSetLayoutBinding uniFrameBinding{};
    uniFrameBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uniFrameBinding.binding = UNI_FRAME_BD;
    uniFrameBinding.descriptorCount = 1;
    uniFrameBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    VkDescriptorSetLayoutBinding uniObjBinding{};
    uniObjBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    uniObjBinding.binding = UNI_OBJ_BD;
    uniObjBinding.descriptorCount = 1;
    uniObjBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    VkDescriptorSetLayoutBinding uniShadowBinding{};
    uniShadowBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    uniShadowBinding.binding = UNI_SHADOW_BD;
    uniShadowBinding.descriptorCount = 1;
    uniShadowBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutBinding texBinding0{};  // 反射率贴图
    texBinding0.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    texBinding0.binding = UNI_MAX_BD;
    texBinding0.descriptorCount = 1;
    texBinding0.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    VkDescriptorSetLayoutBinding pbrBinding{};  // PBR纹理
    pbrBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    pbrBinding.binding = UNI_MAX_BD + 1;
    pbrBinding.descriptorCount = 1;
    pbrBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT;
    VkDescriptorSetLayoutBinding skyBinding{};
    skyBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    skyBinding.binding = UNI_MAX_BD + 2;
    skyBinding.descriptorCount = 1;
    skyBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT;
    VkDescriptorSetLayoutBinding normalBinding{};
    normalBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    normalBinding.binding = UNI_MAX_BD + 3;
    normalBinding.descriptorCount = 1;
    normalBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutBinding shadowBinding{};
    shadowBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    shadowBinding.binding = UNI_MAX_BD + 4;
    shadowBinding.descriptorCount = 1;
    shadowBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutBinding vdynamicBinding{};
    vdynamicBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    vdynamicBinding.binding = UNI_MAX_BD + 5;
    vdynamicBinding.descriptorCount = 1;
    vdynamicBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutBinding tbindings[] = { uniFrameBinding, uniObjBinding, uniShadowBinding,
                                                texBinding0, pbrBinding,
                                                skyBinding, normalBinding, shadowBinding, vdynamicBinding };
    memcpy_s(bindings, sizeof(tbindings), tbindings, sizeof(tbindings));
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
    devFuncs->vkCmdPipelineBarrier(cmdBuf, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0,
                                   nullptr, 0, nullptr, 1, &makeImageAllLevelToDst);

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
        devFuncs->vkCmdPipelineBarrier(cmdBuf, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0,
                                       nullptr, 0, nullptr, 1, &makeImageIndexIm1ToSrc);

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
        devFuncs->vkCmdBlitImage(cmdBuf, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image,
                                 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);

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
        devFuncs->vkCmdPipelineBarrier(cmdBuf, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, 0,
                                       0, nullptr, 0, nullptr, 1, &makeImageIndexIm1ToShader);
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

void Render::loadImage(Texture2D& texture, const char* path, bool srgb, VkExtent2D csize) {
    VkFormat format = srgb ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM;
    QImage file(path);
    file = file.convertToFormat(QImage::Format_RGBA8888);
    VkExtent2D size = { U32(file.width()), U32(file.height()) };
    uint32_t miplevel = MIP(size.width, size.height);
    createImage(size, miplevel, format, notuse, texture.image, VK_IMAGE_USAGE_SAMPLED_BIT |
                VK_IMAGE_USAGE_TRANSFER_DST_BIT, texture.memory, texture.requirement, 0, 1, false, csize);
    createImageView(texture.image, VK_IMAGE_ASPECT_COLOR_BIT, format, miplevel, texture.view, 0, 1, false);
    fillImage(file.sizeInBytes(), size, miplevel, file.bits(), texture, { 0, 0 }, 0, false);
    genMipmaps(texture.image, size, miplevel, 0);
}

void Render::loadCubemap(Texture2D& texture, const char* path[6], bool srgb) {
    VkFormat format = srgb ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM;
    QImage files[6];
    VkExtent2D size = { 0, 0 };
    uint32_t miplevel = 0;
    for (size_t i = 0; i < 6; i++) {
        files[i].load(path[i]);
        files[i] = files[i].convertToFormat(QImage::Format_RGBA8888);
        if (i == 0) {
            size = { U32(files[i].width()), U32(files[i].height()) };
            miplevel = MIP(size.width, size.height);
        }
        if (size.width != U32(files[i].width()) || size.height != U32(files[i].height()) ||
            files[i].width() != files[i].height()) {
            perror("Cubemap image must have same size as each other and same width and height.\n");
            exit(-1);
        }
    }
    createImage(size, miplevel, format, notuse, texture.image, VK_IMAGE_USAGE_SAMPLED_BIT |
                VK_IMAGE_USAGE_TRANSFER_DST_BIT, texture.memory, texture.requirement,
                VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT, 6, false);
    createImageView(texture.image, VK_IMAGE_ASPECT_COLOR_BIT, format, miplevel, texture.view, 0, 6, false);
    for (size_t i = 0; i < 6; i++) {
        fillImage(files[i].sizeInBytes(), size, miplevel, files[i].bits(), texture, { 0, 0 }, i, i != 0);
        genMipmaps(texture.image, size, miplevel, i);
    }
}

void Render::loadVertex(IBuffer& buffer, uint32_t size) {
    createBuffer(size, buffer.data, buffer.buffer, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                 VK_BUFFER_USAGE_TRANSFER_DST_BIT, buffer.memory, buffer.requirement, false);
}

void Render::loadIndex(IBuffer& buffer, uint32_t size) {
    createBuffer(size, buffer.data, buffer.buffer, VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                 VK_BUFFER_USAGE_TRANSFER_DST_BIT, buffer.memory, buffer.requirement, false);
}

void Render::loadInstance(IBuffer& buffer, uint32_t size) {
    createBuffer(size, buffer.data, buffer.buffer, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                 buffer.memory, buffer.requirement, true);
}

void Render::loadUniform(IBuffer& buffer, uint32_t size, bool dynamic) {
    createBuffer(size, buffer.data, buffer.buffer, dynamic ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC :
                                                             VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                 buffer.memory, buffer.requirement, true);
}

void Render::fillLocalData(IBuffer& buffer, const void* data, uint32_t size, VkDeviceSize offset) {
    memcpy_s(static_cast<uint8_t*>(buffer.data) + offset, size, data, size);
    VkMappedMemoryRange range{};
    range.memory = buffer.memory;
    range.offset = offset;
    range.size = size;
    range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    devFuncs->vkFlushMappedMemoryRanges(window->device(), 1, &range);
}

void Render::destroyBuffer(IBuffer buffer) {
    devFuncs->vkUnmapMemory(window->device(), buffer.memory);
    devFuncs->vkFreeMemory(window->device(), buffer.memory, VDFT);
    devFuncs->vkDestroyBuffer(window->device(), buffer.buffer, VDFT);
}

void Render::destroyImage(Texture2D texture) {
    devFuncs->vkUnmapMemory(window->device(), texture.memory);
    devFuncs->vkDestroyImageView(window->device(), texture.view, VDFT);
    devFuncs->vkFreeMemory(window->device(), texture.memory, VDFT);
    devFuncs->vkDestroyImage(window->device(), texture.image, VDFT);
}

void Render::updateImage(const QImage& img, Texture2D& texture, bool srgb, bool array) {
    devFuncs->vkDestroyImage(window->device(), texture.image, VDFT);
    devFuncs->vkDestroyImageView(window->device(), texture.view, VDFT);
    VkFormat format = srgb ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM;
    VkExtent2D size = { U32(img.width()), U32(img.height()) };
    uint32_t miplevel = MIP(size.width, size.height);
    createImageOnly(size, miplevel, format, texture, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                    0, 1, false);
    createImageView(texture.image, VK_IMAGE_ASPECT_COLOR_BIT, format, miplevel, texture.view, 0, 1, array);
    fillImage(img.sizeInBytes(), size, miplevel, img.bits(), texture, { 0, 0 }, 0, false);
    genMipmaps(texture.image, size, miplevel, 0);
}

void Render::updateCubeMap(const QImage* img, Texture2D& texture) {
    devFuncs->vkDestroyImage(window->device(), texture.image, VDFT);
    devFuncs->vkDestroyImageView(window->device(), texture.view, VDFT);
    VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;
    VkExtent2D size = { U32(img[0].width()), U32(img[0].height()) };
    uint32_t miplevel = MIP(size.width, size.height);
    createImageOnly(size, miplevel, format, texture, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                    VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT, 6, false);
    createImageView(texture.image, VK_IMAGE_ASPECT_COLOR_BIT, format, miplevel, texture.view, 0, 6, false);
    fillImage(img[0].sizeInBytes(), size, miplevel, img[0].bits(), texture, { 0, 0 }, 0, false);
    fillImage(img[1].sizeInBytes(), size, miplevel, img[1].bits(), texture, { 0, 0 }, 1, true);
    fillImage(img[2].sizeInBytes(), size, miplevel, img[2].bits(), texture, { 0, 0 }, 2, true);
    fillImage(img[3].sizeInBytes(), size, miplevel, img[3].bits(), texture, { 0, 0 }, 3, true);
    fillImage(img[4].sizeInBytes(), size, miplevel, img[4].bits(), texture, { 0, 0 }, 4, true);
    fillImage(img[5].sizeInBytes(), size, miplevel, img[5].bits(), texture, { 0, 0 }, 5, true);
    genMipmaps(texture.image, size, miplevel, 0);
    genMipmaps(texture.image, size, miplevel, 1);
    genMipmaps(texture.image, size, miplevel, 2);
    genMipmaps(texture.image, size, miplevel, 3);
    genMipmaps(texture.image, size, miplevel, 4);
    genMipmaps(texture.image, size, miplevel, 5);
}

bool Render::ready() const {
    return readyToRender;
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
