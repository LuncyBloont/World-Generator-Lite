#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(SceneView* mainView, SceneView* topView, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow) {
    ui->setupUi(this);

    connect(ui->viewToHillsButton, &QPushButton::pressed, this, &MainWindow::viewToHillsButton_clicked);
    connect(ui->viewToPlantButton, &QPushButton::pressed, this, &MainWindow::viewToPlantButton_clicked);
    connect(ui->viewToWaterButton, &QPushButton::pressed, this, &MainWindow::viewToWaterButton_clicked);

    connect(this, &MainWindow::signal_pushMessage, this, &MainWindow::slot_pushMessage);

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

    QWidget* cont0 = QWidget::createWindowContainer(mainView);
    ui->scenePart->layout()->replaceWidget(ui->sceneView, cont0);
    ui->sceneView->hide();

    QWidget* cont1 = QWidget::createWindowContainer(topView);
    ui->dataParent->layout()->replaceWidget(ui->dataView, cont1);
    ui->dataView->hide();

    viewToHillsButton_clicked();
    viewToWaterButton_clicked();
    viewToPlantButton_clicked();
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::addWidgetToList(QWidget* w) {
    ui->properties->addWidget(w);
}

void MainWindow::pushLog(const std::string& s) {
    msgbuf = s;
    emit signal_pushMessage();
}

void MainWindow::viewToHillsButton_clicked() {
    if (topViewHills) {
        ui->viewToHillsButton->setText("山脉开");
        topViewHills = false;
    } else {
        ui->viewToHillsButton->setText("山脉关");
        topViewHills = true;
    }
}


void MainWindow::viewToWaterButton_clicked() {
    if (topViewWater) {
        ui->viewToWaterButton->setText("水域开");
        topViewWater = false;
    } else {
        ui->viewToWaterButton->setText("水域关");
        topViewWater = true;
    }
}


void MainWindow::viewToPlantButton_clicked() {
    if (topViewPlants) {
        ui->viewToPlantButton->setText("植被开");
        topViewPlants = false;
    } else {
        ui->viewToPlantButton->setText("植被关");
        topViewPlants = true;
    }
}

void MainWindow::slot_pushMessage() {
    ui->msgLoger->setText(QString::fromStdString(msgbuf));
}

