#include "mainwindow.h"
#include <QApplication>
#include "dataview.h"
#include "helperMacro.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.resize(1280, 720);
    w.show();

    /* DataView d1("height", 45.6f);
    DataView v1("hill color", glm::vec3(0.6, 0.3, 0.4));

    DBPRINT(d1.get(), %f);*/

    return a.exec();
}
