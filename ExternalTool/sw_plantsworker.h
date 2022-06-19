#ifndef SW_PLANTSWORKER_H
#define SW_PLANTSWORKER_H
#include "wglcore.h"

class SW_PlantsWorker : public StepWorker
{
public:
    SW_PlantsWorker(WGLCore* core);


private:
    WGLCore* core;

    DataView* larborC;
    DataView* arborC;
    DataView* lshrubC;
    DataView* shrubC;
    DataView* flowerC;
    DataView* grassC;
    DataView* climberC;
    DataView* stoneC;

    float bias = 0.95f;

    int times = 4;

    int gent = 0;
    std::vector<Transform> plib[8];

    int all[8] = { 10, 10, 10, 10, 10, 10, 10, 10 };
    int stage[9] = { 0 };

    int getType();

    float toDen(glm::vec2 uv, float height, int type);

    GEN_MEMORY;

    // StepWorker interface
public:
    void update() override;
    void reset() override;
    bool isComplete() const override;
    float progress() const override;
    void setup() override;
    void bind(size_t id) override;
    void unbind() override;
    const char* getName() const override;
};

#endif // SW_PLANTSWORKER_H
