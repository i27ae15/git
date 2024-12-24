#ifndef VEST_OBJECTS_H
#define VEST_OBJECTS_H

#include <string>
#include <file/types.h>

namespace VestObjects {

    std::string writeObject(std::string& fContent);

    std::string createTree(std::filesystem::path& root);

    std::string prepareBlob(std::vector<unsigned char>& fileContent);
    std::string createBlob(std::string& filePath);

}

#endif // VEST_OBJECTS_H