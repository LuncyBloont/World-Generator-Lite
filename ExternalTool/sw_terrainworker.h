#ifndef SW_TERRAINWORKER_H
#define SW_TERRAINWORKER_H
#include "datacontext.h"
#include "dataview.h"
#include "wglcore.h"
#include "sceneview.h"
#include "model.h"

class SW_TerrainWorker final : public StepWorker {
    // 地形生成代理

public:
    SW_TerrainWorker(WGLCore* core);
    ~SW_TerrainWorker();

    void update() override;                              // 步进生成
    void reset() override;                               // 重置工作
    bool isComplete() const override;                    // 返回工作完成情况
    float progress() const override;                     // 返回工作进度，如果可能
    void setup() override;                               // 提交工作到核心
    void bind(size_t id) override;                       // 注册到核心
    void unbind() override;                              // 释放
    const char* getName() const override;                // 返回名称

private:

    struct TerrainShader : public Core::Shader {
        glm::vec2 scale;
        glm::vec2 offset;
        float detal;
        float rough;
        glm::vec4 mainImage(glm::vec2 uv, glm::vec2 resolution) override;
    };

    DataView* terrainSize;              // vec2
    DataView* terrainHeight;            // float
    DataView* noiseType;         // enum
    DataView* resolutionX;       // Int
    DataView* resolutionY;       // Int
    DataView* scale;
    DataView* offset;
    DataView* detal;
    DataView* roughness;

    WGLCore* core;

    QImage heightMap;
    TerrainShader tshader;
    Vertexs map;
    Indexs indexs;

    int pos = 0;

    const int innerResolution = 4;

    GEN_MEMORY;
};

#endif // SW_TERRAINWORKER_H
