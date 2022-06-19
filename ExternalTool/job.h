#ifndef JOB_H
#define JOB_H
#include <vector>
#include <cstdint>
#include <QImage>
#include "model.h"
#include <QImage>

class Job {
public:
    Job();
    void outputFile(const std::string& fname);

public:
    // 导出成品相关数据，由Workers修改

    QImage heightMap;
    QImage typeMap;
    std::vector<float> hmap;
    float mapSizeU;
    float mapSizeV;
    float mapHeight;
    QImage normalMap;

    QImage mainTexture;

    bool hillReady = false;

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
