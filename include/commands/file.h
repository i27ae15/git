#ifndef VEST_FILE_H
#define VEST_FILE_H

#include <string>
#include <fstream>


namespace Vest {

    struct DecompressedData {
        std::vector<unsigned char> data;
        z_stream stream;

        bool isEmpty();
    };

    constexpr const size_t DECOMPRESSED_SIZE = 1024 * 1024;
    constexpr const uint8_t SHA_SIZE = 20;

    bool isValidCharForString(unsigned char* c);
    void saveToFile(const std::string& filePath, const std::vector<unsigned char>& content);

    std::string computeSHA1(const std::vector<unsigned char>& data);
    std::string constructFilePath(std::string fileID, std::string root = ".git/objects/");

    std::vector<unsigned char> readFile(std::string& filePath);
    std::vector<unsigned char> readFile(std::string&& filePath);
    std::vector<unsigned char> compressData(const std::vector<unsigned char>& inputData);
    DecompressedData decompressData(
        const std::vector<unsigned char>& compressedData,
        const size_t decompressedSize = DECOMPRESSED_SIZE
    );

}

#endif // VEST_FILE_H