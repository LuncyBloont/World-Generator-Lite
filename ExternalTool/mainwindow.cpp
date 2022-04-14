#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    QString splitterStyle = "QSplitter:handle { background: #989898; }";
    QString splitterBorderStyle = "QSplitter { border: 2px solid green }";
    int splitterWidth = 5;

    QSplitter* splitter1 = new QSplitter(Qt::Horizontal, ui->centralwidget);
    splitter1->setHandleWidth(splitterWidth);
    splitter1->setStyleSheet(splitterStyle + splitterBorderStyle);

    splitter1->addWidget(ui->views);
    splitter1->addWidget(ui->propertyView);
    QList<int> sizes1;
    sizes1 << 50000 << 15000;
    splitter1->setSizes(sizes1);

    this->centralWidget()->layout()->replaceWidget(ui->widget, splitter1);
    ui->widget->hide();

    QSplitter* splitter2 = new QSplitter(Qt::Vertical, ui->centralwidget);
    splitter2->setHandleWidth(splitterWidth);
    splitter2->setStyleSheet(splitterStyle + "QSplitter { border: 0px solid green }");

    splitter2->addWidget(ui->dataPart);
    splitter2->addWidget(ui->scenePart);
    QList<int> sizes2;
    sizes2 << 40000 << 40000;
    splitter2->setSizes(sizes2);

    ui->views->layout()->replaceWidget(ui->viewsInner, splitter2);
    ui->viewsInner->hide();
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_exitButton_triggered()
{

}

