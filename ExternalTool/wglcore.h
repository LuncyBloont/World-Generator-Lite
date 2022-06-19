#ifndef WGLCORE_H
#define WGLCORE_H
#include <QThread>
#include <vector>
#include <glm/glm/glm.hpp>
#include <mutex>
#include "datacontext.h"
#include "model.h"
#include "ui.h"
#include "job.h"

#define GEN_MEMORY int submitC = 1; int setupC = 0;
#define MEMORY_SKIP if ((submitC == setupC && !resetup) || !isComplete()) { return; } setupC = submitC;
#define CLEAR_MEMORY submitC++;

#define MAKE_DATA(type, name, title, value, min, max, dtype, core, id) do {\
    type dft##name = value, dft##name##min = min, dft##name##max = max;\
    name = new DataView(title, &dft##name, &dft##name##min, &dft##name##max, dtype, core, id);\
    core->add(name); } while (false);

extern std::mutex renderLock0;
extern std::mutex renderLock1;

extern bool resetup;

extern Job job;

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
    void wake(const std::string& name);

    void add(DataView* view, StepWorker* worker);

    void genSleep();
    void genStart();

private:
    void generate();

public:
    DataContext dataContext;
    UI ui;

private:
    std::vector<StepWorker*> workers;
    WorkersThread thread;

    std::vector<size_t> waitQueue;

public:
    // 供Workers修改和使用：
    std::vector<std::vector<float> > heightMap;
    bool unGen = false;
};

namespace Core {

struct Shader {
    virtual glm::vec4 mainImage(glm::vec2 uv, glm::vec2 resolution) = 0;
};

QImage shaderIt(glm::vec2 size, Shader* shader);

glm::vec4 texture(const QImage& img, glm::vec2 uv);

void pic(QImage& image, Shader* shader, int x, int y);

float hash(float s);
glm::vec2 hash2(glm::vec2 uv);
glm::vec2 hashPan(glm::vec2 uv);
float slerp(float k);
float mod(float s, float m);
glm::vec2 mod2(glm::vec2 s, glm::vec2 m);
glm::vec3 mod3(glm::vec3 s, glm::vec3 m);
glm::vec4 mod4(glm::vec4 s, glm::vec4 m);

float perline(glm::vec2 uv, glm::vec2 scale, glm::vec2 offset);
float multiPerlin(glm::vec2 uv, glm::vec2 scale, glm::vec2 offset, float atte, int times);

}

#endif // WGLCORE_H
