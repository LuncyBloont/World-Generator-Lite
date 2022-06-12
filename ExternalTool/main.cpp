#include "mainwindow.h"
#include <QApplication>
#include "dataview.h"
#include "helperMacro.h"
#include "sceneview.h"
#include "wglcore.h"
#include "ui.h"
#include "sw_terrainworker.h"
#include "sw_renderworker.h"

int main(int argc, char* argv[]) {
    QApplication a(argc, argv);
    QFile styleFile(":/assets/theme/main.qss");
    styleFile.open(QFile::ReadOnly);
    a.setStyleSheet(styleFile.readAll());
    styleFile.close();

    SceneView* sceneView;
    SceneView* topView;
    QVulkanInstance* instance;

    instance = new QVulkanInstance();
    instance->setLayers(QByteArrayList() << "VK_LAYER_LUNARG_standard_validation");
    if (!instance->create()) { perror("Failed to create Vulkan instance.\n"); exit(-1); }
    QLoggingCategory::setFilterRules(QStringLiteral("qt.vulkan=true"));

    topView = new SceneView(nullptr, nullptr, instance, true);
    sceneView = new SceneView(nullptr, nullptr, instance, false);

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

    SW_RenderWorker* renderWorker = new SW_RenderWorker(core);
    core->addWorker(renderWorker);

    /*
     * Then core start, workers will listening their DataViews.
     */

    core->start();

    int exitCode = a.exec();

    delete terrianWorker;
    delete core;

    return exitCode;
}
