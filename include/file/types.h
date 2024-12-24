#ifndef VEST_TYPE_H
#define VEST_TYPE_H

#include <string>
#include <fstream>
#include <cstdint>
#include <filesystem>
#include <vector>
#include <zlib.h>


namespace VestTypes {
    constexpr const uint32_t BLOB_FILE = 100644;
    constexpr const uint32_t TREE_FILE = 040000;

    constexpr const char* BLOB_FILE_STR = "100644";
    constexpr const char* TREE_FILE_STR = "40000";

    constexpr const size_t DECOMPRESSED_SIZE = 1024 * 1024;
    constexpr const uint8_t SHA_BYTES_SIZE = 20;

    struct FileType {
        uint32_t iStr;
        const char* mode;
        const char* name;
    };

    constexpr FileType BLOB_F{BLOB_FILE, BLOB_FILE_STR, "blob"};
    constexpr FileType TREE_F{TREE_FILE, TREE_FILE_STR, "tree"};

    struct TreeObject {
        std::string sha1;
        std::string fContent;
    };

    struct DecompressedData {
        std::vector<unsigned char> data;
        z_stream stream;

        bool isEmpty();
    };
}

#endif // VEST_TYPE_H