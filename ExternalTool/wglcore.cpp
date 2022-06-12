#include "wglcore.h"

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
    return id;
}

void WGLCore::start() {
    printf("Core start\n");
    thread.start();
}

uint32_t WGLCore::update() {
    for (auto& w : workers) {
        w->update();
    }

    std::string msg = "Status:\n";

    bool allComplete = true;
    for (const auto& w : workers) {
        if (!w->isComplete()) {
            allComplete = false;
        }
        msg.append(w->getName());
        msg.append(": ");
        msg.append(std::to_string(w->progress() * 100.0f) + "%\n");
    }
    msg.append("will generate: ");
    msg.append(allComplete && unGen ? "Yes" : "No");
    msg.append("\n");

    if (allComplete && unGen) {
        generate();
        unGen = false;
    }
    ui.log(msg);

    if (!allComplete) {
        return 10;
    }
    return 100;
}

void WGLCore::wake(size_t id) {
    workers[id]->reset();
}

void WGLCore::generate() {
    for (auto& w : workers) {
        w->setup();
    }
    printf("Generate...\n");
}

WorkersThread::WorkersThread(WGLCore* area): area(area) {

}

void WorkersThread::run() {
    printf("Steps start...\n");
    while (runable) {
        uint32_t w = area->update();
        msleep(w);
    }
}

void WorkersThread::stopWork() {
    runable = false;
}
