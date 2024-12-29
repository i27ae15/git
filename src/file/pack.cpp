#include <iostream>
#include <fstream>
#include <vector>
#include <stdexcept>
#include <cstdint>
#include <zlib.h>
#include <arpa/inet.h>
#include <openssl/sha.h>
#include <vector>
#include <sstream>
#include <iomanip>

#include <file/pack.h>
#include <file/file.h>

#include <utils.h>

namespace VestPack {

    void byteToHex(uint8_t byte) {
        std::ostringstream oss;
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
        std::cout << oss.str();
    }

    void byteToBinary(uint8_t byte) {
        std::string binary;
        for (int i = 7; i >= 0; --i) { // Loop through each bit
            binary += (byte & (1 << i)) ? '1' : '0';
        }
        std::cout << binary;
    }

    void printHexAndBinary(uint8_t byte) {
        byteToHex(byte);
        std::cout << " | ";
        byteToBinary(byte);
        std::cout << "\x0A";
    }

    ObjectHeader parseObjectHeader(const std::vector<uint8_t>& rData, size_t& offset) {
        if (offset >= rData.size()) {
            throw std::runtime_error("Invalid PACK file: Offset exceeds data size");
        }

        uint32_t size = 0;
        uint8_t type = 0;
        int shift = 0;

        // Read the first byte
        uint8_t currentByte = rData[offset++];
        printHexAndBinary(currentByte);
        // printHexAndBinary('\x07');
        type = (currentByte >> 4) & 0x07; // (1) Extract type (bits 6-4)
        // printHexAndBinary('\x0F');
        size = currentByte & 0x0F;        // (2) Extract size (bits 3-0)

        // Process continuation bytes if MSB is set
        // printHexAndBinary('\x80');
        while (currentByte & 0x80) { // MSB (bit 7) is 1 => more bytes follow

            if (offset >= rData.size()) {
                throw std::runtime_error("Invalid PACK file: Header is incomplete");
            }
            currentByte = rData[offset++];
            printHexAndBinary(currentByte);
            size |= (currentByte & 0x7F) << shift; // (3) Append 7 bits to size
            shift += 7;
        }

        PRINT_HIGHLIGHT("OBJ_TYPE: " + std::to_string(type));
        PRINT_HIGHLIGHT("START: " + std::to_string(offset));
        PRINT_HIGHLIGHT("SIZE: " + std::to_string(size));

        ObjectHeader obj {type, offset, size};
        return obj;
    }

    uint32_t parsePackHeader(const std::vector<uint8_t>& rData, size_t& offset) {

        // Read version and object count
        uint32_t version =  (static_cast<uint8_t>(rData[offset + 0]) << 24) |
                            (static_cast<uint8_t>(rData[offset + 1]) << 16) |
                            (static_cast<uint8_t>(rData[offset + 2]) << 8)  |
                             static_cast<uint8_t>(rData[offset + 3]);

        uint32_t nObjects = (static_cast<uint8_t>(rData[offset + 4]) << 24) |
                            (static_cast<uint8_t>(rData[offset + 5]) << 16) |
                            (static_cast<uint8_t>(rData[offset + 6]) << 8)  |
                             static_cast<uint8_t>(rData[offset + 7]);

        std::cout << "PACK version: " << version << ", Object count: " << nObjects << std::endl;

        offset += 8;
        return nObjects;
    }

    void processPack(std::vector<uint8_t>& rData, size_t offset) {

        size_t _offset = offset;

        uint32_t nObjects = parsePackHeader(rData, _offset);

        for (uint32_t i {}; i < nObjects; i++) {
            ObjectHeader objHeader = parseObjectHeader(rData, _offset);
            std::vector<uint8_t> nextObject(rData.begin() + objHeader.start, rData.end());

            // if (objHeader.type == VestTypes::OFS_DELTA || objHeader.type == VestTypes::REF_DELTA) {
            //     _offset += objHeader.size;
            //     continue;
            // }

            VestTypes::DecompressedData dData = VestFile::decompressData(
                nextObject, VestTypes::EXPAND_AS_NEEDED
            );

            for (char c : dData.data) std::cout << c;

            PRINT_SUCCESS("USED: " + std::to_string(dData.compressedUsed));
            _offset += dData.compressedUsed;

            if (i == 10) break;
        }

    }
}