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
};

typedef std::vector<Vertex> Vertexs;
typedef std::vector<uint32_t> Indexs;
#define IndexSize sizeof(uint32_t)
#define IndexRestart UINT32_MAX
#define IndexType VK_INDEX_TYPE_UINT32
#define VertexSize sizeof(Vertex)

class Model
{
public:
    static constexpr uint32_t VERTEX_INPUT_COUNT = 6;

    static void vertexInputAttributeDescription(VkVertexInputAttributeDescription* descriptions);

    static void vertexInputBindingDescription(VkVertexInputBindingDescription& inputBinding);

    static void loadFromOBJFile(const QString& fileName, Vertexs& model, Indexs& indexs, bool log = false);
};

#endif // MODEL_H
