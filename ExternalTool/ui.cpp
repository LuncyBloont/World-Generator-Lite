#include "ui.h"
#include <cstring>

#define CATCHVAL(type, name, p) type name = *(reinterpret_cast<const type*>(p))

UI::UI(MainWindow* mainWin, SceneView* mainV, SceneView* top): mainWin(mainWin), topView(top), mainView(mainV) {

}

void UI::addDataItem(DataView* dataview) {
    const auto makeNumberBox = [] () {
        QDoubleSpinBox* box = new QDoubleSpinBox();
        box->setSingleStep(0.01);
        box->setDecimals(6);
        box->setMaximum(1e9);
        box->setMinimum(-1e9);
        return box;
    };
    const auto makeIntBox = [] () {
        QSpinBox* box = new QSpinBox();
        box->setSingleStep(1);
        box->setMaximum(INT32_MAX);
        box->setMinimum(INT32_MIN);
        return box;
    };

    QLabel* name = new QLabel();
    name->setText(dataview->getName().c_str());
    mainWin->addWidgetToList(name);

    if (dataview->getType() == Type::Name::Vec2) {
        CATCHVAL(glm::vec2, dft, dataview->get());
        QDoubleSpinBox* v0 = makeNumberBox();
        QDoubleSpinBox* v1 = makeNumberBox();
        v0->setValue(dft.x);
        v1->setValue(dft.y);
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
    if (dataview->getType() == Type::Name::Vec3) {
        CATCHVAL(glm::vec3, dft, dataview->get());
        QDoubleSpinBox* v0 = makeNumberBox();
        QDoubleSpinBox* v1 = makeNumberBox();
        QDoubleSpinBox* v2 = makeNumberBox();
        v0->setValue(dft.x);
        v1->setValue(dft.y);
        v2->setValue(dft.y);
        v0->connect(v0, &QDoubleSpinBox::textChanged, [v0, dataview] () {
            CATCHVAL(glm::vec3, val, dataview->get());
            val.x = v0->value();
            dataview->set(&val);
        });
        v1->connect(v1, &QDoubleSpinBox::textChanged, [v1, dataview] () {
            CATCHVAL(glm::vec3, val, dataview->get());
            val.y = v1->value();
            dataview->set(&val);
        });
        v2->connect(v2, &QDoubleSpinBox::textChanged, [v2, dataview] () {
            CATCHVAL(glm::vec3, val, dataview->get());
            val.z = v2->value();
            dataview->set(&val);
        });
        mainWin->addWidgetToList(v0);
        mainWin->addWidgetToList(v1);
        mainWin->addWidgetToList(v2);
    }
    if (dataview->getType() == Type::Name::Vec4) {
        CATCHVAL(glm::vec4, dft, dataview->get());
        QDoubleSpinBox* v0 = makeNumberBox();
        QDoubleSpinBox* v1 = makeNumberBox();
        QDoubleSpinBox* v2 = makeNumberBox();
        QDoubleSpinBox* v3 = makeNumberBox();
        v0->setValue(dft.x);
        v1->setValue(dft.y);
        v2->setValue(dft.y);
        v3->setValue(dft.y);
        v0->connect(v0, &QDoubleSpinBox::textChanged, [v0, dataview] () {
            CATCHVAL(glm::vec4, val, dataview->get());
            val.x = v0->value();
            dataview->set(&val);
        });
        v1->connect(v1, &QDoubleSpinBox::textChanged, [v1, dataview] () {
            CATCHVAL(glm::vec4, val, dataview->get());
            val.y = v1->value();
            dataview->set(&val);
        });
        v2->connect(v2, &QDoubleSpinBox::textChanged, [v2, dataview] () {
            CATCHVAL(glm::vec4, val, dataview->get());
            val.z = v2->value();
            dataview->set(&val);
        });
        v3->connect(v3, &QDoubleSpinBox::textChanged, [v3, dataview] () {
            CATCHVAL(glm::vec4, val, dataview->get());
            val.w = v3->value();
            dataview->set(&val);
        });
        mainWin->addWidgetToList(v0);
        mainWin->addWidgetToList(v1);
        mainWin->addWidgetToList(v2);
        mainWin->addWidgetToList(v3);
    }
    if (dataview->getType() == Type::Name::Float) {
        CATCHVAL(float, dft, dataview->get());
        QDoubleSpinBox* v =makeNumberBox();
        v->setValue(dft);
        v->connect(v, &QDoubleSpinBox::textChanged, [v, dataview] () {
            float val = v->value();
            dataview->set(&val);
        });
        mainWin->addWidgetToList(v);
    }
    if (dataview->getType() == Type::Name::Double) {
        CATCHVAL(double, dft, dataview->get());
        QDoubleSpinBox* v = makeNumberBox();
        v->setValue(dft);
        v->connect(v, &QDoubleSpinBox::textChanged, [v, dataview] () {
            double val = v->value();
            dataview->set(&val);
        });
        mainWin->addWidgetToList(v);
    }
    if (dataview->getType() == Type::Name::Int) {
        CATCHVAL(int, dft, dataview->get());
        QSpinBox* v = makeIntBox();
        v->setValue(dft);
        v->connect(v, &QSpinBox::textChanged, [v, dataview] () {
            int val = v->value();
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
