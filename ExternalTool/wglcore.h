#ifndef WGLCORE_H
#define WGLCORE_H
#include <QThread>
#include <vector>
#include <glm/glm/glm.hpp>
#include "datacontext.h"
#include "model.h"
#include "ui.h"

class WGLCore;

class WorkersThread: public QThread {
private:
    WGLCore* area;
    bool runable = true;
public:
    WorkersThread(WGLCore* area);
    void run() override;
    void stopWork();
}; // 生成用线程

class StepWorker {
public:
    virtual void update() = 0;                              // 步进生成
    virtual void reset() = 0;                               // 重置工作
    virtual bool isComplete() const = 0;                    // 返回工作完成情况
    virtual float progress() const = 0;                     // 返回工作进度，如果可能
    virtual void setup() = 0;                               // 提交工作到核心
    virtual void bind(size_t id) = 0;                       // 注册到核心
    virtual void unbind() = 0;                              // 释放
    virtual const char* getName() const = 0;                // 返回名称
}; // 生成用代理

class WGLCore {
public:
    WGLCore(const UI& ui);
    ~WGLCore();
    size_t addWorker(StepWorker* w);        // 添加生成代理

public:
    void start();                               // 核心启动
    uint32_t update();                              // 全局更新
    void wake(size_t id);

private:
    void generate();

public:
    DataContext dataContext;
    UI ui;

private:
    std::vector<StepWorker*> workers;
    WorkersThread* thread;

public:
    // 供Workers修改和使用：
    std::vector<std::vector<float> > heightMap;
    bool unGen = false;
};

#endif // WGLCORE_H
