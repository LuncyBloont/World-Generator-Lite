#include "sceneview.h"

SceneView::SceneView(QVulkanWindow *parent, const DataContext* dataContext,
                     QVulkanInstance* instance, const bool topView)
    : QVulkanWindow{parent}, dataContext(dataContext), instance(instance), topView(topView) {
    setVulkanInstance(instance);
    setSampleCount(2);
    setCursor(Qt::CrossCursor);

}

QVulkanWindowRenderer* SceneView::createRenderer() {
    return new Render(this, topView);
}

Render::Render(QVulkanWindow* w, const bool topView): topView(topView), window(w) {
    Model::loadFromOBJFile(":/assets/models/smoothMonkey.obj", monkey, monkeyIndex, false);

}

void Render::initResources() {
    devFuncs = window->vulkanInstance()->deviceFunctions(window->device());
    functions = window->vulkanInstance()->functions();

    uniformSet = new VkDescriptorSet[window->concurrentFrameCount()];
    const VkPhysicalDeviceLimits *pdevLimits = &window->physicalDeviceProperties()->limits;
    const VkDeviceSize uniAlign = pdevLimits->minUniformBufferOffsetAlignment;
    uniformSize = (sizeof(Uniform) / uniAlign + 1) * uniAlign;
    const VkDeviceSize texAlign = pdevLimits->minTexelBufferOffsetAlignment;
    texturesSize = (MAX_TMP_IMAGE_SIZE / texAlign + 1) * texAlign;
    printf("Allcate %u uniform description set.\n", window->concurrentFrameCount());

    fillBufferInit();
    fillImageInit();
    createSampler();

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

    createBuffer(uniformSize * window->concurrentFrameCount(),
                 uniformData, uniform, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                 uniformMemory, unigormMemoryRequirement, true);

    QImage testImageFile(":/assets/textures/grass.png");
    QImage postImageFile = testImageFile.convertToFormat(QImage::Format_RGBA8888);
    VkExtent2D testISize = { static_cast<uint32_t>(postImageFile.width()), static_cast<uint32_t>(postImageFile.height()) };
    uint32_t testMipLevel = glm::floor(glm::log(glm::max(testISize.width, testISize.height)) / glm::log(2) + 0.5f);
    createImage(testISize, testMipLevel, VK_FORMAT_R8G8B8A8_SRGB, testImageData, testImage,
                VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                testIMemory, testIMemoryRequirement, false);
    createImageView(testImage, VK_IMAGE_ASPECT_COLOR_BIT, VK_FORMAT_R8G8B8A8_SRGB, testMipLevel, testImageView);
    fillImage(postImageFile.sizeInBytes(), testISize, testMipLevel, postImageFile.bits(), testImage, { 0, 0 });
    genMipmaps(testImage, testISize, testMipLevel);

    loadShader();
    buildPipeline(window->swapChainImageSize().width(), window->swapChainImageSize().height());

    pushHill(monkey, monkeyIndex);

    createBuffer(MAX_PLANT_COUNT * InstanceSize, testInstanceData, testInstanceBuffer, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                 testInstanceMemory, testInstanceMemoryRequirement, true);
    testInstanceCount = 16;
    std::vector<Transform> idata;
    for (uint32_t i = 0; i < testInstanceCount; i++) {
        Transform t{};
        t.position = glm::vec3(glm::sphericalRand(3.5f));
        t.rotation = glm::vec3(glm::linearRand(0.0, 6.28), glm::linearRand(0.0, 6.28), glm::linearRand(0.0, 6.28));
        t.scale = glm::vec3(0.5f);
        idata.push_back(t);
    }
    memcpy(testInstanceData, idata.data(), idata.size() * InstanceSize);

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

    devFuncs->vkDestroyPipelineLayout(dev, sceneViewPipelineLayout, VDFT);
    devFuncs->vkDestroyPipeline(dev, sceneViewPipeline, VDFT);
    sceneViewPipelineLayout = VK_NULL_HANDLE;
    sceneViewPipeline = VK_NULL_HANDLE;

    devFuncs->vkDestroyImageView(dev, testImageView, VDFT);
    devFuncs->vkDestroyImage(dev, testImage, VDFT);
    devFuncs->vkFreeMemory(dev, testIMemory, VDFT);

    devFuncs->vkDestroySampler(dev, standSampler, VDFT);

    devFuncs->vkFreeMemory(dev, testInstanceMemory, VDFT);
    devFuncs->vkDestroyBuffer(dev, testInstanceBuffer, VDFT);

    /*

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

    VkImage hillTextures[AREA_TYPE_NUM];
    VkImage hillType0;
    VkImage hillType1;
    VkImage plantType0;
    VkImage plantType1;
    VkImage plantsTextures[PLANT_TYPE_NUM];
    VkImage testImage;
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
    */
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
    devFuncs->vkCmdBeginRenderPass(cmdBuf, &rpBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    devFuncs->vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, sceneViewPipeline);

    VkViewport viewport{};
    viewport.width = window->swapChainImageSize().width();
    viewport.height = window->swapChainImageSize().height();
    viewport.minDepth = 0.0;
    viewport.maxDepth = 1.0;
    viewport.x = 0.0;
    viewport.y = 0.0;
    devFuncs->vkCmdSetViewport(cmdBuf, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = VkExtent2D {
        static_cast<uint32_t>(window->swapChainImageSize().width()),
        static_cast<uint32_t>(window->swapChainImageSize().height())
    };
    devFuncs->vkCmdSetScissor(cmdBuf, 0, 1, &scissor);

    Uniform unif{};
    unif.time = static_cast<float>(clock() / 1000.0f);
    unif.resolution = glm::vec4(static_cast<float>(scissor.extent.width), static_cast<float>(scissor.extent.height), 1.0f, 1.0f);
    memcpy(static_cast<uint8_t*>(uniformData) + window->currentFrame() * uniformSize, &unif, sizeof(Uniform));

    devFuncs->vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, sceneViewPipelineLayout, 0, 1,
                                      &uniformSet[window->currentFrame()], 0, nullptr);

    VkDeviceSize offsets[] = { 0 };
    devFuncs->vkCmdBindVertexBuffers(cmdBuf, 0, 1, &hillVertexBuffer, offsets);
    devFuncs->vkCmdBindIndexBuffer(cmdBuf, hillIndexBuffer, 0, IndexType);
    VkDeviceSize instanceOffsets[] = { 0 };
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
                         VkDeviceMemory& memory, VkMemoryRequirements& memoryRequirement, bool local) {
    VkImageCreateInfo imageCreateInfo{};
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.extent.width = extent.width;
    imageCreateInfo.extent.height = extent.height;
    imageCreateInfo.extent.depth = 1;
    imageCreateInfo.arrayLayers = 1;
    imageCreateInfo.format = format;
    imageCreateInfo.initialLayout = local ? VK_IMAGE_LAYOUT_PREINITIALIZED : VK_IMAGE_LAYOUT_UNDEFINED;
    imageCreateInfo.mipLevels = mipsLevel;
    imageCreateInfo.tiling = local ? VK_IMAGE_TILING_LINEAR : VK_IMAGE_TILING_OPTIMAL;
    imageCreateInfo.usage = usage;
    imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VK(devFuncs->vkCreateImage(window->device(), &imageCreateInfo, VDFT, &image));

    devFuncs->vkGetImageMemoryRequirements(window->device(), image, &memoryRequirement);

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

void Render::fillImage(uint32_t size, VkExtent2D extent, uint32_t mipsLevel, const void* data, VkImage image, VkOffset2D offset) {
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
    makeImageDstBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    makeImageDstBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    makeImageDstBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    makeImageDstBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    makeImageDstBarrier.srcAccessMask = VK_ACCESS_NONE;
    makeImageDstBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    makeImageDstBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    makeImageDstBarrier.subresourceRange.baseArrayLayer = 0;
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
    copy.imageSubresource.baseArrayLayer = 0;
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
    makeImageShaderUseBarrier.subresourceRange.baseArrayLayer = 0;
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

void Render::buildPipeline(uint32_t width, uint32_t height) {
    destroyPipeline();

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
    VkDescriptorSetLayoutBinding texBinding0{};
    texBinding0.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    texBinding0.binding = 1;
    texBinding0.descriptorCount = 1;
    texBinding0.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    VkDescriptorSetLayoutBinding bindings[] = { uniBinding, texBinding0 };

    setLayoutCreateInfo.bindingCount = 2;
    setLayoutCreateInfo.pBindings = bindings;
    VK(devFuncs->vkCreateDescriptorSetLayout(window->device(), &setLayoutCreateInfo, VDFT, &uniformSetLayout));

    pipelineLayoutCreateInfo.pSetLayouts = &uniformSetLayout;
    VK(devFuncs->vkCreatePipelineLayout(window->device(), &pipelineLayoutCreateInfo, VDFT, &sceneViewPipelineLayout));

    VkDescriptorPoolCreateInfo poolCreateInfo{};

    VkDescriptorPoolSize uniPoolSize{};
    uniPoolSize.descriptorCount = window->concurrentFrameCount();
    uniPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    VkDescriptorPoolSize texPoolSize0{};
    texPoolSize0.descriptorCount = window->concurrentFrameCount();
    texPoolSize0.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    VkDescriptorPoolSize poolSizes[] = { uniPoolSize, texPoolSize0 };

    poolCreateInfo.poolSizeCount = 2;
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
    colorBlendState.colorWriteMask = ~0x0;
    colorInfo.pAttachments = &colorBlendState;

    VkPipelineViewportStateCreateInfo viewportCreateInfo{};
    VkViewport viewport{};
    viewport.width = width;
    viewport.height = height;
    viewport.minDepth = 0.0;
    viewport.maxDepth = 1.0;
    viewport.x = 0.0;
    viewport.y = 0.0;
    viewportCreateInfo.pViewports = &viewport;
    viewportCreateInfo.viewportCount = 1;
    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = VkExtent2D {
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height)
    };
    viewportCreateInfo.pScissors = &scissor;
    viewportCreateInfo.scissorCount = 1;
    viewportCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;

    VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo{};
    graphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    graphicsPipelineCreateInfo.layout = sceneViewPipelineLayout; // TODO
    graphicsPipelineCreateInfo.pColorBlendState = &colorInfo;
    graphicsPipelineCreateInfo.pViewportState = &viewportCreateInfo;
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
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
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

    VK(devFuncs->vkCreateGraphicsPipelines(window->device(), VDFT, 1, &graphicsPipelineCreateInfo, VDFT, &sceneViewPipeline));
}

void Render::destroyPipeline() {
    if (sceneViewPipeline != VK_NULL_HANDLE) {
        devFuncs->vkDestroyPipeline(window->device(), sceneViewPipeline, VDFT);
        sceneViewPipeline = VK_NULL_HANDLE;
    }
    if (sceneViewPipelineLayout != VK_NULL_HANDLE) {
        devFuncs->vkDestroyPipelineLayout(window->device(), sceneViewPipelineLayout, VDFT);
        sceneViewPipelineLayout = VK_NULL_HANDLE;
    }
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

void Render::createImageView(VkImage image, VkImageAspectFlags aspect, VkFormat format, uint32_t mipsLevel, VkImageView& view) {
    VkImageViewCreateInfo viewInfo{};
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = aspect;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.layerCount = 1;
    viewInfo.subresourceRange.levelCount = mipsLevel;
    VK(devFuncs->vkCreateImageView(window->device(), &viewInfo, VDFT, &view));
}

void Render::genMipmaps(VkImage image, VkExtent2D size, uint32_t mipsLevel) {
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
    makeImageAllLevelToDst.subresourceRange.baseArrayLayer = 0;
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
        makeImageIndexIm1ToSrc.subresourceRange.baseArrayLayer = 0;
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
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = 1;
        blit.srcSubresource.mipLevel = level - 1;
        blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.baseArrayLayer = 0;
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
        makeImageIndexIm1ToShader.subresourceRange.baseArrayLayer = 0;
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














