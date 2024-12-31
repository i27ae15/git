#include <iostream>
#include <fstream>
#include <vector>
#include <stdexcept>
#include <cstdint>
#include <zlib.h>
#include <arpa/inet.h>
#include <openssl/sha.h>
#include <vector>
#include <sstream>
#include <iomanip>
#include <stdexcept>

#include <file/pack.h>
#include <file/file.h>
#include <file/types.h>

#include <objects/initializers.h>
#include <objects/helpers.h>
#include <objects/structs.h>

#include <utils.h>

namespace VestPack {

    ObjectHeader parseObjectHeader(const std::vector<uint8_t>& rData, size_t& offset) {
        if (offset >= rData.size()) {
            throw std::runtime_error("Invalid PACK file: Offset exceeds data size");
        }

        uint32_t size = 0;
        uint8_t type = 0;
        int shift = 0;

        // Read the first byte
        uint8_t currentByte = rData[offset++];
        type = (currentByte >> 4) & 0x07; // (1) Extract type (bits 6-4)
        size = currentByte & 0x0F;        // (2) Extract size (bits 3-0)

        // Process continuation bytes if MSB is set
        while (currentByte & 0x80) { // MSB (bit 7) is 1 => more bytes follow

            if (offset >= rData.size()) {
                throw std::runtime_error("Invalid PACK file: Header is incomplete");
            }
            currentByte = rData[offset++];
            // printHexAndBinary(currentByte);
            size |= (currentByte & 0x7F) << shift; // (3) Append 7 bits to size
            shift += 7;
        }

        ObjectHeader obj {type, offset, size};
        return obj;
    }

    ObjectHeader setFileContent(std::vector<uint8_t>& rData, size_t& offset, std::string& fContent) {

        ObjectHeader objHeader = parseObjectHeader(rData, offset);

        if (objHeader.type == VestTypes::REF_DELTA) {
            // PRINT_WARNING("REF_DELTA");
            objHeader.start += VestTypes::SHA_BYTES_SIZE;
            offset += VestTypes::SHA_BYTES_SIZE;
        }

        std::vector<uint8_t> nextObject(rData.begin() + objHeader.start, rData.end());
        VestTypes::DecompressedData dData = VestFile::decompressData(
            nextObject, VestTypes::EXPAND_AS_NEEDED
        );

        offset += dData.compressedUsed;
        fContent = std::string(dData.data.begin(), dData.data.end());
        return objHeader;
    }

    uint32_t parsePackHeader(const std::vector<uint8_t>& rData, size_t& offset) {

        // Read version and object count
        uint32_t version =  (static_cast<uint8_t>(rData[offset + 0]) << 24) |
                            (static_cast<uint8_t>(rData[offset + 1]) << 16) |
                            (static_cast<uint8_t>(rData[offset + 2]) << 8)  |
                             static_cast<uint8_t>(rData[offset + 3]);

        uint32_t nObjects = (static_cast<uint8_t>(rData[offset + 4]) << 24) |
                            (static_cast<uint8_t>(rData[offset + 5]) << 16) |
                            (static_cast<uint8_t>(rData[offset + 6]) << 8)  |
                             static_cast<uint8_t>(rData[offset + 7]);

        // std::cout << "PACK version: " << version << ", Object count: " << nObjects << std::endl;

        offset += 8;
        return nObjects;
    }

    void processCommit(
        VestObjects::CommitLinkedList* commitList,
        std::string& fContent,
        std::string& dir
    ) {
        VestTypes::CommitFile* commitFile = VestFile::readCommit(fContent);
        commitList->addNode(commitFile);
        PRINT_WARNING("COMMIT SHA1 WRITTEN: " + VestObjects::createCommit(fContent, dir));
    }

    void processTree(
        VestObjects::Tree* treeClass,
        VestObjects::TreeNode* parent,
        std::string& fContent,
        std::string& dir,
        bool& writeOnFile
    ) {

        VestTypes::TreeFile* treeFile = VestFile::readTreeFile(fContent);

        // We write the tree on the files
        std::string fileToWrite = "tree " + std::to_string(fContent.size()) + '\x00';
        fileToWrite += fContent;
        std::string sha1 = VestObjects::writeObject(fileToWrite, dir);

        PRINT_HIGHLIGHT("TREE SHA1: " + sha1);
        for (VestTypes::TreeFileLine* t : treeFile->tLines) {
            PRINT_HIGHLIGHT("TYPE: " + std::to_string(t->fType) + " NAME: " + t->fName + " SHA1: " + t->sha1());
        }

        if (treeClass->getRoot() == nullptr) {
            treeClass->setRoot(treeFile, dir);
            return;
        }

        VestTypes::TreeFileLine* treeLine = parent->getCurrentLine();
        if (sha1 != treeLine->sha1()) {
            PRINT_ERROR("TREE: NOT CURRENT SHA1: " + treeLine->sha1());
            throw std::runtime_error("");
            return;
        }
        parent->incrementIndex();

        // We have to check the parent at this point, and mark that line as read
        VestObjects::TreeNode* currentNode = new VestObjects::TreeNode(treeFile, treeLine->fName, parent);
        parent->addChild(currentNode);


        // And we set the parent to be the current node;
        parent = currentNode;

        // At this point we should create the folder if we are on the head:
        // Otherwise we should only return
        if (!writeOnFile) return;

        std::string path = currentNode->getPath();
        PRINT_HIGHLIGHT("PATH TO WRITE: " + path);

        std::filesystem::create_directory(path);

        PRINT_HIGHLIGHT("TREE WRITTEN: " + sha1);
        PRINT_SML_SEPARATION;

    }

    void processBlob(
        VestObjects::TreeNode* parent,
        std::string& fContent,
        std::string& dir,
        bool& writeOnFile
    ) {

        std::string fileToWrite = "blob " + std::to_string(fContent.size()) + '\x00';
        fileToWrite += fContent;

        std::cout << fileToWrite;

        std::string sha1 = VestObjects::writeObject(fileToWrite, dir);
        PRINT_COLOR(BLUE, "BLOB SHA1: " + sha1);

        VestTypes::TreeFileLine* treeLine = parent->getCurrentLine();
        if (sha1 != treeLine->sha1()) {
            PRINT_ERROR("BLOB: NOT CURRENT SHA1: " + treeLine->sha1());
            throw std::runtime_error("");
            return;
        }
        parent->incrementIndex();

        if (!writeOnFile) return;

        std::string path = parent->getPath() + treeLine->fName;
        PRINT_COLOR(BLUE, "PATH TO WRITE: " + path);

        VestFile::saveToFile(path, {fContent.begin(), fContent.end()});

        PRINT_COLOR(BLUE, "BLOB WRITTEN: " + sha1);
        PRINT_SML_SEPARATION;
    }

    void processPack(
        std::vector<uint8_t>& rData,
        size_t offset,
        std::string& dir
    ) {

        size_t _offset = offset;
        uint32_t nObjects = parsePackHeader(rData, _offset);

        VestObjects::CommitLinkedList* commitList = new VestObjects::CommitLinkedList();
        VestObjects::Tree* tree = new VestObjects::Tree();

        bool isHead {true};
        bool p {true};

        std::string fContent {};
        for (uint32_t i {}; i < nObjects; i++) {

            ObjectHeader objHeader = setFileContent(rData, _offset, fContent);

            switch (objHeader.type) {
                case VestTypes::COMMIT:
                    processCommit(commitList, fContent, dir);
                    break;

                case VestTypes::TREE:
                    processTree(tree, tree->getIndex(), fContent, dir, isHead);
                    break;

                case VestTypes::BLOB:
                    processBlob(tree->getIndex(), fContent, dir, isHead);
                    break;

                default:
                    PRINT_ERROR("NOT VALID TYPE FOUND");
                    break;
            }

            if (i == 10) break;
        }

    }
}