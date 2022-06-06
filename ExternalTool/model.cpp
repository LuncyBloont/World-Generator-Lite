#include "model.h"


void Model::vertexInputAttributeDescription(VkVertexInputAttributeDescription* descriptions)
{
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

void Model::vertexInputBindingDescription(VkVertexInputBindingDescription& inputBinding)
{
   inputBinding.binding = 0; // Use first buffer when vkCmdBindVertexBuffers();
   inputBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
   inputBinding.stride = sizeof(Vertex);
}

void Model::loadFromOBJFile(const QString& fileName, Vertexs& model, Indexs& indexs, bool log)
{
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
    while (!file.atEnd())
    {
        buf = file.readLine();
        const std::string str = buf.toStdString();
        const char* line = str.data();
        if (sscanf(line, "%s %f %f %f", type, &frg0, &frg1, &frg2) == 4)
        {
            if (strcmp(type, "v") == 0)
            {
                rawVs.push_back(glm::vec3(frg0, frg1, frg2));
                continue;
            }
            if (strcmp(type, "vn") == 0)
            {
                rawNs.push_back(glm::vec3(frg0, frg1, frg2));
                continue;
            }
        }
        if (sscanf(line, "%s %f %f", type, &frg0, &frg1) == 3)
        {
            if (strcmp(type, "vt") == 0)
            {
                rawUVs.push_back(glm::vec2(frg0, frg1));
                continue;
            }
        }
        if (sscanf(line, "%s", type) == 1)
        {
            if (strcmp(type, "f") == 0)
            {
                size_t findex = str.find("f") + 1;
                bool sping = true;
                for (; findex < str.size(); findex++)
                {
                    if (str[findex] == ' ') { sping = true; }
                    if (str[findex] != ' ' && sping)
                    {
                        uint32_t vid, uvid, nid;
                        sscanf(line + findex, "%u/%u/%u", &vid, &uvid, &nid);
                        Vertex v;
                        v.position = rawVs[vid - 1];
                        v.uv = rawUVs[uvid - 1];
                        v.normal = rawNs[nid - 1];
                        model.push_back(v);
                        indexs.push_back(model.size() - 1);
                    }
                    if (str[findex] != ' ') { sping = false; }
                }
                indexs.push_back(IndexRestart);
            }
        }
    }
    if (log)
    {
        printf("MODEL\n");
        for (size_t i = 0; i < model.size(); i++)
        {
            printf("V(%f,%f,%f/%f,%f,%f/%f,%f)", model[i].position.x, model[i].position.y, model[i].position.z,
                   model[i].normal.x, model[i].normal.y, model[i].normal.z, model[i].uv.x, model[i].uv.y);
        }
        printf("\n\n");
        for (size_t i = 0; i < indexs.size(); i++)
        {
            printf("%u ", indexs[i]);
        }
        printf("\n\n");
    }
    printf("Load %llu vertexs.\n", model.size());
}
