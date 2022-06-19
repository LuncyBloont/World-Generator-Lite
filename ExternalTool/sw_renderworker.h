#ifndef SW_RENDERWORKER_H
#define SW_RENDERWORKER_H
#include "dataview.h"
#include "wglcore.h"
#include "datacontext.h"

class SW_RenderWorker final : public StepWorker {
public:
    SW_RenderWorker(WGLCore* core);
    ~SW_RenderWorker();
    void update() override;                              // 步进生成
    void reset() override;                               // 重置工作
    bool isComplete() const override;                    // 返回工作完成情况
    float progress() const override;                     // 返回工作进度，如果可能
    void setup() override;                               // 提交工作到核心
    void bind(size_t id) override;                       // 注册到核心
    void unbind() override;                              // 释放
    const char* getName() const override;                // 返回名称

private:
    DataView* walkerR;
    DataView* walkerS;
    DataView* walkerM;
    DataView* walkerC;
    DataView* walkerSS;
    DataView* walkerRB;
    DataView* walkerSB;
    DataView* walkerMB;
    DataView* walkerCB;
    DataView* walkerSSB;
    DataView* walkerType;
    WGLCore* core;

    float wait = 0.0f;

    GEN_MEMORY;
};

#endif // SW_RENDERWORKER_H
