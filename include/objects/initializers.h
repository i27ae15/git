#ifndef VEST_OBJECTS_INITIALIZERS_H
#define VEST_OBJECTS_INITIALIZERS_H

#include <string>
#include <file/types.h>

namespace VestObjects {

    uint8_t initializeVest();

    std::string createCommit(
        std::string& tSha1,
        std::string& parent,
        std::string commitMsg
    );

    std::string createTree(std::filesystem::path& root);
    std::string createBlob(std::string& fPath);
}

#endif // VEST_OBJECTS_INITIALIZERS_H