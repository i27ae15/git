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
#include <file/utils.h>

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

    void setFileContent(
        const size_t& startAt,
        std::vector<uint8_t>& rData,
        size_t& offset,
        std::string& fContent
    ) {
        std::vector<uint8_t> nextObject(rData.begin() + startAt, rData.end());
        VestTypes::DecompressedData dData = VestFile::decompressData(
            nextObject, VestTypes::EXPAND_AS_NEEDED
        );

        offset += dData.compressedUsed;
        fContent = std::string(dData.data.begin(), dData.data.end());
    }

    void copyDelta(
        std::string& baseBlob,
        std::string& objBlob,
        std::string& fContent,
        size_t& offset
    ) {

        // verify that infact the first bit is a 1
        uint8_t cByte = fContent[offset];
        if ((cByte & 0x80) == 0) return; // MSB is 0, not a copy command.

        // PRINT_WARNING("IN COPY_DELTA");

        PRINT_HIGHLIGHT("BINARY:" + VestFileUtils::byteToBinary(cByte));
        PRINT_HIGHLIGHT("HEX:" + VestFileUtils::byteToHex(cByte));

        // We know bit7 == 1 -> copy command.
        uint32_t offsetVal = 0;
        uint32_t sizeVal   = 0;

        // We'll consume bytes from fContent after cByte:
        size_t i = offset + 1;

        // If bit0 is set, read one byte for offset’s low 8 bits
        if (cByte & 0x01) {
            offsetVal |= (unsigned char)fContent.at(i++) << 0;
        }
        // If bit1 is set, read one byte for offset’s next 8 bits
        if (cByte & 0x02) {
            offsetVal |= (unsigned char)fContent.at(i++) << 8;
        }
        if (cByte & 0x04) {
            offsetVal |= (unsigned char)fContent.at(i++) << 16;
        }
        if (cByte & 0x08) {
            offsetVal |= (unsigned char)fContent.at(i++) << 24;
        }

        // For the size:
        if (cByte & 0x10) {
            sizeVal |= (unsigned char)fContent.at(i++) << 0;
        }
        if (cByte & 0x20) {
            sizeVal |= (unsigned char)fContent.at(i++) << 8;
        }
        if (cByte & 0x40) {
            sizeVal |= (unsigned char)fContent.at(i++) << 16;
        }
        if (sizeVal == 0) {
            sizeVal = 0x10000;
        }

        PRINT_HIGHLIGHT("OFFSET_val: " + std::to_string(offsetVal));
        PRINT_HIGHLIGHT("SIZE_VAL: " + std::to_string(sizeVal));

        offset++;

        auto combineBytes = [](std::string& fContent, size_t& offset, uint8_t bytesToCount) -> uint32_t {
            uint32_t value {};

            while (bytesToCount--) value = (value << 8) | fContent.at(offset++);
            return value;
        };

        uint32_t startCopyAt = combineBytes(fContent, offset, offsetVal);
        uint32_t bytesToCopy = combineBytes(fContent, offset, sizeVal);

        // std::cout << baseBlob << "\x0A";

        // At this point we have everything to start the copy into objBlob
        for (uint32_t i = 0; i < bytesToCopy; i++) objBlob += baseBlob[startCopyAt + i];
    }

    void addDelta(
        std::string& blob,
        std::string& fContent,
        size_t& offset
    ) {

        if (fContent[offset] & 0x80 != 0) return; // MSB is 1, not an add command.
        offset++;
        PRINT_WARNING("IN ADD_DELTA");

        std::string dataToAdd {};
        std::vector<uint8_t> v(fContent.begin(), fContent.end());
        // setFileContent(offset, v, offset, dataToAdd);

        blob += fContent.substr(offset);
    }

    void processRefDelta(
        size_t& offset,
        std::string& baseBlob,
        std::vector<uint8_t>& rData
    ) {

        // Let's get the SHA1 for the ref object
        std::string refDeltaSha1 {};
        size_t prevInt = offset;
        for (uint8_t _ {}; VestTypes::SHA_BYTES_SIZE > _; _++) {
            refDeltaSha1 += VestFileUtils::byteToHex(rData.at(offset));
            offset++;
        }

        // PRINT_HIGHLIGHT("REF_DELTA PARENT SHA1: " + refDeltaSha1);

        std::string fContent {};
        (void)setFileContent(offset, rData, offset, fContent);

        auto parseRefDeltaSizes = [](std::string& rData, size_t& offset) -> uint64_t {

            uint64_t value {};
            uint16_t shift {};

            while (rData.size() > offset) {
                uint8_t cByte = rData[offset++];

                value |= static_cast<uint64_t>(cByte & 0x7F) << shift;

                if ((cByte & 0x80) == 0) break; // If true MSB == 0, so we can terminate
                shift += 7;
            }

            return value;
        };

        size_t internalOffset {};

        // Let's grab the base size from the decompressedFcontent
        uint64_t bSize = parseRefDeltaSizes(fContent, internalOffset);
        uint64_t rSize = parseRefDeltaSizes(fContent, internalOffset);

        // PRINT_HIGHLIGHT("B_SIZE: " + std::to_string(bSize));
        // PRINT_HIGHLIGHT("R_SIZE: " + std::to_string(rSize));
        // PRINT_WARNING("OFFSET: " + std::to_string(offset));
        // PRINT_WARNING("INTERNAL_OFFSET: " + std::to_string(internalOffset));

        // std::cout << fContent << "\x0A";

        // Now, we have to start copying from the blob
        std::string newBlob {}; newBlob.resize(rSize);
        bool inRef {true};

        while (inRef) {

            uint8_t cByte = fContent[internalOffset];

            switch (cByte & 0x80) {
                case 0x80:
                    copyDelta(baseBlob, newBlob, fContent, internalOffset);
                    break;

                case 0x00:
                    addDelta(baseBlob, fContent, internalOffset);
                    break;

                default:
                    inRef = false;
                    break;
            }

        }

        // At this point or newBlob object shoudl be completed, so we need to compute the SHA1
        // Of it, write the REF_DELTA under the same SHA1 with the newBlob content, not with the
        // content of the REF_DELTA

        std::string sha1 = VestFileUtils::computeSHA1(newBlob);
        PRINT_HIGHLIGHT("SHA1 FOR REF_DELTA: " + sha1);
    }

    void processCommit(
        VestObjects::CommitLinkedList* commitList,
        std::string& fContent,
        std::string& dir
    ) {
        VestTypes::CommitFile* commitFile = VestFile::readCommit(fContent);
        commitList->addNode(commitFile);
        std::cout << fContent << "\x0A";
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
        PRINT_SML_SEPARATION;

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
        // Set here the Tree->index
        treeClass->setIndex(currentNode);

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
        std::string& lastBlob,
        bool& writeOnFile
    ) {

        fContent += "\x0A";

        int count {};
        int pink_c {};

        // for (char c : fContent) {
        //     count++;
        //     if (c == '\x0A') PRINT_WARNING_NO_SPACE("\\x0A");
        //     if (c == ' ') {
        //         PRINT_HIGHLIGHT_NO_SPACE(std::to_string(pink_c) + " ");
        //         pink_c++;
        //         continue;
        //     }
        //     std::cout << c;
        // }

        // PRINT_WARNING("TOTAL COUNT: " + std::to_string(count));

        std::string fileToWrite = "blob " + std::to_string(fContent.size()) + "\x00";
        fileToWrite += fContent;
        lastBlob = fContent;

        // std::cout << fileToWrite;
        // PRINT_SML_SEPARATION;

        std::string sha1 = VestObjects::writeObject(fileToWrite, dir);
        PRINT_COLOR(BLUE, "BLOB SHA1: " + sha1);

        VestTypes::TreeFileLine* treeLine = parent->getCurrentLine();
        if (sha1 != treeLine->sha1()) {
            PRINT_ERROR("BLOB: NOT CURRENT SHA1: " + treeLine->sha1());
            // throw std::runtime_error("");
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
        std::string lastBlob {};

        bool isHead {true};
        bool p {true};

        for (uint32_t i {}; i < nObjects; i++) {

            ObjectHeader objHeader = parseObjectHeader(rData, _offset);

            if (objHeader.type == VestTypes::REF_DELTA) {
                (void)processRefDelta(_offset, lastBlob, rData);
                continue;
            }

            std::string fContent {};
            (void)setFileContent(_offset, rData, _offset, fContent);

            switch (objHeader.type) {
                case VestTypes::COMMIT:
                    (void)processCommit(commitList, fContent, dir);
                    break;

                case VestTypes::TREE:
                    (void)processTree(tree, tree->getIndex(), fContent, dir, isHead);
                    break;

                case VestTypes::BLOB:
                    (void)processBlob(tree->getIndex(), fContent, dir, lastBlob, isHead);
                    break;

                case VestTypes::OFS_DELTA:
                    PRINT_WARNING("OFS_DELTA - DOING NOTHING");
                    break;

                default:
                    PRINT_ERROR("NOT VALID TYPE FOUND");
                    break;
            }

            if (i == 10) break;
        }

    }
}