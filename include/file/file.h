#ifndef VEST_FILE_H
#define VEST_FILE_H

#include <string>
#include <fstream>
#include <cstdint>
#include <filesystem>

#include <file/types.h>

namespace VestFile {

    void saveToFile(const std::string& fPath, const std::vector<unsigned char>& content);

    uint8_t getFileType(std::string& fPath);
    uint8_t getFileType(std::string&& fPath);

    std::vector<unsigned char> readFile(std::string& fPath);
    std::vector<unsigned char> readFile(std::string&& fPath);
    std::vector<unsigned char> compressData(std::string& inputData);
    std::vector<unsigned char> compressData(const std::vector<unsigned char>& inputData);

    VestTypes::DecompressedData decompressData(
        const std::vector<unsigned char>& compressedData,
        const size_t decompressedSize = VestTypes::DECOMPRESSED_SIZE
    );

}

#endif // VEST_FILE_H