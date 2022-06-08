#include "ui.h"
#include <cstring>

#define CATCHVAL(type, name, p) type name = *(reinterpret_cast<const type*>(p))

UI::UI(MainWindow* mainWin, SceneView* mainV, SceneView* top): mainWin(mainWin), topView(top), mainView(mainV) {

}

void UI::addDataItem(DataView* dataview) {
    QLabel* name = new QLabel();
    name->setText(dataview->getName().c_str());
    mainWin->addWidgetToList(name);

    if (dataview->getType() == Type::Name::Vec2) {
        CATCHVAL(glm::vec2, dft, dataview->get());
        QDoubleSpinBox* v0 = new QDoubleSpinBox();
        QDoubleSpinBox* v1 = new QDoubleSpinBox();
        v0->setMaximum(1e9); v0->setMinimum(-1e9); v0->setValue(dft.x);
        v1->setMaximum(1e9); v1->setMinimum(-1e9); v1->setValue(dft.y);
        v0->connect(v0, &QDoubleSpinBox::textChanged, [v0, dataview] () {
            CATCHVAL(glm::vec2, val, dataview->get());
            val.x = v0->value();
            dataview->set(&val);
        });
        v1->connect(v1, &QDoubleSpinBox::textChanged, [v1, dataview] () {
            CATCHVAL(glm::vec2, val, dataview->get());
            val.y = v1->value();
            dataview->set(&val);
        });
        mainWin->addWidgetToList(v0);
        mainWin->addWidgetToList(v1);
    }
    if (dataview->getType() == Type::Name::Float) {
        CATCHVAL(float, dft, dataview->get());
        QDoubleSpinBox* v = new QDoubleSpinBox();
        v->setMaximum(1e9);
        v->setMinimum(-1e9);
        v->setValue(dft);
        v->connect(v, &QDoubleSpinBox::textChanged, [v, dataview] () {
            float val = v->value();
            dataview->set(&val);
        });
        mainWin->addWidgetToList(v);
    }
    if (dataview->getType() == Type::Name::Double) {
        CATCHVAL(double, dft, dataview->get());
        QDoubleSpinBox* v = new QDoubleSpinBox();
        v->setMaximum(1e9);
        v->setMinimum(-1e9);
        v->setValue(dft);
        v->connect(v, &QDoubleSpinBox::textChanged, [v, dataview] () {
            double val = v->value();
            dataview->set(&val);
        });
        mainWin->addWidgetToList(v);
    }
    if (dataview->getType() == Type::Name::Enum) {
        const char* data = reinterpret_cast<const char*>(dataview->get());
        char* newdata = new char[dataview->typeData().size];
        memcpy(newdata, data, dataview->typeData().size);

        QComboBox* box = new QComboBox();
        for (size_t i = 1; i < DATA_VIEW_ITEM_COUNT; i++) {
            char* item = DataView::getEnumString(newdata, i);
            if (strlen(item) > 0) {
                box->addItem(item);
            }
        }
        box->setCurrentIndex(DataView::getActive(newdata));
        box->connect(box, &QComboBox::currentIndexChanged, [box, dataview, newdata] () {
            const char* now = DataView::getEnumString(newdata, box->currentIndex() + 1);
            DataView::setEnumString(newdata, now, 0);
            dataview->set(newdata);
        });
        mainWin->addWidgetToList(box);
    }
}

void UI::addInfoTitle(const char* info) {
    QLabel* infoLabel = new QLabel();
    infoLabel->setText(info);
    QFont font;
    font.setBold(true);
    infoLabel->setFont(font);
    mainWin->addWidgetToList(infoLabel);
}

void UI::log(const std::string& s) {
    mainWin->pushLog(s);
}
