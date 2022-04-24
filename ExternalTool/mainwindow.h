#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "helperMacro.h"
#include <QMainWindow>
#include <QSplitter>
#include <QWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QList>

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
    void on_exitButton_triggered();

private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
