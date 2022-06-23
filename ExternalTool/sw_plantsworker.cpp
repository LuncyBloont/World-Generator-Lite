#include "sw_plantsworker.h"

SW_PlantsWorker::SW_PlantsWorker(WGLCore* core): core(core) {

}

int SW_PlantsWorker::getType() {
    int k = static_cast<int>(Core::hash(gent * 1.0f / stage[8]) * stage[8]);

    for (int i = 0; i < 8; i++) {
        if (k >= stage[i] && k < stage[i + 1]) {
            return i;
        }
    }
    return 7;
}

float SW_PlantsWorker::toDen(glm::vec2 uv, float height, int type) {
    float highComfirm[8] = { 0.3f, 0.4f, 0.6f, 0.65f, 0.8f, 0.8f, 0.89f, 0.6f };
    return glm::max(glm::pow(1.0f - glm::abs(uv.x - 0.5f) / 0.5f, 0.5f), glm::pow(1.0f - glm::abs(uv.y - 0.5f) / 0.5f, 0.5f)) *
            glm::mix(1.0f - height, 1.0f, highComfirm[type]);
}

void SW_PlantsWorker::update() {
    if (gent >= times) { return; }

    for (size_t i = 0; i < glm::pow(2, gent) * stage[8]; i++) {
        int id = getType();
        int limit = 10;
        while (limit-- > 0 && job.hillReady) {
            float rx = Core::hash(3.453f + limit * 0.023f + gent * 1.0f / all[7]) * bias + (1.0f - bias) * 0.5f;
            float ry = Core::hash(8.341f + limit * 0.023f + gent * 1.0f / all[7]) * bias + (1.0f - bias) * 0.5f;
            int x = rx * job.heightMap.width();
            int y = ry * job.heightMap.height();
            float px = rx * job.mapSizeU;
            float py = ry * job.mapSizeV;
            float height = job.hmap[x + y * job.heightMap.width()];

            if (limit > 1 && toDen({ rx, ry }, height, id) > Core::hash(7.351f + limit * 4.442f + gent * 1.0f / all[7])) {
                continue;
            }

            if (limit > 1 && job.typeMap.pixelColor(rx * job.typeMap.width(), ry * job.typeMap.height()).red() < 4) {
                continue;
            }

            float usePos = 1.0f;
            for (int testx = -5; testx <= 5; testx++) {
                for (int testy = -5; testy <= 5; testy++) {
                    if (rx * job.typeMap.width() + testx >= 0 && rx * job.typeMap.width() + testx < job.typeMap.width() &&
                        ry * job.typeMap.height() + testy >=0 && ry * job.typeMap.height() + testy < job.typeMap.height()) {
                        int testId = job.typeMap.pixelColor(rx * job.typeMap.width() + testx, ry * job.typeMap.height() + testy).red();
                        if (testId != id) {
                            usePos *= 0.9f;
                        }
                    }
                    if (glm::linearRand(0.0f, 1.0f) > 0.6f) {
                        float l = glm::linearRand(0.0f, 8.0f);
                        float r = glm::linearRand(0.0f, 6.28f);
                        QColor old = job.mainTexture.pixelColor(rx * job.mainTexture.width() + glm::cos(r) * l,
                                                                ry * job.mainTexture.height() + glm::sin(r) * l);
                        job.mainTexture.setPixelColor(rx * job.mainTexture.width() + glm::cos(r) * l,
                                                      ry * job.mainTexture.height() + glm::sin(r) * l,
                                                      QColor((54 + 15 * glm::linearRand(-1.0f, 1.0f)) * 0.4 + old.red() * 0.6,
                                                             (88 + 15 * glm::linearRand(-1.0f, 1.0f)) * 0.4 + old.green() * 0.6,
                                                             (48 + 15 * glm::linearRand(-1.0f, 1.0f)) * 0.4 + old.blue() * 0.6, 255));
                    }
                }
            }

            if (limit > 1 && Core::hash(6.4489f + limit * 7.1479f + gent * 1.0f / all[7]) < usePos) {
                continue;
            }

            Transform t;
            t.position = glm::vec3(px - job.mapSizeU * 0.5f, py - job.mapSizeV * 0.5f, height * job.mapHeight);
            t.rotation = glm::vec3(0.0f, 0.0f, Core::hash(6.8132f * gent + 8.5145f * limit) * 6.28f);
            t.scale = glm::vec3(1.4f + 0.7f * Core::hash(8.661f * gent + 0.7023f * limit));
            plib[id].push_back(t);
            job.typeMap.setPixelColor(rx * job.typeMap.width(), ry * job.typeMap.height(), QColor(id, 0, 0, 255));
            break;
        }
    }

    gent++;
}

void SW_PlantsWorker::reset() {
    job.typeMap = QImage(job.heightMap.width() / 4, job.heightMap.height() / 4, QImage::Format_RGBA8888);
    job.mainTexture = QImage(job.heightMap.width(), job.heightMap.height(), QImage::Format_RGBA8888);
    job.mainTexture.fill(QColor(70, 115, 63, 255));
    job.typeMap.fill(QColor(46, 0, 0, 255));

    for (int i = 0; i < 8; i++) {
        plib[i].clear();
    }
    all[0] = *FOR_DATA(int, larborC->get());
    all[1] = *FOR_DATA(int, arborC->get());
    all[2] = *FOR_DATA(int, lshrubC->get());
    all[3] = *FOR_DATA(int, shrubC->get());
    all[4] = *FOR_DATA(int, flowerC->get());
    all[5] = *FOR_DATA(int, grassC->get());
    all[6] = *FOR_DATA(int, climberC->get());
    all[7] = *FOR_DATA(int, stoneC->get());
    stage[0] = 0;
    for (int i = 1; i <= 8; i++) {
        stage[i] = stage[i - 1] + all[i - 1];
    }

    gent = 0;

    CLEAR_MEMORY;
}

bool SW_PlantsWorker::isComplete() const {
    return gent >= times;
}

float SW_PlantsWorker::progress() const {
    return gent * 1.0f / times;
}

void SW_PlantsWorker::setup() {
    MEMORY_SKIP;

    QFile csv("C:/tmp/trees.csv");
    csv.open(QFile::WriteOnly);
    csv.write("---,PlantTypeIndex,Position,Rotation,Scale\n");

    for (int i = 0; i < 8 && job.hillReady; i++) {
        std::vector<MMat> buf;
        buf.resize(plib[i].size());
        for (size_t j = 0; j < plib[i].size(); j++) {
            buf[j] = Model::packTransform(plib[i][j], 1.0f);
            csv.write((std::string("P") + std::to_string(i) + "_" + std::to_string(j) + ",\"" + std::to_string(i) + "\",\"").data());
            csv.write((std::string("(X=") + std::to_string(plib[i][j].position.x * 1000.0f) + "," +
                      std::string("Y=") + std::to_string(plib[i][j].position.y * 1000.0f) + "," +
                      std::string("Z=") + std::to_string(plib[i][j].position.z * 1000.0f) + ")\",\"").data());
            csv.write((std::string("(Pitch=") + std::to_string(plib[i][j].rotation.x) + "," +
                      std::string("Yaw=") + std::to_string(plib[i][j].rotation.y) + "," +
                      std::string("Roll=") + std::to_string(plib[i][j].rotation.z) + ")\",\"").data());
            csv.write((std::string("(X=") + std::to_string(plib[i][j].scale.x) + "," +
                      std::string("Y=") + std::to_string(plib[i][j].scale.y) + "," +
                      std::string("Z=") + std::to_string(plib[i][j].scale.z) + ")\"\n").data());
        }
        core->ui.mainView->render->fillLocalData(core->ui.mainView->render->plantsInstance[i],
                                                 buf.data(), buf.size() * InstanceSize, 0);
        core->ui.mainView->render->plantsInstance[i].count = buf.size();

    }

    csv.close();

    job.mainTexture.save("C:/tmp/main.png");

    core->ui.mainView->render->updateImage(job.mainTexture, core->ui.mainView->render->testAlbedo, SRGB, false);
}

void SW_PlantsWorker::bind(size_t id) {
    core->ui.addInfoTitle("植被生成");
    int lac = 4;
    int ac = 5;
    int lsc = 6;
    int sc = 7;
    int fc = 256;
    int gc = 1024;
    int cc = 512;
    int stc = 64;
    int cmin = 0, cmax = 2048;
    larborC = new DataView("高大乔木种子", &lac, &cmin, &cmax, DataView::tInt, core, id);
    core->add(larborC, this);
    arborC = new DataView("乔木种子", &ac, &cmin, &cmax, DataView::tInt, core, id);
    core->add(arborC, this);
    lshrubC = new DataView("大型灌木种子", &lsc, &cmin, &cmax, DataView::tInt, core, id);
    core->add(lshrubC, this);
    shrubC = new DataView("灌木种子", &sc, &cmin, &cmax, DataView::tInt, core, id);
    core->add(shrubC, this);
    flowerC = new DataView("开花植物种子", &fc, &cmin, &cmax, DataView::tInt, core, id);
    core->add(flowerC, this);
    grassC = new DataView("草种子", &gc, &cmin, &cmax, DataView::tInt, core, id);
    core->add(grassC, this);
    climberC = new DataView("匍匐植物种子", &cc, &cmin, &cmax, DataView::tInt, core, id);
    core->add(climberC, this);
    stoneC = new DataView("碎石生成点", &stc, &cmin, &cmax, DataView::tInt, core, id);
    core->add(stoneC, this);
}

void SW_PlantsWorker::unbind() {

}

const char* SW_PlantsWorker::getName() const {
    return "植被生成";
}
