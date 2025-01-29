#include <cstdint>
#include <vector>
#include <stdexcept>

#include <file/file.h>
#include <file/pack/helpers.h>

#include <objects/helpers.h>

#include <utils.h>


namespace VestPack {

    void setFileContent(
        const size_t& startAt,
        std::vector<uint8_t>& rData,
        size_t& offset,
        std::string& fContent
    ) {
        std::vector<uint8_t> nextObject(rData.begin() + startAt, rData.end());
        VestTypes::DecompressedData dData = VestFile::decompressData(
            nextObject, VestTypes::EXPAND_AS_NEEDED
        );

        offset += dData.compressedUsed;
        fContent = std::string(dData.data.begin(), dData.data.end());
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

}