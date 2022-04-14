#ifndef DATAVIEW_H
#define DATAVIEW_H
#include "helperMacro.h"
#include "datacontext.h"

FOR_TYPE(T)
class DataView
{
public:
    DataView();
    DataView(const DataView& other);
    DataView& operator =(const DataView& other);
    T get() const;
    void set(const T& val);

private:
    T value;
    DataContext* context;
};

#endif // DATAVIEW_H
