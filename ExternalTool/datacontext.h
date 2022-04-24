#ifndef DATACONTEXT_H
#define DATACONTEXT_H
#include <cstdint>
#include <map>
#include <string>
#include <cstdint>

class DataView;

class DataContext
{
public:
    struct BinaryPack;
    DataContext();
    bool isSaved() const;
    void markUnsaved();
    void markSaved();
    void saveToBinary(BinaryPack* pack) const;
    void loadFromBinary(const BinaryPack* pack);
    void fromOther(const DataContext& other);
    uint64_t getTime() const;
    void modify();

    struct BinaryPack
    {
        uint64_t length;
        char* data;
    };

private:
    bool unsaved = false;
    std::map<std::string, DataView*> data;
    uint64_t dataTime = 0;
};

#endif // DATACONTEXT_H
