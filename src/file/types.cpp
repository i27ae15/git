#include <iostream>

#include <utils.h>

#include <file/types.h>
#include <file/utils.h>

namespace VestTypes {

    bool DecompressedData::isEmpty() {
        return data.empty();
    }

    void TreeFile::addLine(uint8_t fType, std::string fName, std::string bSha1) {
        tLines.push_back(new TreeFileLine(fType, fName, bSha1));
    }

    void TreeFile::addLine(std::string fType, std::string fName, std::string bSha1) {
        tLines.push_back(new TreeFileLine(fType, fName, bSha1));
    }

    TreeFileLine::TreeFileLine() : bSha1 {}, fType {}, fName{}, sSha1 {} {}

    TreeFileLine::TreeFileLine(std::string fType, std::string fName, std::string bSha1)
        : TreeFileLine((fType == VestTypes::BLOB_FILE_STR) ? VestTypes::BLOB
        : (fType == VestTypes::TREE_FILE_STR) ? VestTypes::TREE
        : 0, // Default or error value
        fName, bSha1
    ) {}

    TreeFileLine::TreeFileLine(uint8_t fType, std::string fName, std::string bSha1) :
        bSha1 {bSha1}, fType {fType}, fName{fName}, sSha1 {} {}

    std::string TreeFileLine::sha1() {
        if (!sSha1.empty()) return sSha1;

        std::ostringstream oss;
        for (uint8_t i {}; i < bSha1.size(); i++) {
            oss << VestFileUtils::byteToHex(bSha1[i]);
        }

        sSha1 = oss.str();
        return sSha1;
    }

    void CommitFile::printCommitFile() {
        std::cout << tSha1     << '\x0A';
        if (!pSha1.empty()) std::cout << pSha1     << '\x0A';
        std::cout << author    << '\x0A';
        std::cout << commiter  << '\x0A';
        std::cout << '\x0A';
        std::cout << commitMsg << '\x0A';
        std::cout << '\x0A';
    }

    void TreeFile::printTree() {

        PRINT_SML_SEPARATION;
        for (VestTypes::TreeFileLine* t : tLines) {
            PRINT_TREE("TYPE: " + std::to_string(t->fType) + " NAME: " + t->fName + " SHA1: " + t->sha1());
        }
        PRINT_TREE("TREE SHA1: " + sha1 + " | SIZE: " + std::to_string(tLines.size()));
        PRINT_SML_SEPARATION;

    }

    std::string headerTypeToString(HeaderType objType) {
        std::string value {};

        switch (objType) {
            case HeaderType::BLOB:   value = "blob";   break;
            case HeaderType::TREE:   value = "tree";   break;
            case HeaderType::COMMIT: value = "commit"; break;
        }

        return value;
    }
}