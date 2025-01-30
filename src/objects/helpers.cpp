#include <sstream>
#include <vector>

#include <file/file.h>
#include <file/utils.h>


namespace VestObjects {

    std::ostringstream computefPath(std::string& objSha1) {
        std::ostringstream path {};
        path << ".git/objects/" << objSha1[0] << objSha1[1] << "/" << objSha1.substr(2);
        return path;
    }

    std::string writeObject(std::string& fContent, std::string dirRoot, std::string sha1) {
        // PRINT_HIGHLIGHT(std::to_string(fContent.size()));
        std::vector<unsigned char> compressedContent = VestFile::compressData(fContent);

        if (sha1.empty()) sha1 = VestFileUtils::computeSHA1(fContent);

        std::ostringstream pathToSaveFile {};
        pathToSaveFile << dirRoot + ".git/objects/" << sha1[0] << sha1[1];

        std::filesystem::create_directory(pathToSaveFile.str());
        pathToSaveFile << '/' << sha1.substr(2); // Create the directory

        VestFile::saveToFile(pathToSaveFile.str(), compressedContent);

        return sha1;
    }

    std::string writeObject(std::string&& fContent, std::string dirRoot, std::string sha1) {
        return writeObject(fContent, dirRoot, sha1);
    }

    std::string prepareObjectHeader(std::string& fContent, VestTypes::HeaderType objType) {
        std::string headerType = VestTypes::headerTypeToString(objType);

        std::string header = headerType + " " + std::to_string(fContent.size()) + '\x00';
        return header + fContent;

    }

    std::string prepareBlob(std::string& fContent) { return prepareObjectHeader(fContent, VestTypes::HeaderType::BLOB); }
    std::string prepareTree(std::string& fContent) { return prepareObjectHeader(fContent, VestTypes::HeaderType::TREE); }
    std::string prepareCommit(std::string& fContent) { return prepareObjectHeader(fContent, VestTypes::HeaderType::COMMIT); }

    std::string prepareBlob(std::vector<unsigned char>& fContent) {
        std::string header = "blob " + std::to_string(fContent.size()) + '\x00';
        header.append(fContent.begin(), fContent.end());
        return header;
    }




}