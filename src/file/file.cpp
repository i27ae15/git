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
#include <file/utils.h>
#include <file/pack/pack.h>


namespace VestFile {

    void printAsHex(const char* buffer, std::size_t length) {
        for (std::size_t i = 0; i < length; ++i) {
            std::cout << std::hex << std::setw(2) << std::setfill('0')
                    << (static_cast<unsigned int>(buffer[i]) & 0xFF) << " ";
        }
        std::cout << std::endl;
    }

    void saveToFile(const std::string& fPath, const std::vector<uint8_t>& content) {

        std::filesystem::path absolutePath = std::filesystem::absolute(fPath);
        std::ofstream outFile(absolutePath, std::ios::binary);
        outFile.write(reinterpret_cast<const char*>(content.data()), content.size());

        if (!outFile) {
            PRINT_ERROR("FAILED TO OPEN " + absolutePath.string());
            throw std::runtime_error("");
        }

        std::vector<uint8_t> fContent = VestFile::readFile(absolutePath.string());
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

        // Read a portion of the file for decompression (e.g., 30 bytes)
        std::vector<uint8_t> compressedData(
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

        // Determine and return the file type
        if (type == "tree") return VestTypes::TREE;
        if (type == "blob") return VestTypes::BLOB;

        throw std::runtime_error("Cannot determine the file type");
    }

    std::vector<uint8_t> readFile(std::string&& fPath) {
        return readFile(fPath);
    }

    std::vector<uint8_t> readFile(std::string& fPath) {
        std::filesystem::path absolutePath = std::filesystem::absolute(fPath);
        std::ifstream file(absolutePath, std::ios::binary);

        if (!file) {
            PRINT_ERROR("FAILED TO OPEN FILE: " + absolutePath.string());
            throw std::runtime_error("");
        }

        std::vector<uint8_t> fileContent (
            (std::istreambuf_iterator<char>(file)),
            std::istreambuf_iterator<char>()
        );

        file.close();

        return fileContent;
    }

    std::vector<uint8_t> compressData(std::string& inputData) {
        std::vector<uint8_t> data(inputData.begin(), inputData.end());
        return compressData(data);
    }

    std::vector<uint8_t> compressData(
        const std::vector<uint8_t>& inputData
    ) {
        uLongf compressedSize = compressBound(inputData.size());
        std::vector<uint8_t> compressedData(compressedSize);

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
        std::vector<uint8_t>& compressedData,
        size_t size
    ) {

        bool performResize {};
        if (size == VestTypes::EXPAND_AS_NEEDED) {
            size = VestTypes::KB;
            performResize = true;
        }

        std::vector<uint8_t> uncompressed(size); // KB

        z_stream stream {};
        inflateInit(&stream);

        stream.next_in = &compressedData[0];
        stream.avail_in = compressedData.size();  // all remaining bytes in the pack
        stream.next_out = uncompressed.data();
        stream.avail_out = uncompressed.size();

        size_t uSize {size};

        // Keep decompressing until we have exactly 'objHeader.size' bytes output
        while (true) {
            int status = inflate(&stream, Z_NO_FLUSH);

            if (status == Z_STREAM_END) break;

            if (status != Z_OK) {
                if (performResize && stream.avail_out == 0) {
                    uncompressed.resize(uSize * 2);
                    stream.next_out = uncompressed.data() + uSize;
                    stream.avail_out = uSize;

                    uSize *= 2;
                    // PRINT_WARNING("ADDED MORE SPACE: " + std::to_string(uSize));
                }
                else {
                    inflateEnd(&stream);
                    PRINT_HIGHLIGHT("STATUS: " + std::to_string(status));
                    throw std::runtime_error("Zlib inflate error");
                }
            }
        }

        uncompressed.resize(stream.total_out);
        inflateEnd(&stream);

        size_t compressedUsed = compressedData.size() - stream.avail_in;
        return VestTypes::DecompressedData {uncompressed, stream, compressedUsed};
    }

    VestTypes::DecompressedData decompressData(std::vector<uint8_t>& compressedData) {
        return decompressData(compressedData, VestTypes::EXPAND_AS_NEEDED);
    }

}