#include <string>
#include <sstream>
#include <map>

#include <utils.h>

#include <file/types.h>
#include <file/utils.h>
#include <file/file.h>

namespace VestObjects {

    std::ostringstream computefPath(std::string& objSha1) {
        std::ostringstream path {};
        path << ".git/objects/" << objSha1[0] << objSha1[1] << "/" << objSha1.substr(2);
        return path;
    }

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

    std::string writeObject(std::string&& fContent) {
        return writeObject(fContent);
    }

    std::string prepareBlob(std::vector<unsigned char>& fContent) {
        // Allocate a vector to hold the header and file content
        std::string header = "blob " + std::to_string(fContent.size()) + '\0';
        header.append(fContent.begin(), fContent.end());
        return header;
    }

    std::string createBlob(std::string& fPath) {
        std::vector<unsigned char> fContent = VestFile::readFile(fPath);
        std::string blob = prepareBlob(fContent);

        std::string sha1 = writeObject(blob);
        return sha1;
    }

    std::string prepareCommit(std::string& fContent) {
        std::string header = "commit " + std::to_string(fContent.size()) + '\0';
        header.append(fContent.begin(), fContent.end());
        return header;
    }

    std::string createCommit(std::string& tSha1, std::string& parent, std::string commitMsg) {

        std::ostringstream fContent {};
        fContent << "tree " << tSha1 << "\x0A";

        if (!parent.empty()) fContent << "parent " << parent + "\x0A";

        fContent << "author root <root@Destiny.> 1735071151 +0100 \x0A";
        fContent << "committer root <root@Destiny.> 1735071151 +0100 \x0A";
        fContent << "\x0A";
        fContent << commitMsg << "\x0A";

        // Prepend the header (fContent type and size) to the fContent body
        std::string fContentStr = fContent.str();
        std::string commit = prepareCommit(fContentStr);

        // PRINT_HIGHLIGHT(commit);
        std::string sha1 = writeObject(commit);
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