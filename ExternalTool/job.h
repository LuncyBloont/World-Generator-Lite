#ifndef JOB_H
#define JOB_H
#include <vector>
#include <cstdint>
#include <QImage>
#include "wglcore.h"
#include "model.h"

class Job {
public:
    Job();
    void outputFile(const std::string& fname);

public:
    // 导出成品相关数据，由Workers修改

    std::vector<float> heightMap;
    uint32_t mapSizeU;
    uint32_t mapSizeV;
    float mapWidth;
    float mapHeight;
    QImage normalMap;

    QImage mainTexture;

    QImage typeMap0;
    QImage typeMap1;

    std::vector<Transform> tallArborIns;        // 高大乔木实例
    std::vector<Transform> arborIns;            // 乔木实例
    std::vector<Transform> largeShrubIns;       // 大型灌木实例
    std::vector<Transform> shrubIns;            // 灌木实例
    std::vector<Transform> flowerIns;           // 开花植物实例
    std::vector<Transform> grassIns;            // 草实例
    std::vector<Transform> creepingIns;         // 匍匐植物实例
    std::vector<Transform> stoneIns;            // 石头实例
};

#endif // JOB_H
