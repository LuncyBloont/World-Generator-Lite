#ifndef MODEL_H
#define MODEL_H
#include <glm/glm/glm.hpp>
#include <vector>
#include <map>
#include <cstddef>
#include <vulkan/vulkan.h>
#include <QFile>
#include <cstdio>
#include <cstring>

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec4 tangent;
    glm::vec2 uv = glm::vec2(0.0f, 0.0f);
    glm::vec4 data0 = glm::vec4(0.0f);
    glm::vec4 data1 = glm::vec4(0.0f);
}; // 模型顶点

struct Transform {
    glm::vec3 position;
    glm::vec3 rotation;
    glm::vec3 scale;
}; // 物体变换

struct MMat {
    alignas(16) glm::mat4 offsetRotate;
    alignas(16) glm::vec4 scale;
};

typedef std::vector<Vertex> Vertexs;        // 模型顶点集
typedef std::vector<uint32_t> Indexs;       // 模型顶点索引

#define IndexSize sizeof(uint32_t)
#define IndexRestart UINT32_MAX
#define IndexType VK_INDEX_TYPE_UINT32
#define VertexSize sizeof(Vertex)
#define InstanceSize sizeof(MMat)

class Model {
public:
    static constexpr uint32_t VERTEX_INPUT_COUNT = 6;           // 顶点数据项树
    static constexpr uint32_t INSTANCE_INPUT_COUNT = 5;         // 实例数据项数

    static void vertexInputAttributeDescription(VkVertexInputAttributeDescription* descriptions);
    // 设置顶点输入配置（用于着色器）

    static void vertexInputBindingDescription(VkVertexInputBindingDescription& inputBinding);
    // 设置顶点绑定配置（用于着色器）

    static void loadFromOBJFile(const QString& fileName, Vertexs& model, Indexs& indexs, bool log = false);
    // 读取OBJ格式模型

    static void instanceInputAttributeDescription(VkVertexInputAttributeDescription* descriptions);
    // 设置实例输入配置（用于着色器）

    static void instanceInputBindingDescription(VkVertexInputBindingDescription& inputBinding);
    // 设置实例绑定配置（用于着色器）

    static MMat packTransform(Transform t);

}; // 模型渲染配置静态辅助函数 helper functions

#endif // MODEL_H
