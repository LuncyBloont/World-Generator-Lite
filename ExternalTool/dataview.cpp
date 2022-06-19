#include "dataview.h"
#include "wglcore.h"

DataView::DataView(std::string name, void* val, void* minv, void* maxv, Type type, WGLCore* core, size_t listener):
    type(type), name(name), context(nullptr), core(core), listener(listener) {
    value = new uint8_t[type.size];
    min = new uint8_t[type.size];
    max = new uint8_t[type.size];
    if (val) { memcpy(value, val, type.size); }
    else { memset(value, 0, type.size); }
    if (minv) { memcpy(min, minv, type.size); }
    else { memset(min, 0, type.size); }
    if (maxv) { memcpy(max, maxv, type.size); }
    else { memset(max, 0, type.size); }
}

const void* DataView::get() const {
    return value;
}

const void* DataView::getMin() const {
    return min;
}

const void* DataView::getMax() const {
    return max;
}

void DataView::set(const void* val, bool call) {
    memcpy(value, val, type.size);
    if (call) {
        change = true;
        core->wake(listener);
        core->unGen = true;
    }
}

const std::string& DataView::getName() const {
    return name;
}

bool DataView::isModified() const {
    return change;
}

void DataView::mark() {
    change = false;
}

Type::Name DataView::getType() const {
    return type.type;
}

Type DataView::typeData() const {
    return type;
}

const char* DataView::getEnumString(const char* s, size_t index) {
    if (index >= DATA_VIEW_ITEM_COUNT) {
        throw std::runtime_error("Index out of size");
    }
    return s + (DATA_VIEW_ITEM_LENGTH + 1) * index;
}

char* DataView::getEnumString(char* s, size_t index) {
    if (index >= DATA_VIEW_ITEM_COUNT) {
        throw std::runtime_error("Index out of size");
    }
    return s + (DATA_VIEW_ITEM_LENGTH + 1) * index;
}

void DataView::setEnumString(char* s, const char* item, size_t index) {
    if (index >= DATA_VIEW_ITEM_COUNT) {
        throw std::runtime_error("Index out of size");
    }
    strcpy_s(s + (DATA_VIEW_ITEM_LENGTH + 1) * index, DATA_VIEW_ITEM_LENGTH + 1, item);
}

void DataView::initEnumString(char* s) {
    memset(s, 0, sizeof(char) * (DATA_VIEW_ITEM_LENGTH + 1) * (DATA_VIEW_ITEM_COUNT) / sizeof(uint8_t));
}

int DataView::getActive(const char* s) {
    const char* now = DataView::getEnumString(s, 0);
    for (size_t i = 1; i < DATA_VIEW_ITEM_COUNT; i++) {
        const char* item = DataView::getEnumString(s, i);
        printf("cpm [%s] and [%s]\n", item, now);
        if (strcmp(item, now) == 0) {
            return i - 1;
        }
    }
    return 0;
}

void DataView::makeEnum(DataView* dataview, const std::vector<std::string>& ss) {
    if (dataview->typeData().type != Type::Name::Enum) {
        throw std::runtime_error("Make enum with non enum DataView");
    }
    std::string buf;
    buf.resize(dataview->typeData().size);
    for (size_t i = 0; i < ss.size(); i++) {
        DataView::setEnumString(buf.data(), ss[i].data(), i);
    }
    dataview->set(buf.data(), false);
}

Type::Type(Name tname) {
    type = tname;
    switch (tname) {
    case Name::Char:
        size = sizeof(char) / sizeof(uint8_t);
        break;
    case Name::Double:
        size = sizeof(double) / sizeof(uint8_t);
        break;
    case Name::Float:
        size = sizeof(float) / sizeof(uint8_t);
        break;
    case Name::Int:
        size = sizeof(int) / sizeof(uint8_t);
        break;
    case Name::String:
        size = sizeof(char) * (DATA_VIEW_STRING_LENGTH  + 1) / sizeof(uint8_t);
        break;
    case Name::Vec2:
        size = sizeof(glm::vec2) / sizeof(uint8_t);
        break;
    case Name::Vec3:
        size = sizeof(glm::vec3) / sizeof(uint8_t);
        break;
    case Name::Vec4:
        size = sizeof(glm::vec4) / sizeof(uint8_t);
        break;
    case Name::Enum:
        size = sizeof(char) * (DATA_VIEW_ITEM_LENGTH + 1) * (DATA_VIEW_ITEM_COUNT) / sizeof(uint8_t);
        break;
    }
}

Type::Type(const Type& t): type(t.type), size(t.size) {

}

const Type DataView::tInt { Type::Name::Int };
const Type DataView::tFloat { Type::Name::Float };
const Type DataView::tDouble { Type::Name::Double };
const Type DataView::tVec2 { Type::Name::Vec2 };
const Type DataView::tVec3 { Type::Name::Vec3 };
const Type DataView::tVec4 { Type::Name::Vec4 };
const Type DataView::tString { Type::Name::String };
const Type DataView::tChar { Type::Name::Char };
const Type DataView::tEnum { Type::Name::Enum };
