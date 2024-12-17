#ifndef VEST_FILE_H
#define VEST_FILE_H

#include <string>
#include <fstream>


namespace Vest {

    void saveToFile(const std::string& filePath, const std::vector<unsigned char>& content);

    std::string computeSHA1(const std::vector<unsigned char>& data);

    std::vector<unsigned char> readFile(std::string& filePath);
    std::vector<unsigned char> readFile(std::string&& filePath);
    std::vector<unsigned char> compressData(const std::vector<unsigned char>& inputData);

}

#endif // VEST_FILE_H