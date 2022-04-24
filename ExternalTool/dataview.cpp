#include "dataview.h"

/* DataView::DataView(): name("??"), context(nullptr), dataTime(0)
{
    history.clear();
    now = history.begin();
}

DataView::DataView(std::string name, void* val, uint64_t size): name(name), context(nullptr), dataTime(0)
{
    history.clear();
    now = history.begin();
    this->size = size;
    val = new char[size];
}

DataView::DataView(const DataView& other)
{
    copyFrom(other);
}

DataView& DataView::operator =(const DataView& other)
{
    copyFrom(other);
    return *this;
}

T DataView::get() const
{
    return value;
}

void DataView::set(const T& val)
{
    value = val;
}

const std::string& DataView::getName() const
{
    return name;
}

void DataView::rename(const std::string name)
{
    this->name = name;
}

uint64_t DataView::getTime() const
{
    return dataTime;
}

void DataView::copyFrom(const DataView& other)
{
    history.clear();
    for (auto it = other.history.begin(); it != other.history.end(); it++) {
        history.push_back(*it);
    }
    now = history.begin();
    for (auto it = other.history.begin(); it != other.history.end(); it++) {
        if (other.now == it) {
            break;
        }
        now++;
    }

    value = other.value;
    name = other.name;
    context = other.context;
    dataTime = other.dataTime;
}
*/


Type::Type(Name tname)
{
    type = tname;
    switch (tname) {
    case Name::Char:
        size = sizeof(char) / sizeof(char);
    }
}
