#include "model.h"


void Model::vertexInputAttributeDescription(VkVertexInputAttributeDescription* descriptions) {
   descriptions[0].binding = 0; // Use first buffer when vkCmdBindVertexBuffers()
   descriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
   descriptions[0].location = 0; // location in shader.
   descriptions[0].offset = offsetof(Vertex, position);

   descriptions[1].binding = 0;
   descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
   descriptions[1].location = 1;
   descriptions[1].offset = offsetof(Vertex, normal);

   descriptions[2].binding = 0;
   descriptions[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;
   descriptions[2].location = 2;
   descriptions[2].offset = offsetof(Vertex, tangent);

   descriptions[3].binding = 0;
   descriptions[3].format = VK_FORMAT_R32G32_SFLOAT;
   descriptions[3].location = 3;
   descriptions[3].offset = offsetof(Vertex, uv);

   descriptions[4].binding = 0;
   descriptions[4].format = VK_FORMAT_R32G32B32A32_SFLOAT;
   descriptions[4].location = 4;
   descriptions[4].offset = offsetof(Vertex, data0);

   descriptions[5].binding = 0;
   descriptions[5].format = VK_FORMAT_R32G32B32A32_SFLOAT;
   descriptions[5].location = 5;
   descriptions[5].offset = offsetof(Vertex, data1);
}

void Model::vertexInputBindingDescription(VkVertexInputBindingDescription& inputBinding) {
   inputBinding.binding = 0; // Use first buffer when vkCmdBindVertexBuffers();
   inputBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
   inputBinding.stride = sizeof(Vertex);
}

void Model::loadFromOBJFile(const QString& fileName, Vertexs& model, Indexs& indexs, bool log) {
    model.clear();
    indexs.clear();

    if (log) { printf("Load %s\n", fileName.toStdString().data()); }
    QFile file(fileName);
    file.open(QFile::ReadOnly);
    std::vector<glm::vec3> rawVs;
    std::vector<glm::vec2> rawUVs;
    std::vector<glm::vec3> rawNs;

    QString buf;
    char type[16];
    float frg0, frg1, frg2;
    while (!file.atEnd()) {
        buf = file.readLine();
        const std::string str = buf.toStdString();
        const char* line = str.data();
        if (sscanf(line, "%s %f %f %f", type, &frg0, &frg1, &frg2) == 4) {
            if (strcmp(type, "v") == 0) {
                rawVs.push_back(glm::vec3(frg0, frg1, frg2));
                continue;
            }
            if (strcmp(type, "vn") == 0) {
                rawNs.push_back(glm::vec3(frg0, frg1, frg2));
                continue;
            }
        }
        if (sscanf(line, "%s %f %f", type, &frg0, &frg1) == 3) {
            if (strcmp(type, "vt") == 0) {
                rawUVs.push_back(glm::vec2(frg0, frg1));
                continue;
            }
        }
        if (sscanf(line, "%s", type) == 1) {
            if (strcmp(type, "f") == 0) {
                size_t findex = str.find("f") + 1;
                bool sping = true;
                for (; findex < str.size(); findex++) {
                    if (str[findex] == ' ') { sping = true; }
                    if (str[findex] != ' ' && sping) {

                        uint32_t vid = 0, uvid = 0, nid = 0;
                        sscanf(line + findex, "%u/", &vid);
                        for (; findex < str.size(); findex++) { if (str[findex] == '/') { findex++; break; } }
                        sscanf(line + findex, "%u/", &uvid);
                        for (; findex < str.size(); findex++) { if (str[findex] == '/') { findex++; break; } }
                        sscanf(line + findex, "%u", &nid);

                        Vertex v;
                        if (vid > 0 && vid > rawVs.size()) {
                            printf("vertex%u out of db\n", vid);
                            exit(-1);
                        }
                        if (uvid > 0 && uvid > rawUVs.size()) {
                            printf("UV%u out of db\n", uvid);
                            exit(-1);
                        }
                        if (nid > 0 && nid > rawNs.size()) {
                            printf("normal%u out of db\n", nid);
                            exit(-1);
                        }
                        v.position = vid > 0 ? rawVs[vid - 1] : glm::vec3(0.0f);
                        v.uv = uvid > 0 ? rawUVs[uvid - 1] : glm::vec2(v.position.x, v.position.z);
                        v.normal = nid > 0 ? rawNs[nid - 1] : glm::vec3(1.0f, 0.0f, 0.0f);
                        model.push_back(v);
                        indexs.push_back(model.size() - 1);
                    }
                    if (str[findex] != ' ') { sping = false; }
                }
                indexs.push_back(IndexRestart);
            }
        }
    }
    if (log) {
        printf("MODEL\n");
        for (size_t i = 0; i < model.size(); i++) {
            printf("V(%f,%f,%f/%f,%f,%f/%f,%f)", model[i].position.x, model[i].position.y, model[i].position.z,
                   model[i].normal.x, model[i].normal.y, model[i].normal.z, model[i].uv.x, model[i].uv.y);
        }
        printf("\n\n");
        for (size_t i = 0; i < indexs.size(); i++) {
            printf("%u ", indexs[i]);
        }
        printf("\n\n");
    }
    printf("Load %llu vertexs.\n", model.size());
}

void Model::instanceInputAttributeDescription(VkVertexInputAttributeDescription* descriptions) {
    descriptions[0].binding = 1;
    descriptions[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    descriptions[0].location = VERTEX_INPUT_COUNT;
    descriptions[0].offset = offsetof(MMat, offsetRotate);

    descriptions[1].binding = 1;
    descriptions[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    descriptions[1].location = VERTEX_INPUT_COUNT + 1; // offsetRotate is a mat4
    descriptions[1].offset = offsetof(MMat, offsetRotate) + sizeof(glm::vec4);

    descriptions[2].binding = 1;
    descriptions[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    descriptions[2].location = VERTEX_INPUT_COUNT + 2; // offsetRotate is a mat4
    descriptions[2].offset = offsetof(MMat, offsetRotate) + sizeof(glm::vec4) * 2;

    descriptions[3].binding = 1;
    descriptions[3].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    descriptions[3].location = VERTEX_INPUT_COUNT + 3; // offsetRotate is a mat4
    descriptions[3].offset = offsetof(MMat, offsetRotate) + sizeof(glm::vec4) * 3;

    descriptions[4].binding = 1;
    descriptions[4].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    descriptions[4].location = VERTEX_INPUT_COUNT + 4; // offsetRotate is a mat4
    descriptions[4].offset = offsetof(MMat, scale);
}

void Model::instanceInputBindingDescription(VkVertexInputBindingDescription& inputBinding) {
    inputBinding.binding = 1;
    inputBinding.inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;
    inputBinding.stride = sizeof(MMat);
}

MMat Model::packTransform(Transform t) {
    float cx = glm::cos(t.rotation.x);
    float sx = glm::sin(t.rotation.x);
    float cy = glm::cos(t.rotation.y);
    float sy = glm::sin(t.rotation.y);
    float cz = glm::cos(t.rotation.z);
    float sz = glm::sin(t.rotation.z);
    MMat mm;
    mm.scale = glm::vec4(t.scale, 1.0f);
    mm.offsetRotate = glm::mat4(1.0f, 0.0f, 0.0f, 0.0f,
                                0.0f, 1.0f, 0.0f, 0.0f,
                                0.0f, 0.0f, 1.0f, 0.0f,
                                t.position.x, t.position.y, t.position.z, 1.0f) *
                      glm::mat4(cz, -sz, 0.f, 0.f,
                                sz, cz, 0.f, 0.f,
                                0.f, 0.f, 1.f, 0.f,
                                0.f, 0.f, 0.f, 1.f) *
                      glm::mat4(1.f, 0.f, 0.f, 0.f,
                                0.f, cx, -sx, 0.f,
                                0.f, sx, cx, 0.f,
                                0.f, 0.f, 0.f, 1.f) *
                      glm::mat4(cy, 0.f, sy, 0.f,
                                0.f, 1.f, 0.f, 0.f,
                                -sy, 0.f, cy, 0.f,
                                0.f, 0.f, 0.f, 1.f);;
    return mm;
}
