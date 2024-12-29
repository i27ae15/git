#ifndef VEST_TYPE_H
#define VEST_TYPE_H

#include <string>
#include <fstream>
#include <cstdint>
#include <filesystem>
#include <vector>
#include <zlib.h>


namespace VestTypes {

    constexpr const uint8_t COMMIT = 1;
    constexpr const uint8_t TREE = 2;
    constexpr const uint8_t BLOB = 3;
    constexpr const uint8_t TAG = 4;
    constexpr const uint8_t OFS_DELTA = 6;
    constexpr const uint8_t REF_DELTA = 7;

    constexpr const char* BLOB_FILE_STR = "100644";
    constexpr const char* TREE_FILE_STR = "40000";

    constexpr const uint16_t KB = 1024;
    constexpr const uint32_t MB = KB * KB;
    constexpr const uint8_t SHA_BYTES_SIZE = 20;

    constexpr const uint8_t EXPAND_AS_NEEDED = 0;

    struct FileType {
        const char* mode;
        const char* name;
    };

    constexpr FileType BLOB_F{BLOB_FILE_STR, "blob"};
    constexpr FileType TREE_F{TREE_FILE_STR, "tree"};

    struct TreeObject {
        std::string sha1;
        std::string fContent;
    };

    struct DecompressedData {
        std::vector<unsigned char> data;
        z_stream stream;
        size_t compressedUsed;

        bool isEmpty();
    };
}

#endif // VEST_TYPE_H