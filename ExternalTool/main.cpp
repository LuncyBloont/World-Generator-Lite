#include "mainwindow.h"
#include <QApplication>
#include "dataview.h"
#include "helperMacro.h"
#include "sceneview.h"
#include "wglcore.h"
#include "ui.h"
#include "sw_terrainworker.h"

int main(int argc, char* argv[]) {
    QApplication a(argc, argv);

    SceneView* sceneView;
    SceneView* topView;
    QVulkanInstance* instance;

    instance = new QVulkanInstance();
    instance->setLayers(QByteArrayList() << "VK_LAYER_LUNARG_standard_validation");
    if (!instance->create()) { perror("Failed to create Vulkan instance.\n"); exit(-1); }
    QLoggingCategory::setFilterRules(QStringLiteral("qt.vulkan=true"));

    sceneView = new SceneView(nullptr, nullptr, instance, false);
    topView = new SceneView(nullptr, nullptr, instance, true);

    MainWindow w(sceneView, topView);
    w.resize(1280, 720);
    w.show();

    UI tmpui(&w, sceneView, topView);
    WGLCore* core = new WGLCore(tmpui);

    /*
     * Add all workers into core.
     */

    SW_TerrainWorker* terrianWorker = new SW_TerrainWorker(core);
    core->addWorker(terrianWorker);

    /*
     * Then core start, workers will listening their DataViews.
     */

    core->start();

    int exitCode = a.exec();

    delete terrianWorker;
    delete core;

    return exitCode;
}
