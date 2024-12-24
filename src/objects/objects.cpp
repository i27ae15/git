#include <string>
#include <sstream>
#include <map>

#include <utils.h>

#include <file/types.h>
#include <file/utils.h>
#include <file/file.h>

namespace VestObjects {

    std::string writeObject(std::string& fContent) {
        // PRINT_HIGHLIGHT(std::to_string(fContent.size()));
        std::vector<unsigned char> compressedContent = VestFile::compressData(fContent);
        std::string sha1 = VestFileUtils::computeSHA1(fContent);

        std::ostringstream pathToSaveFile {};
        pathToSaveFile << ".git/objects/" << sha1[0] << sha1[1];

        std::filesystem::create_directory(pathToSaveFile.str());
        pathToSaveFile << '/' << sha1.substr(2); // Create the directory

        VestFile::saveToFile(pathToSaveFile.str(), compressedContent);

        return sha1;
    }

    std::string prepareBlob(std::vector<unsigned char>& fileContent) {
        // Allocate a vector to hold the header and file content
        std::vector<unsigned char> blobData;
        std::string header = "blob " + std::to_string(fileContent.size()) + '\0';
        header.append(fileContent.begin(), fileContent.end());
        return header;
    }

    std::string createBlob(std::string& filePath) {
        std::vector<unsigned char> fContent = VestFile::readFile(filePath);
        std::string blob = prepareBlob(fContent);

        std::string sha1 = writeObject(blob);
        return sha1;
    }

    std::string createTree(std::filesystem::path& root) {

        size_t cSize {};
        std::map<std::string, std::string> objs {};

        VestTypes::FileType fType {};

        for (std::filesystem::directory_entry entry : std::filesystem::directory_iterator(root)) {
            std::string cSha1 {};
            std::string cPath {entry.path().string()};
            std::string fileName {entry.path().filename().string()};

            if (fileName == ".git") continue;

            if (entry.is_directory()) {
                std::filesystem::path p {entry.path()};
                std::cerr << "NEXT_PATH: " + p.string() << '\x0A';
                cSha1 = createTree(p);
                fType = VestTypes::TREE_F;
            }

            if (entry.is_regular_file()) {
                cSha1 = createBlob(cPath);
                fType = VestTypes::BLOB_F;
            }

            objs[fileName] = VestFileUtils::constructFileLine(fType, cSha1, fileName);
            cSize += VestTypes::SHA_BYTES_SIZE  // 20 bytes for SHA-1
            + fileName.size()            // size of the filename
            + 1                          // null terminator (\0)
            + (fType.mode == VestTypes::TREE_FILE_STR ? 5 : 6) // mode length
            + 1;                         // space separator

        }

        cSize--; // I don't know why I have to do this.

        std::string fContent = "tree " + std::to_string(cSize + 1) + '\x00';
        for (std::map<std::string, std::string>::iterator it {objs.begin()}; it != objs.end(); ++it) {
            fContent += it->second;
        }
        std::cerr << fContent << "\x0A";
        std::string sha1 = writeObject(fContent);

        return sha1;
    }


}