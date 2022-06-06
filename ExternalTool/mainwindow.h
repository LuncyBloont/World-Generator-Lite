#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "helperMacro.h"
#include <QMainWindow>
#include <QSplitter>
#include <QWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QList>
#include "sceneview.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:

    void on_viewToHillsButton_clicked();

    void on_viewToWaterButton_clicked();

    void on_viewToPlantButton_clicked();

private:
    Ui::MainWindow *ui;
    SceneView* sceneView;
    SceneView* topView;
    QVulkanInstance* instance;

    bool topViewHills = true;
    bool topViewWater = true;
    bool topViewPlants = true;
};
#endif // MAINWINDOW_H
