#include "sw_renderworker.h"

SW_RenderWorker::SW_RenderWorker(WGLCore* core): core(core) {

}

SW_RenderWorker::~SW_RenderWorker() {

}

void SW_RenderWorker::update() {
    wait += 1.0f;
    if (wait >= 8.0f) { wait = 8.0f; }
}

void SW_RenderWorker::reset() {
    wait = 0.0f;
}

bool SW_RenderWorker::isComplete() const {
    return wait >= 8.0f;
}

float SW_RenderWorker::progress() const {
    return wait / 8.0f;
}

void SW_RenderWorker::setup() {
    core->ui.mainView->render->testPBR.roughness = *FOR_DATA(float, walkerR->get());
    core->ui.mainView->render->testPBR.specular = *FOR_DATA(float, walkerS->get());
    core->ui.mainView->render->testPBR.metallic = *FOR_DATA(float, walkerM->get());
    core->ui.mainView->render->testPBR.contrast = *FOR_DATA(float, walkerC->get());
    core->ui.mainView->render->renderType = DataView::getActive(FOR_DATA(char, walkerType->get()));
}

void SW_RenderWorker::bind(size_t id) {
    core->ui.addInfoTitle("渲染");

    float r = 0.1f;
    float s = 0.5f;
    float m = 0.0f;
    float c = 0.5f;
    walkerR = new DataView("粗糙度", &r, DataView::tFloat, core, id);
    walkerS = new DataView("高光度", &s, DataView::tFloat, core, id);
    walkerM = new DataView("金属度", &m, DataView::tFloat, core, id);
    walkerC = new DataView("饱和度", &c, DataView::tFloat, core, id);
    core->dataContext.insert(walkerR);
    core->dataContext.insert(walkerS);
    core->dataContext.insert(walkerM);
    core->dataContext.insert(walkerC);
    core->ui.addDataItem(walkerR);
    core->ui.addDataItem(walkerS);
    core->ui.addDataItem(walkerM);
    core->ui.addDataItem(walkerC);

    walkerType = new DataView("风格", nullptr, DataView::tEnum, core, id);
    DataView::makeEnum(walkerType, {
                           "标准",
                           "标准",
                           "菲涅尔",
                           "漫反射",
                           "环境光",
                           "高光",
                           "镜面反射",
                           "天空",
                           "法线"
                       });
    core->ui.addDataItem(walkerType);
    core->dataContext.insert(walkerType);
}

void SW_RenderWorker::unbind() {
    delete walkerR;
    delete walkerS;
    delete walkerM;
    delete walkerC;
}

const char* SW_RenderWorker::getName() const {
    return "渲染配置";
}
