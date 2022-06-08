#ifndef DATACONTEXT_H
#define DATACONTEXT_H
#include <cstdint>
#include <map>
#include <string>
#include <cstdint>

class DataView;

class DataContext {
    // 数据上下文，主要用UI查询修改worker的数据，以及项目保存状态管理

public:
    struct BinaryPack;
    DataContext();
    bool isSaved() const;                               // 是否所有DataView都是未保存，即已修改
    void markSaved();                                   // 标记所有DataView已经记录，即未修改
    void saveToBinary(BinaryPack* pack) const;          // 保存数据到二进制包
    void loadFromBinary(const BinaryPack* pack);        // 从二进制包读取数据
    void insert(DataView* dataView);                    // 插入新的DataView
    DataView* get(const std::string& name);             // 获取指定名称的DataView
    // void erease(const std::string& name);               // 删除某个DataView，已弃用，数据环境不能动态更改，属于程序启动时的固定配置

    struct BinaryPack {
        uint64_t length;
        char* data;
    }; // 二进制包，用于存入项目文件，或从文件读取

private:
    std::map<std::string, DataView*> data;
};

#endif // DATACONTEXT_H
