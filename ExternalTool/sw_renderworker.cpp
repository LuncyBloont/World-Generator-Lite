#include "sw_renderworker.h"

SW_RenderWorker::SW_RenderWorker(WGLCore* core): core(core) {

}

SW_RenderWorker::~SW_RenderWorker() {

}

void SW_RenderWorker::update() {
    if (wait < 8.0f) {
        wait += 1.0f;
    }
}

void SW_RenderWorker::reset() {
    wait = 0.0f;
    CLEAR_MEMORY;
}

bool SW_RenderWorker::isComplete() const {
    return wait >= 8.0f;
}

float SW_RenderWorker::progress() const {
    return wait / 8.0f;
}

void SW_RenderWorker::setup() {
    MEMORY_SKIP;

    core->ui.mainView->render->testPBRInfo.roughness = *FOR_DATA(float, walkerR->get());
    core->ui.mainView->render->testPBRInfo.specular = *FOR_DATA(float, walkerS->get());
    core->ui.mainView->render->testPBRInfo.metallic = *FOR_DATA(float, walkerM->get());
    core->ui.mainView->render->testPBRInfo.contrast = *FOR_DATA(float, walkerC->get());
    core->ui.mainView->render->testPBRInfo.ss = *FOR_DATA(float, walkerSS->get());
    core->ui.mainView->render->testPBRBase.roughness = *FOR_DATA(float, walkerRB->get());
    core->ui.mainView->render->testPBRBase.specular = *FOR_DATA(float, walkerSB->get());
    core->ui.mainView->render->testPBRBase.metallic = *FOR_DATA(float, walkerMB->get());
    core->ui.mainView->render->testPBRBase.contrast = *FOR_DATA(float, walkerCB->get());
    core->ui.mainView->render->testPBRBase.ss = *FOR_DATA(float, walkerSSB->get());

    core->ui.mainView->render->renderType = DataView::getActive(FOR_DATA(char, walkerType->get()));
    printf("%s : I done!\n", getName());
}

void SW_RenderWorker::bind(size_t id) {
    core->ui.addInfoTitle("渲染");

    float r = 1.0f, rmin = 0.0f, rmax = 1.0f;
    float s = 0.35f, smin = 0.0f, smax = 1.0f;
    float m = 1.0f, mmin = 0.0f, mmax = 1.0f;
    float c = 1.0f, cmin = 0.0f, cmax = 1.0f;
    float ss = 1.0f, ssmin = 0.0f, ssmax = 1.0f;
    walkerR = new DataView("粗糙度", &r, &rmin, &rmax, DataView::tFloat, core, id);
    walkerS = new DataView("高光度", &s, &smin, &smax, DataView::tFloat, core, id);
    walkerM = new DataView("金属度", &m, &mmin, &mmax, DataView::tFloat, core, id);
    walkerC = new DataView("饱和度", &c, &cmin, &cmax, DataView::tFloat, core, id);
    walkerSS = new DataView("次表面散射", &ss, &ssmin, &ssmax, DataView::tFloat, core, id);
    float rb = 0.0f, rbmin = 0.0f, rbmax = 1.0f;
    float sb = 0.0f, sbmin = 0.0f, sbmax = 1.0f;
    float mb = 0.0f, mbmin = 0.0f, mbmax = 1.0f;
    float cb = 0.0f, cbmin = 0.0f, cbmax = 1.0f;
    float ssb = 0.0f, ssbmin = 0.0f, ssbmax = 1.0f;
    walkerRB = new DataView("粗糙度基值", &rb, &rbmin, &rbmax, DataView::tFloat, core, id);
    walkerSB = new DataView("高光度基值", &sb, &sbmin, &sbmax, DataView::tFloat, core, id);
    walkerMB = new DataView("金属度基值", &mb, &mbmin, &mbmax, DataView::tFloat, core, id);
    walkerCB = new DataView("饱和度基值", &cb, &cbmin, &cbmax, DataView::tFloat, core, id);
    walkerSSB = new DataView("次表面基值", &ssb, &ssbmin, &ssbmax, DataView::tFloat, core, id);
    core->add(walkerR, this);
    core->add(walkerRB, this);
    core->add(walkerS, this);
    core->add(walkerSB, this);
    core->add(walkerM, this);
    core->add(walkerMB, this);
    core->add(walkerC, this);
    core->add(walkerCB, this);
    core->add(walkerSS, this);
    core->add(walkerSSB, this);

    walkerType = new DataView("风格", nullptr, nullptr, nullptr, DataView::tEnum, core, id);
    DataView::makeEnum(walkerType, {
                           "标准",
                           "标准",
                           "菲涅尔",
                           "漫反射",
                           "环境光",
                           "高光",
                           "镜面反射",
                           "天空",
                           "法线",
                           "粗糙度",
                           "高光强度",
                           "金属度",
                           "对比度"
                       });
    core->add(walkerType, this);
}

void SW_RenderWorker::unbind() {
    delete walkerR;
    delete walkerS;
    delete walkerM;
    delete walkerC;
    delete walkerSS;
    delete walkerRB;
    delete walkerSB;
    delete walkerMB;
    delete walkerCB;
    delete walkerSSB;
    delete walkerType;
}

const char* SW_RenderWorker::getName() const {
    return "渲染配置";
}
