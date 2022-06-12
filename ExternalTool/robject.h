#ifndef ROBJECT_H
#define ROBJECT_H
#include "sceneview.h"
#include "model.h"

class RObject {
public:
    RObject();

    VkBuffer vertexs;
    VkBuffer indexs;
    VkDeviceMemory vmemory;
    VkDeviceMemory imemory;
    VkMemoryRequirements vRequirement;
    VkMemoryRequirements iRequirement;

    Uniform uniform;

    VkBuffer instance;
    VkDeviceMemory instanceMemory;
    VkMemoryRequirements instanceRequirement;
    void* instanceData;

};

#endif // ROBJECT_H
