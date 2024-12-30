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

#include <objects/initializers.h>
#include <objects/helpers.h>

#include <utils.h>

namespace VestPack {

    ObjectHeader parseObjectHeader(const std::vector<uint8_t>& rData, size_t& offset) {
        if (offset >= rData.size()) {
            throw std::runtime_error("Invalid PACK file: Offset exceeds data size");
        }

        uint32_t size = 0;
        uint8_t type = 0;
        int shift = 0;

        // Read the first byte
        uint8_t currentByte = rData[offset++];
        type = (currentByte >> 4) & 0x07; // (1) Extract type (bits 6-4)
        size = currentByte & 0x0F;        // (2) Extract size (bits 3-0)

        // Process continuation bytes if MSB is set
        while (currentByte & 0x80) { // MSB (bit 7) is 1 => more bytes follow

            if (offset >= rData.size()) {
                throw std::runtime_error("Invalid PACK file: Header is incomplete");
            }
            currentByte = rData[offset++];
            // printHexAndBinary(currentByte);
            size |= (currentByte & 0x7F) << shift; // (3) Append 7 bits to size
            shift += 7;
        }

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

        // std::cout << "PACK version: " << version << ", Object count: " << nObjects << std::endl;

        offset += 8;
        return nObjects;
    }

    void byteToHex(uint8_t byte) {
        std::ostringstream oss;
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
        std::cout << "\\x" << oss.str();
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

    void createFiles(uint8_t& fType, std::string& fContent, std::string& dir) {

        std::string sha1 {};
        std::string treeFile {};
        std::uint16_t cTreeSize {};

        switch (fType) {
            case VestTypes::COMMIT:
                std::cout << fContent;
                std::cout << "\x0A";
                sha1 = VestObjects::createCommit(fContent, dir);
                PRINT_WARNING("SHA1 WRITTEN: " + sha1);
                break;

            case VestTypes::TREE:

                for (uint8_t c : fContent) {
                    cTreeSize++;
                    byteToHex(c);
                }

                std::cout << "\x0A";

                treeFile = "tree " + std::to_string(fContent.size()) + '\x00';
                treeFile += fContent;

                PRINT_HIGHLIGHT(
                    "MANUAL SIZE: " + std::to_string(cTreeSize) +
                    " | F_CONTENT.SIZE: " + std::to_string(fContent.size())
                );

                // std::cout << treeFile;
                sha1 = VestObjects::writeObject(treeFile, dir);
                PRINT_COLOR(BLUE, "SHA1 WRITTEN: " + sha1);
                break;

            default:
                break;
        }

        if (fType == VestTypes::TREE || fType == VestTypes::COMMIT) {
            PRINT_BIG_SEPARATION;
        }
    }

    void processPack(std::vector<uint8_t>& rData, size_t offset, std::string& dir) {

        size_t _offset = offset;
        uint32_t nObjects = parsePackHeader(rData, _offset);

        for (uint32_t i {}; i < nObjects; i++) {
            ObjectHeader objHeader = parseObjectHeader(rData, _offset);

            if (objHeader.type == VestTypes::REF_DELTA) {
                PRINT_WARNING("REF_DELTA");
                objHeader.start += VestTypes::SHA_BYTES_SIZE;
                _offset += VestTypes::SHA_BYTES_SIZE;
            }

            std::vector<uint8_t> nextObject(rData.begin() + objHeader.start, rData.end());
            VestTypes::DecompressedData dData = VestFile::decompressData(
                nextObject, VestTypes::EXPAND_AS_NEEDED
            );

            std::string fContent = std::string(dData.data.begin(), dData.data.end());
            createFiles(objHeader.type, fContent, dir);

            _offset += dData.compressedUsed;

            // if (i == 10) break;
        }

    }
}