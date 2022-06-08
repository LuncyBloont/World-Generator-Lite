#ifndef DATAVIEW_H
#define DATAVIEW_H
#include "helperMacro.h"
#include <string>
#include <vector>
#include <cstdint>
#include <list>
#include <GL/gl.h>
#include <glm/glm/glm.hpp>
#include "datacontext.h"

class WGLCore;

#define DATA_VIEW_STRING_LENGTH 1024
#define DATA_VIEW_ITEM_LENGTH 256
#define DATA_VIEW_ITEM_COUNT 64
#define FOR_DATA(type, v) reinterpret_cast<const type*>(v)

struct Type {
    enum class Name { Int, Float, Double, Vec2, Vec3, Vec4, String, Char, Enum };
    Name type;
    uint64_t size;

    Type(Name tname);
    Type(const Type& t);
}; // 数据类型

class DataView {
    // 数据视图（管理单个数据）

public:
    explicit DataView(const std::string name, void* val, Type type, WGLCore* core, size_t listener);
    const void* get() const;                // 获取数据
    void set(const void* val);              // 修改数据，报告监听者，标记已修改
    const std::string& getName() const;     // 获取名称
    bool isModified() const;                // 返回是否被修改
    void mark();                            // 标记为修改状态已知，之后isModified()将返回false
    Type::Name getType() const;
    Type typeData() const;

private:
    void* value;                    // 数据
    Type type;                      // 类型
    std::string name;               // 数据名称
    DataContext* context;           // 数据上下文
    bool change = false;
    WGLCore* const core;
    const size_t listener;

public:
    static const Type tInt;
    static const Type tFloat;
    static const Type tDouble;
    static const Type tVec2;
    static const Type tVec3;
    static const Type tVec4;
    static const Type tString;
    static const Type tChar;
    static const Type tEnum;

    static const char* getEnumString(const char* s, size_t index);
    static char* getEnumString(char* s, size_t index);
    static void setEnumString(char* s, const char* item, size_t index);
    static void initEnumString(char* s);
    static int getActive(const char* s);

    static void makeEnum(DataView* dataview, const std::vector<std::string>& ss);
    static void makeString(DataView* dataview, const std::string& s);
};

#endif // DATAVIEW_H
