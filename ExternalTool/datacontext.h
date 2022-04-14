#ifndef DATACONTEXT_H
#define DATACONTEXT_H
#include <cstdint>

class DataContext
{
public:
    struct BinaryPack;
    DataContext();
    bool isSaved() const;
    void markUnsaved();
    void markSaved();
    void saveToBinary(BinaryPack* pack);
    void loadFromBinary(const BinaryPack* pack);

    struct BinaryPack
    {
        uint64_t length;
        char* data;
    };

private:
    bool unsaved = false;
};

#endif // DATACONTEXT_H
