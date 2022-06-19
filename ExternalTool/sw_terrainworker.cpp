#include "sw_terrainworker.h"

SW_TerrainWorker::SW_TerrainWorker(WGLCore* core): core(core) {

}

SW_TerrainWorker::~SW_TerrainWorker() {
    printf("Terrain Worker deleted.\n");
}

void SW_TerrainWorker::update() {
    int width = *FOR_DATA(int, resolutionX->get());
    int height = *FOR_DATA(int, resolutionY->get());
    float sizeu = FOR_DATA(glm::vec2, terrainSize->get())->x;
    float sizev = FOR_DATA(glm::vec2, terrainSize->get())->y;
    float sizeh = *FOR_DATA(float, terrainHeight->get());

    for (int i = 0; i < 1024; i++) {
        if (pos >= width * height) { break; }
        int u = pos % width;
        int v = pos / width;

        map[pos].uv = { u * 1.0f / width, 1.0f - v * 1.0f / height };
        float h = tshader.mainImage({ u * 1.0f / width, v * 1.0f / height }, glm::vec2(width, height)).r;
        map[pos].position = glm::vec3(u * sizeu / width - sizeu * 0.5f,
                                     v * sizev / height - sizev * 0.5f,
                                     h * sizeh);
        map[pos].normal = glm::vec3(0.0f, 0.0f, 1.0f);
        map[pos].tangent = glm::vec3(1.0f, 0.0f, 0.0f);
        map[pos].biotangent = glm::vec3(0.0f, 1.0f, 0.0f);

        for (int ix = 0; ix < innerResolution; ix++) {
            for (int iy = 0; iy < innerResolution; iy++) {
                Core::pic(heightMap, &tshader, u * innerResolution + ix, v * innerResolution + iy);
                if (u * innerResolution + ix + (v * innerResolution + iy) * heightMap.width() < static_cast<int>(job.hmap.size()))
                {
                    job.hmap[u * innerResolution + ix + (v * innerResolution + iy) * heightMap.width()] =
                        tshader.mainImage(glm::vec2((u * innerResolution + ix) * 1.0f / heightMap.width(),
                                                    (v * innerResolution + iy) * 1.0f / heightMap.height()),
                                          glm::vec2(heightMap.width(), heightMap.height())).r;
                }
            }
        }

        pos += 1;
    }
}

void SW_TerrainWorker::reset() {
    pos = 0;
    int width = *FOR_DATA(int, resolutionX->get());
    int height = *FOR_DATA(int, resolutionY->get());

    heightMap = QImage(width * innerResolution, height * innerResolution, QImage::Format_RGBA8888);
    job.hmap.resize(width * innerResolution * height * innerResolution);
    map.resize(width * height);
    indexs.resize((width - 1) * (height - 1) * 5);

    printf("size w %f\n", FOR_DATA(glm::vec2, terrainSize->get())->x);
    printf("size h %f\n", FOR_DATA(glm::vec2, terrainSize->get())->y);
    printf("height %f\n", *FOR_DATA(float, terrainHeight->get()));
    printf("type %s\n", DataView::getEnumString(FOR_DATA(char, noiseType->get()), 0));
    printf("resolution: %dx%d\n", width, height);

    tshader.offset = *FOR_DATA(glm::vec2, offset->get());
    tshader.scale = *FOR_DATA(glm::vec2, scale->get());
    tshader.detal = *FOR_DATA(int, detal->get());
    tshader.rough = *FOR_DATA(float, roughness->get());

    job.hillReady = false;

    CLEAR_MEMORY;
}

bool SW_TerrainWorker::isComplete() const {
    int width = *FOR_DATA(int, resolutionX->get());
    int height = *FOR_DATA(int, resolutionY->get());
    return pos >= width * height;
}

float SW_TerrainWorker::progress() const {
    int width = *FOR_DATA(int, resolutionX->get());
    int height = *FOR_DATA(int, resolutionY->get());
    return pos / 1.0f / (width * height);
}

void SW_TerrainWorker::setup() {
    MEMORY_SKIP;

    QFile obj("C:/tmp/terrain.obj");
    obj.open(QFile::WriteOnly);

    int width = *FOR_DATA(int, resolutionX->get());
    int height = *FOR_DATA(int, resolutionY->get());

    printf("size w %f\n", FOR_DATA(glm::vec2, terrainSize->get())->x);
    printf("size h %f\n", FOR_DATA(glm::vec2, terrainSize->get())->y);
    printf("height %f\n", *FOR_DATA(float, terrainHeight->get()));
    printf("type %s\n", DataView::getEnumString(FOR_DATA(char, noiseType->get()), 0));
    printf("resolution: %dx%d\n", width, height);

    for (int x = 0; x < width; x++) {
        for (int y = 0; y < height; y++) {
            int ix = y * width + x;
            int ixl = y * width + x - 1;
            int ixr = y * width + x + 1;
            int ixu = (y + 1) * width + x;
            int ixd = (y - 1) * width + x;
            map[ix].normal = glm::vec3(0.0f);
            if (x > 0 && y > 0) { map[ix].normal += glm::normalize(glm::cross(map[ixl].position - map[ix].position,
                                                                              map[ixd].position - map[ix].position)); }
            if (x < width - 1 && y > 0) { map[ix].normal += glm::normalize(glm::cross(map[ixd].position - map[ix].position,
                                                                              map[ixr].position - map[ix].position)); }
            if (x < width - 1 && y < height - 1) { map[ix].normal += glm::normalize(glm::cross(map[ixr].position - map[ix].position,
                                                                              map[ixu].position - map[ix].position)); }
            if (x > 0 && y < height - 1) { map[ix].normal += glm::normalize(glm::cross(map[ixu].position - map[ix].position,
                                                                              map[ixl].position - map[ix].position)); }
            map[ix].normal = glm::normalize(map[ix].normal);
            map[ix].tangent = glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), map[ix].normal));
            map[ix].biotangent = glm::cross(map[ix].normal, map[ix].tangent);
            obj.write((std::string("v ") + std::to_string(map[ix].position.x * 1000.0f) + " " +
                      std::to_string(map[ix].position.y * 1000.0f) + " " +
                      std::to_string(map[ix].position.z * 1000.0f) + "\n").data());
            obj.write((std::string("vt ") + std::to_string(map[ix].uv.x) + " " +
                      std::to_string(map[ix].uv.y) + "\n").data());
            obj.write((std::string("vn ") + std::to_string(map[ix].normal.x) + " " +
                      std::to_string(map[ix].normal.y) + " " +
                      std::to_string(map[ix].normal.z) + "\n").data());
        }
    }

    for (int x = 0; x < width - 1; x++) {
        for (int y = 0; y < height - 1; y++) {
            int ix = (y * (width - 1) + x) * 5;
            indexs[ix + 3] = y * width + x;
            indexs[ix + 2] = (1 + y) * width + x;
            indexs[ix + 1] = (1 + y) * width + x + 1;
            indexs[ix + 0] = y * width + x + 1;
            indexs[ix + 4] = IndexRestart;

            obj.write((std::string("f ") +
                       std::to_string(y * width + x + 1) + "/" + std::to_string(y * width + x + 1) + "/" +
                            std::to_string(y * width + x + 1) + " " +

                       std::to_string((1 + y) * width + x + 1) + "/" + std::to_string((1 + y) * width + x + 1) + "/" +
                            std::to_string((1 + y) * width + x + 1) + " " +

                       std::to_string((1 + y) * width + x + 1 + 1) + "/" + std::to_string((1 + y) * width + x + 1 + 1) + "/" +
                            std::to_string((1 + y) * width + x + 1 + 1) + " " +

                       std::to_string(y * width + x + 1 + 1) + "/" + std::to_string(y * width + x + 1 + 1) + "/" +
                            std::to_string(y * width + x + 1 + 1) +
                      "\n").data());
        }
    }

    obj.close();

    core->ui.mainView->render->fillBuffer(map.size() * VertexSize, map.data(), core->ui.mainView->render->terrainVertex, 0);
    core->ui.mainView->render->terrainVertex.count = map.size();
    core->ui.mainView->render->fillBuffer(indexs.size() * IndexSize, indexs.data(), core->ui.mainView->render->terrainIndex, 0);
    core->ui.mainView->render->terrainIndex.count = indexs.size();

    core->ui.mainView->render->updateImage(heightMap, core->ui.mainView->render->testAlbedo, SRGB);

    job.heightMap = heightMap;

    float sizeu = FOR_DATA(glm::vec2, terrainSize->get())->x;
    float sizev = FOR_DATA(glm::vec2, terrainSize->get())->y;
    float sizeh = *FOR_DATA(float, terrainHeight->get());

    job.mapHeight = sizeh;
    job.mapSizeU = sizeu;
    job.mapSizeV = sizev;

    printf("%s : I done!\n", getName());

    job.hillReady = true;
    core->wake("植被生成");
}

void SW_TerrainWorker::bind(size_t id) {
    core->ui.addInfoTitle("地形生成");

    int rx = 128, rxmin = 1, rxmax = 800;
    int ry = 128, rymin = 1, rymax = 800;
    resolutionX = new DataView("U分辨率", &rx, &rxmin, &rxmax, DataView::tInt, core, id);
    core->add(resolutionX, this);
    resolutionY = new DataView("V分辨率", &ry, &rymin, &rymax, DataView::tInt, core, id);
    core->add(resolutionY, this);

    heightMap = QImage(rx * innerResolution, ry * innerResolution, QImage::Format_RGBA8888);

    glm::vec2 dftsize = glm::vec2(12.0f, 12.0f);
    glm::vec2 dftsizemin = glm::vec2(-30.0f, -30.0f);
    glm::vec2 dftsizemax = glm::vec2(30.0f, 30.0f);
    terrainSize = new DataView("尺寸（x, y）", &dftsize, &dftsizemin, &dftsizemax, DataView::tVec2, core, id);
    core->add(terrainSize, this);

    float dftheight = 8.0f, dftheightmin = -30.0f, dftheightmax = 30.0f;
    terrainHeight = new DataView("高度", &dftheight, &dftheightmin, &dftheightmax, DataView::tFloat, core, id);
    core->add(terrainHeight, this);

    noiseType = new DataView("类型", nullptr, nullptr, nullptr, DataView::tEnum, core, id);
    DataView::makeEnum(noiseType, {
                           "Perlin",
                           "ABS Perlin",
                           "Perlin",
                           "Moon",
                           "Desert"
                       });
    core->add(noiseType, this);

    glm::vec2 dftScale{ 2.0f, 2.0f }, dftScaleMin{ -30.0f, -30.0f }, dftScaleMax{ 30.0f, 30.0f };
    scale = new DataView("数据缩放", &dftScale, &dftScaleMin, &dftScaleMax, DataView::tVec2, core, id);
    core->add(scale, this);

    glm::vec2 dftOffset{ 0.0f, 0.0f }, dftOffsetMin{ -100.0f, -100.0f }, dftOffsetMax{ 100.0f, 100.0f };
    offset = new DataView("数据偏移", &dftOffset, &dftOffsetMin, &dftOffsetMax, DataView::tVec2, core, id);
    core->add(offset, this);

    int dftDetal = 6, dftDetalMin = 1, dftDetalMax = 16;
    detal = new DataView("细节", &dftDetal, &dftDetalMin, &dftDetalMax, DataView::tInt, core, id);
    core->add(detal, this);

    float dftRough = 0.5f, dftRoughMin = 0.01f, dftRoughMax = 0.99f;
    roughness = new DataView("粗糙度", &dftRough, &dftRoughMin, &dftRoughMax, DataView::tFloat, core, id);
    core->add(roughness, this);

}

void SW_TerrainWorker::unbind() {
    delete terrainSize;              // vec2
    delete terrainHeight;            // float
    delete noiseType;         // enum
    delete resolutionX;       // Int
    delete resolutionY;       // Int
    delete scale;
    delete offset;
    delete detal;
    delete roughness;
}

const char* SW_TerrainWorker::getName() const {
    return "地形生成代理";
}

glm::vec4 SW_TerrainWorker::TerrainShader::mainImage(glm::vec2 uv, glm::vec2 /*resolution*/) {


    float k = Core::multiPerlin(uv, scale, offset, 1.0f - rough, detal);

    return glm::vec4(k, k, k, 1.0f);
}
