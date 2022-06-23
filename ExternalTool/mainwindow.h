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

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(SceneView* mview, SceneView* topView, QWidget *parent = nullptr);
    ~MainWindow();

    void addWidgetToList(QWidget* w);
    void pushLog(const std::string& s);

    void changeEvent(QEvent* event) override;
    void closeEvent(QCloseEvent *event) override;

private slots:

    void viewToHillsButton_clicked();

    void viewToWaterButton_clicked();

    void viewToPlantButton_clicked();

private:
    Ui::MainWindow *ui;

    bool topViewHills = true;
    bool topViewWater = true;
    bool topViewPlants = true;

    std::string msgbuf;

    SceneView* mainView;

signals:
    void signal_pushMessage();

private slots:
    void slot_pushMessage();
};
#endif // MAINWINDOW_H
