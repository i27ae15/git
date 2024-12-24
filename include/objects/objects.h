#ifndef VEST_OBJECTS_H
#define VEST_OBJECTS_H

#include <string>
#include <file/types.h>

namespace VestObjects {

    std::string writeObject(std::string&& fContent);
    std::string writeObject(std::string& fContent);

    std::string prepareCommit(std::string& fContent);
    std::string createCommit(
        std::string& tSha1,
        std::string& parent,
        std::string commitMsg
    );

    std::string createTree(std::filesystem::path& root);

    std::string prepareBlob(std::vector<unsigned char>& fileContent);
    std::string createBlob(std::string& fPath);

}

#endif // VEST_OBJECTS_H