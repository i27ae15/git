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

    void writeCommit(
        VestObjects::CommitLinkedList* commitList,
        std::string& fContent,
        std::string& dir
    ) {
        VestTypes::CommitFile* commitFile = VestFile::readCommit(fContent);
        commitList->addNode(commitFile);
        PRINT_WARNING("SHA1 WRITTEN: " + VestObjects::createCommit(fContent, dir));
    }

    void writeTree(VestObjects::Tree* tree, std::string& fContent, std::string dir) {

    }

    void createFiles(uint8_t& fType, std::string& fContent, std::string& dir) {

        std::string sha1 {};
        std::string fileToWrite {};

        VestTypes::TreeFile tFile {};

        switch (fType) {
            case VestTypes::TREE:

                tFile = VestFile::readTreeFile(fContent);

                PRINT_HIGHLIGHT("TREE FILE");
                for (VestTypes::TreeFileLine* t : tFile.tLines) {
                    PRINT_HIGHLIGHT("TYPE: " + std::to_string(t->fType) + " NAME: " + t->fName + " SHA1: " + t->sha1());
                }
                PRINT_SML_SEPARATION;

                fileToWrite = "tree " + std::to_string(fContent.size()) + '\x00';
                fileToWrite += fContent;

                sha1 = VestObjects::writeObject(fileToWrite, dir);
                PRINT_COLOR(BLUE, "SHA1 WRITTEN: " + sha1);
                break;

            case VestTypes::BLOB:
                fileToWrite = "blob " + std::to_string(fContent.size()) + '\x00';
                fileToWrite += fContent;

                sha1 = VestObjects::writeObject(fileToWrite, dir);
                PRINT_COLOR(GREEN, "SHA1 WRITTEN: " + sha1);
                break;

            default:
                break;
        }

        if (fType == VestTypes::TREE || fType == VestTypes::BLOB) {
            PRINT_BIG_SEPARATION;
        }
    }

    ObjectHeader processFile(std::vector<uint8_t>& rData, size_t& offset, std::string& fContent) {

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

    void processCommits(std::vector<uint8_t>& rData, size_t& offset, uint32_t& index, std::string& dir) {

        std::string fContent {};
        ObjectHeader ObjectHeader {};

        while (true) {
            ObjectHeader = processFile(rData, offset, fContent);
        }


    }

    void processPack(std::vector<uint8_t>& rData, size_t offset, std::string& dir) {

        size_t _offset = offset;
        uint32_t nObjects = parsePackHeader(rData, _offset);

        VestObjects::CommitLinkedList* commitList = new VestObjects::CommitLinkedList();
        VestObjects::Tree* tree = new VestObjects::Tree();

        bool isHead {true};
        bool p {true};

        std::string fContent {};
        for (uint32_t i {}; i < nObjects; i++) {

            ObjectHeader objHeader = processFile(rData, _offset, fContent);

            switch (objHeader.type) {
                case VestTypes::COMMIT:
                    writeCommit(commitList, fContent, dir);
                    break;

                default:
                    if (p) { PRINT_HIGHLIGHT("PRINTING COMMITS"); commitList->printCommits(); }
                    p = false;
                    createFiles(objHeader.type, fContent, dir);
                    break;
            }

            if (i == 10) break;
        }

    }
}