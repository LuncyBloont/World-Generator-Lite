#include "datacontext.h"

DataContext::DataContext()
{
    dataTime = 0;
}

bool DataContext::isSaved() const
{
    return !unsaved;
}

void DataContext::markUnsaved()
{
    unsaved = true;
}

void DataContext::markSaved()
{
    unsaved = false;
}

void DataContext::fromOther(const DataContext& other)
{
    data.clear();
    dataTime = other.dataTime;
    // for (auto it = other.data.begin(); it != other.data.end(); it++) {
        // data.insert(std::make_pair(it->first, ))
    // }
}

uint64_t DataContext::getTime() const
{
    return dataTime;
}

void DataContext::modify()
{
    dataTime += 1;
}
