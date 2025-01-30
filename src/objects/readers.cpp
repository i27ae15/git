#include <string>
#include <vector>

#include <utils.h>

#include <file/file.h>
#include <file/utils.h>
#include <file/types.h>

#include <objects/readers.h>

namespace VestObjects {

    ObjectRead readObject(std::string& sha1) {
        std::string root = "";
        return readObject(sha1, root);
    }

    ObjectRead readObject(std::string& sha1, std::string& dir) {

        ObjectRead objRead {};

        std::string root = dir + ".git/objects/";
        std::string fPath = VestFileUtils::constructfPath(sha1, root);

        std::vector<uint8_t> vContent = VestFile::readFile(fPath);
        VestTypes::DecompressedData dData = VestFile::decompressData(vContent);

        objRead.fContent = std::string(dData.data.begin(), dData.data.end());

        std::string type = objRead.fContent.substr(0, objRead.fContent.find(' '));
        objRead.fContent = objRead.fContent.substr(objRead.fContent.find('\x00') + 1);

        // Determine and return the file type
        if (type == "tree") objRead.setType(VestTypes::TREE);
        else if (type == "blob") objRead.setType(VestTypes::BLOB);
        else {
            PRINT_ERROR("WRONG FILE TYPE");
            throw std::runtime_error("");
        }

        return objRead;
    }

    VestTypes::CommitFile* readCommit(std::string& fContent, bool fromPack) {

        std::cout << fContent << "\x0A";

        VestTypes::CommitFile* commit = new VestTypes::CommitFile();
        std::string* tps[5] = {
            &commit->tSha1,
            &commit->pSha1,
            &commit->author,
            &commit->commiter,
            &commit->commitMsg
        };

        uint8_t toWrite {};

        for (uint16_t i {}; i < fContent.size(); i++) {

            if (fContent[i] == '\x0A') continue;

            if (toWrite == 1 && fContent[i] != 'p') toWrite++;
            std::string* _using = tps[toWrite];

            while (fContent[i] != '\x0A') {
                *_using += fContent[i];
                i++;
            }

            if (fromPack) {

                switch (toWrite) {
                    case 0:
                        if (_using->find("tree ") != std::string::npos) {
                            *_using = _using->substr(5);
                        }
                        break;

                    case 1:
                        if (_using->find("parent") != std::string::npos) {
                            *_using = _using->substr(7);
                        }

                    default:
                        break;
                }

            }

            toWrite++;
        }

        return commit;
    }

    VestTypes::TreeFile* readTreeFile(std::string& fContent) {

        VestTypes::TreeFile* tree = new VestTypes::TreeFile();

        for (uint16_t i {}; i < fContent.size(); i++) {
            std::string fType {};
            std::string fName {};

            std::string* _using = &fType;

            for (uint16_t j {i}; j < fContent.size(); j++) {

                char& c = fContent[j];

                if (c == ' ') {_using = &fName; continue;}

                *_using += c;
                if (c == '\x00') {
                    i += j - i;
                    break;
                }
            }

            tree->addLine(fType, fName, fContent.substr(i + 1, VestTypes::SHA_BYTES_SIZE));
            i += VestTypes::SHA_BYTES_SIZE;
        }

        return tree;
    }

}