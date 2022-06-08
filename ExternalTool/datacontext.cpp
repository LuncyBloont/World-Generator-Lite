#include "datacontext.h"
#include "dataview.h"

DataContext::DataContext() {
}

bool DataContext::isSaved() const {
    for (const auto& d : data) {
        if (d.second->isModified()) {
            return false;
        }
    }
    return true;
}

void DataContext::markSaved() {
    for (auto& d : data) {
        d.second->mark();
    }
}

void DataContext::insert(DataView* dataView) {

}
