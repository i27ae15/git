#include <string>
#include <sstream>
#include <fstream>
#include <stdexcept>
#include <iostream>
#include <vector>
#include <sstream>
#include <iomanip>
#include <zlib.h>

#include <utils.h>
#include <file/file.h>

namespace VestFile {

    void saveToFile(const std::string& filePath, const std::vector<unsigned char>& content) {
        std::ofstream outFile(filePath, std::ios::binary);

        if (!outFile) throw std::runtime_error("FAILED TO OPEN " + filePath);
        outFile.write(reinterpret_cast<const char*>(content.data()), content.size());

    }

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

    std::vector<unsigned char> compressData(std::string& inputData) {
        std::vector<unsigned char> data(inputData.begin(), inputData.end());
        return compressData(data);
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

    VestTypes::DecompressedData decompressData(
        const std::vector<unsigned char>& compressedData,
        const size_t decompressedSize
    ) {
        std::vector<unsigned char> decompressedBuffer(decompressedSize);

        z_stream stream {};
        stream.next_in = const_cast<Bytef*>(compressedData.data());
        stream.avail_in = compressedData.size();
        stream.next_out = decompressedBuffer.data();
        stream.avail_out = decompressedSize;

        if (inflateInit(&stream) != Z_OK) {
            PRINT_ERROR("FAILED TO INIT ZLIB FOR DECOMPRESSION");
            return {};
        }

        int result = inflate(&stream, Z_FINISH);
        if (result != Z_STREAM_END) {
            PRINT_ERROR("DECOMPRESSION_FAILED");
            return {};
        }

        return VestTypes::DecompressedData {decompressedBuffer, stream};
    }

}