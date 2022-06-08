#include "sw_terrainworker.h"

SW_TerrainWorker::SW_TerrainWorker(WGLCore* core): core(core) {

}

SW_TerrainWorker::~SW_TerrainWorker() {
    printf("Terrain Worker deleted.\n");
}

void SW_TerrainWorker::update() {
    test += glm::linearRand(0.0f, 18.0f);
    if (test > 250.0f) {
        test = 250.0f;
    }
}

void SW_TerrainWorker::reset() {
    test = 0.0f;
    printf("size w %f\n", FOR_DATA(glm::vec2, size->get())->x);
    printf("size h %f\n", FOR_DATA(glm::vec2, size->get())->y);
    printf("height %f\n", *FOR_DATA(float, height->get()));
    printf("type %s\n", DataView::getEnumString(FOR_DATA(char, noiseType->get()), 0));
}

bool SW_TerrainWorker::isComplete() const {
    return test >= 250.0f;
}

float SW_TerrainWorker::progress() const {
    return test / 250.0f;
}

void SW_TerrainWorker::setup() {

}

void SW_TerrainWorker::bind(size_t id) {
    core->ui.addInfoTitle("地形生成");

    glm::vec2 dftsize = glm::vec2(128.0f, 128.0f);
    size = new DataView("尺寸", &dftsize, DataView::tVec2, core, id);
    core->dataContext.insert(size);
    core->ui.addDataItem(size);

    float dftheight = 32.0f;
    height = new DataView("高度", &dftheight, DataView::tFloat, core, id);
    core->dataContext.insert(height);
    core->ui.addDataItem(height);

    noiseType = new DataView("类型", nullptr, DataView::tEnum, core, id);
    DataView::makeEnum(noiseType, {
                           "Perlin",
                           "ABS Perlin",
                           "Perlin",
                           "Moon",
                           "Desert"
                       });
    core->dataContext.insert(noiseType);
    core->ui.addDataItem(noiseType);

}

void SW_TerrainWorker::unbind() {

}

const char* SW_TerrainWorker::getName() const {
    return "地形生成代理";
}
