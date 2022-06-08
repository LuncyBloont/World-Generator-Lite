#ifndef SW_TERRAINWORKER_H
#define SW_TERRAINWORKER_H
#include "datacontext.h"
#include "dataview.h"
#include "wglcore.h"

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
    DataView* size;              // vec2
    DataView* height;            // float
    DataView* noiseType;         // enum
    WGLCore* core;

    float test = 0.0f;
};

#endif // SW_TERRAINWORKER_H
