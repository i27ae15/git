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
#include <file/types.h>

namespace VestFile {

    void printAsHex(const char* buffer, std::size_t length) {
        for (std::size_t i = 0; i < length; ++i) {
            std::cout << std::hex << std::setw(2) << std::setfill('0')
                    << (static_cast<unsigned int>(buffer[i]) & 0xFF) << " ";
        }
        std::cout << std::endl;
    }


    void saveToFile(const std::string& fPath, const std::vector<unsigned char>& content) {
        std::ofstream outFile(fPath, std::ios::binary);

        if (!outFile) throw std::runtime_error("FAILED TO OPEN " + fPath);
        outFile.write(reinterpret_cast<const char*>(content.data()), content.size());
    }

    uint8_t getFileType(std::string&& fPath) {
        return getFileType(fPath);
    }

    uint8_t getFileType(std::string& fPath) {
        // Open the file in binary mode
        std::ifstream file(fPath, std::ios::binary);
        if (!file) {
            throw std::runtime_error("Failed to open file: " + fPath);
        }

        PRINT_HIGHLIGHT("F_PATH: " + fPath);

        // Read a portion of the file for decompression (e.g., 30 bytes)
        std::vector<unsigned char> compressedData(
            (std::istreambuf_iterator<char>(file)),
            std::istreambuf_iterator<char>()
        );

        // Define expected decompressed size (adjust based on typical object size)
        size_t decompressedSize = 100; // You may need to adjust this

        // Decompress the data
        VestTypes::DecompressedData decompressed = decompressData(compressedData, decompressedSize);
        std::string decompressedHeader(decompressed.data.begin(), decompressed.data.end());

        // Get the type (e.g., "tree", "blob")
        std::string type = decompressedHeader.substr(0, decompressedHeader.find(' '));
        PRINT_HIGHLIGHT("FILE_TYPE: " + type);

        // Determine and return the file type
        if (type == "tree") return VestTypes::TREE_FILE;
        if (type == "blob") return VestTypes::BLOB_FILE;

        throw std::runtime_error("Cannot determine the file type");
    }


    std::vector<unsigned char> readFile(std::string&& fPath) {
        return readFile(fPath);
    }

    std::vector<unsigned char> readFile(std::string& fPath) {
        std::ifstream file(fPath, std::ios::binary);

        if (!file) {
            PRINT_ERROR("FAILED TO OPEN FILE: " + fPath);
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