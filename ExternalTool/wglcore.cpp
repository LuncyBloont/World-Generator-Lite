#include "wglcore.h"

std::mutex renderLock0;
std::mutex renderLock1;

std::mutex genLock;

bool threadSleeping = false;

bool resetup = false;

Job job;

WGLCore::WGLCore(const UI& ui): ui(ui), thread(this) {
}

WGLCore::~WGLCore() {
    thread.stopWork();
    thread.wait();
}

size_t WGLCore::addWorker(StepWorker* w) {
    size_t id = workers.size();
    workers.push_back(w);
    workers.back()->bind(id);
    wake(id);
    return id;
}

void WGLCore::start() {
    printf("Core start\n");
    thread.start();
}

uint32_t WGLCore::update() {
    renderLock0.lock();
    renderLock1.lock();

    while (!waitQueue.empty()) {
        workers[waitQueue.back()]->reset();
        waitQueue.pop_back();
    }

    for (auto& w : workers) {
        w->update();
    }

    std::string msg = "Status:\n";

    bool oneComplete = false;
    bool allComplete = true;
    for (const auto& w : workers) {
        if (w->isComplete()) {
            oneComplete = true;
        } else {
            allComplete = false;
        }
        msg.append(w->getName());
        msg.append(": ");
        msg.append(std::to_string(w->progress() * 100.0f) + "%\n");
    }
    msg.append("will generate: ");
    msg.append(oneComplete && unGen ? "Yes" : "No");
    msg.append("\n");

    if (((oneComplete && unGen) || resetup) && ui.mainView->render->ready()) {
        unGen = false;
        generate();
    }
    ui.log(msg);

    if (!allComplete) {
        unGen = true;
        renderLock0.unlock();
        renderLock1.unlock();
        return 10;
    }
    renderLock0.unlock();
    renderLock1.unlock();
    return 100;
}

void WGLCore::wake(size_t id) {
    renderLock0.lock();
    renderLock1.lock();
    workers[id]->reset();
    renderLock1.unlock();
    renderLock0.unlock();
}

void WGLCore::wake(const std::string& name) {
    size_t i = 0;
    for (auto& w : workers) {
        if (name == std::string(w->getName())) {
            waitQueue.push_back(i);
        }
        i++;
    }
}

void WGLCore::add(DataView* view, StepWorker* worker) {
    ui.addDataItem(view);
    dataContext.insert(view, worker->getName());
}

void WGLCore::genSleep() {
    threadSleeping = true;
}

void WGLCore::genStart() {
    threadSleeping = false;
}

void WGLCore::generate() {
    genLock.lock();
    resetup = false;
    for (auto& w : workers) {
        w->setup();
    }
    genLock.unlock();
}

WorkersThread::WorkersThread(WGLCore* area): area(area) {

}

void WorkersThread::run() {
    printf("Steps start...\n");
    while (runable) {
        uint32_t w = 1000;
        if (!threadSleeping) {
            w = area->update();
        }
        msleep(w);
    }
}

void WorkersThread::stopWork() {
    runable = false;
}

QImage Core::shaderIt(glm::vec2 size, Shader* shader) {
    QImage tmp({ static_cast<int>(size.x), static_cast<int>(size.y) }, QImage::Format_RGBA8888);
    for (int x = 0; x < size.x; x++) {
        for (int y = 0; y < size.y; y++) {
            glm::vec4 rgba = shader->mainImage({ x / size.x, y / size.y }, size);
            tmp.setPixelColor(x, y, QColor(rgba.r * 256, rgba.g * 256, rgba.b * 256, rgba.a * 256));
        }
    }
    return tmp;
}

glm::vec4 Core::texture(const QImage& img, glm::vec2 uv) {
    QColor col = img.pixelColor(uv.x * img.width(), uv.y * img.height());
    return glm::vec4(col.redF(), col.greenF(), col.blueF(), col.alphaF());
}

void Core::pic(QImage& image, Shader* shader, int x, int y) {
    glm::vec4 rgba = shader->mainImage({ x * 1.0f / image.width(), y * 1.0f / image.height() },
                                       glm::vec2(image.width(), image.height()));
    rgba = glm::clamp(rgba, glm::vec4(0.0f), glm::vec4(1.0f));
    image.setPixelColor(x, y, QColor(rgba.r * 255, rgba.g * 255, rgba.b * 255, rgba.a * 255));

}

float Core::perline(glm::vec2 uv, glm::vec2 scale, glm::vec2 offset) {
    uv = Core::mod2(uv, glm::vec2(1.0f));
    uv *= scale;
    glm::vec2 inner = glm::fract(uv);
    glm::vec2 p00 = Core::mod2(glm::floor(uv + glm::vec2(0.0f, 0.0f)), scale);
    glm::vec2 p10 = Core::mod2(glm::floor(uv + glm::vec2(1.0f, 0.0f)), scale);
    glm::vec2 p11 = Core::mod2(glm::floor(uv + glm::vec2(1.0f, 1.0f)), scale);
    glm::vec2 p01 = Core::mod2(glm::floor(uv + glm::vec2(0.0f, 1.0f)), scale);

    glm::vec2 v00 = glm::normalize(Core::hashPan(p00 + offset));
    glm::vec2 v10 = glm::normalize(Core::hashPan(p10 + offset));
    glm::vec2 v11 = glm::normalize(Core::hashPan(p11 + offset));
    glm::vec2 v01 = glm::normalize(Core::hashPan(p01 + offset));

    float m0L = glm::mix(glm::dot(inner - glm::vec2(0.0f, 0.0f), v00), glm::dot(inner - glm::vec2(0.0f, 1.0f), v01), Core::slerp(inner.y));
    float m1L = glm::mix(glm::dot(inner - glm::vec2(1.0f, 0.0f), v10), glm::dot(inner - glm::vec2(1.0f, 1.0f), v11), Core::slerp(inner.y));
    return glm::mix(m0L, m1L, Core::slerp(inner.x)) * 0.5f + 0.5f;
}

float Core::hash(float s) {
    return glm::linearRand(0.0f, 1.0f);
    return glm::fract(glm::sin(s * 2.7812f + 1.3811f) * 179.4552f);
}

glm::vec2 Core::hash2(glm::vec2 uv) {
    return glm::vec2(glm::fract(glm::sin(glm::dot(glm::vec2(uv.x, uv.y), glm::vec2(34.8921f, 18.9912f)) + 47.1212f) * 479.4552f),
                     glm::fract(glm::sin(glm::dot(glm::vec2(uv.y, uv.x), glm::vec2(-23.7747f, 76.5823f)) + 99.8212f) * 688.7904f));
}

glm::vec2 Core::hashPan(glm::vec2 uv) {
    glm::vec2 data = hash2(uv);
    float l = glm::pow(data.y + 0.001f, 0.5f);
    float r = data.x * 3.14159f * 2.0f;
    return glm::vec2(glm::cos(r) * l, glm::sin(r) * l);
}

float Core::slerp(float k) {
    return 1.0f - (glm::cos(k * 3.14159265f) * 0.5 + 0.5);
}

float Core::mod(float s, float m) {
    return glm::mod(glm::mod(s, m) + m, m);
}

glm::vec2 Core::mod2(glm::vec2 s, glm::vec2 m) {
    return glm::mod(glm::mod(s, m) + m, m);
}

glm::vec3 Core::mod3(glm::vec3 s, glm::vec3 m) {
    return glm::mod(glm::mod(s, m) + m, m);
}

glm::vec4 Core::mod4(glm::vec4 s, glm::vec4 m) {
    return glm::mod(glm::mod(s, m) + m, m);
}

float Core::multiPerlin(glm::vec2 uv, glm::vec2 scale, glm::vec2 offset, float atte, int times) {
    times = glm::max(1, times);
    float h = 0.0f;
    float base = 0.0f;

    for (int i = 0; i < times; i++) {
        float sl = glm::pow(atte, i);
        h += (Core::perline(uv, scale * glm::pow(2.0f, static_cast<float>(i)), offset) * 2.0f - 1.0f) * sl;
        base += sl;
    }

    return h / base * 0.5f + 0.5f;
}
