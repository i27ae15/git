#include <string>
#include <fstream>
#include <stdexcept>
#include <iostream>
#include <vector>
#include <sstream>
#include <iomanip>
#include <openssl/sha.h>
#include <zlib.h>

#include <utils.h>
#include <commands/file.h>

namespace Vest {

    std::vector<unsigned char> readFile(std::string&& filePath) {
        return readFile(filePath);
    }

    std::vector<unsigned char> readFile(std::string& filePath) {
        std::ifstream file(filePath, std::ios::binary);

        if (!file) {
            PRINT_ERROR("FAILED TO OPEN FILE: " + filePath);
            throw std::runtime_error("");
        }

        std::vector<unsigned char> fileContent (
            (std::istreambuf_iterator<char>(file)),
            std::istreambuf_iterator<char>()
        );

        file.close();

        return fileContent;
    }

    std::vector<unsigned char> compressData(
        const std::vector<unsigned char>& inputData
    ) {
        uLongf compressedSize = compressBound(inputData.size());
        std::vector<unsigned char> compressedData(compressedSize);

        uint8_t result = compress(
            compressedData.data(),
            &compressedSize,
            inputData.data(),
            inputData.size()
        );

        if (result != Z_OK) throw std::runtime_error("ERROR COMPRESSING FILE");

        compressedData.resize(compressedSize);
        return compressedData;
    }

    std::string computeSHA1(const std::vector<unsigned char>& data) {
        unsigned char hash[SHA_DIGEST_LENGTH];
        SHA1(data.data(), data.size(), hash);

        // Convert to hexadecimal string
        std::stringstream ss;
        for (uint8_t i {}; i < SHA_DIGEST_LENGTH; i++) {
            ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
        }

        return ss.str();
    }

    void saveToFile(const std::string& filePath, const std::vector<unsigned char>& content) {
        std::ofstream outFile(filePath, std::ios::binary);

        if (!outFile) throw std::runtime_error("FAILED TO OPEN " + filePath);
        outFile.write(reinterpret_cast<const char*>(content.data()), content.size());

    }

}