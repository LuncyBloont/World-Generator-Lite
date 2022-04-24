#ifndef DATAVIEW_H
#define DATAVIEW_H
#include "helperMacro.h"
#include <string>
#include <cstdint>
#include <list>
#include <GL/gl.h>
#include <glm/glm/glm.hpp>
#include "datacontext.h"

struct Type
{
    enum class Name { Int, Float, Double, Vec2, Vec3, Vec4, String, Char };
    Name type;
    uint64_t size;

    Type(Name tname);
};

class DataView
{
public:
    DataView();
    explicit DataView(const std::string name, void* val, Type type);
    DataView(const DataView& other);
    DataView& operator =(const DataView& other);
    void* get() const;
    void set(const void* val);
    const std::string& getName() const;
    void rename(const std::string name);
    uint64_t getTime() const;

private:
    struct ValImage;
    void* value;
    Type type;
    std::list<ValImage> history;
    typename std::list<ValImage>::iterator now;
    std::string name;
    DataContext* context;
    uint64_t dataTime;
    struct ValImage {
        void* val;
        uint64_t time;
    };

    void copyFrom(const DataView& other);
};

#endif // DATAVIEW_H
