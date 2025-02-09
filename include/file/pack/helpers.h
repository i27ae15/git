#ifndef VEST_PACK_HELPERS_H
#define VEST_PACK_HELPERS_H

#include <vector>
#include <string>

#include <objects/structs.h>


namespace VestPack {

    struct PackHeader {
        char magic[4]; // Should be "PACK"
        uint32_t version;
        uint32_t numObjects;
    };

    struct ObjectHeader {
        uint8_t type;
        size_t start;
        size_t size;
    };

    ObjectHeader parseObjectHeader(const std::vector<uint8_t>& rData, size_t& offset);

    uint32_t parsePackHeader(const std::vector<uint8_t>& rData, size_t& offset);

    void setFileContent(
        const size_t& startAt,
        std::vector<uint8_t>& rData,
        size_t& offset,
        std::string& fContent
    );

}

#endif // VEST_PACK_HELPERS_H