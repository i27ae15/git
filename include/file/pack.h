#ifndef VEST_PACK_H
#define VEST_PACK_H

#include <fstream>
#include <cstdint>
#include <vector>
#include <unordered_map>

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

    void byteToHex(uint8_t byte);
    void byteToBinary(uint8_t byte);
    void printHexAndBinary(uint8_t byte);

    void createFiles(uint8_t& fType, std::string& fContent, std::string& dir);

    void processPack(std::vector<uint8_t>& rData, size_t offset, std::string& dir);

}

#endif // VEST_PACK_H