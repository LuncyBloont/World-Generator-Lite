#ifndef UI_H
#define UI_H
#include <QString>
#include <QWidget>
#include <QLabel>
#include <QTextEdit>
#include <QLineEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <glm/glm/glm.hpp>
#include "mainwindow.h"
#include "sceneview.h"
#include "dataview.h"

class UI {
public:
    MainWindow* mainWin;
    SceneView* topView;
    SceneView* mainView;

public:
    UI(MainWindow* mainWin, SceneView* mainV, SceneView* top);

    void askExit();

    void freshDataList();                               // 刷新用户输入组件
    void addDataItem(DataView* dataview);               // 添加用户输入组件
    void addInfoTitle(const char* info);                // 添加用户输入标题
    void clearItem();                                   // 情况用户输入组件
    void log(const std::string& s);
};

#endif // UI_H
