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

        // verify that in fact the first bit is a 1
        uint8_t cByte = fContent[offset];
        if ((cByte & 0x80) == 0) return; // MSB is 0, not a copy command.
        // PRINT_WARNING("IN COPY_DELTA");

        uint32_t startCopyAt = 0;
        uint32_t bytesToCopy = 0;

        // We'll consume bytes from fContent after cByte:
        offset++;

        // Calculating the offset - bits : 0 - 3
        {
            // If bit0 is set, read one byte for offset’s low 8 bits
            if (cByte & 0x01) {
                startCopyAt |= (unsigned char)fContent.at(offset++) << 0;
            }
            // If bit1 is set, read one byte for offset’s next 8 bits
            if (cByte & 0x02) {
                startCopyAt |= (unsigned char)fContent.at(offset++) << 8;
            }
            if (cByte & 0x04) {
                startCopyAt |= (unsigned char)fContent.at(offset++) << 16;
            }
            if (cByte & 0x08) {
                startCopyAt |= (unsigned char)fContent.at(offset++) << 24;
            }
        }

        // Calculating the size - bits : 4 - 6
        {
            if (cByte & 0x10) {
                bytesToCopy |= (unsigned char)fContent.at(offset++) << 0;
            }
            if (cByte & 0x20) {
                bytesToCopy |= (unsigned char)fContent.at(offset++) << 8;
            }
            if (cByte & 0x40) {
                bytesToCopy |= (unsigned char)fContent.at(offset++) << 16;
            }
            if (bytesToCopy == 0) {
                bytesToCopy = 0x10000;
            }
        }

        // At this point we have everything to start the copy into objBlob
        std::string toCopy {};
        for (uint32_t i = 0; i < bytesToCopy; i++) toCopy += baseBlob[startCopyAt + i];
        // PRINT_DELTA("TO COPY: ");
        // std::cout << toCopy << "\x0A";

        // PRINT_DELTA("RESULT OF COPY: ");
        objBlob += toCopy;
        // std::cout << objBlob << "\x0A";
    }

    void addDelta(
        std::string& blob,
        std::string& fContent,
        size_t& offset
    ) {

        if ((fContent[offset] & 0x80) != 0) return; // MSB is 1, not an add command.

        std::string toAdd = fContent.substr(offset + 1, static_cast<int>(fContent[offset]));
        blob += toAdd; offset += toAdd.size() + 1;

        // PRINT_SML_SEPARATION;
        // PRINT_DELTA("TO ADD: ");
        // std::cout << toAdd << "\x0A";

        // PRINT_DELTA("RESULT: ");
        // std::cout << blob << "\x0A";
        // PRINT_SML_SEPARATION;

    }

    void processRefDelta(
        VestObjects::TreeNode* parent,
        size_t& offset,
        std::string& dir,
        std::string& baseBlob,
        std::vector<uint8_t>& rData,
        bool& writeOnFile,
        bool& checkDelta
    ) {

        size_t originalOffset = offset;

        // Let's get the SHA1 for the ref object
        std::string refDeltaSha1 {};
        for (uint8_t _ {}; VestTypes::SHA_BYTES_SIZE > _; _++) {
            refDeltaSha1 += VestFileUtils::byteToHex(rData.at(offset));
            offset++;
        }

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

        std::string newBlob {};
        bool inRef {true};

        // Now, we have to start copying from the blob or add the new values
        while (inRef && rSize > newBlob.size()) {

            uint8_t cByte = fContent[internalOffset];

            switch (cByte & 0x80) {
                case 0x80:
                    (void)copyDelta(baseBlob, newBlob, fContent, internalOffset);
                    break;

                case 0x00:
                    (void)addDelta(newBlob, fContent, internalOffset);
                    break;

                default:
                    PRINT_WARNING("GETTING OUT");
                    inRef = false;
                    break;
            }

        }

        // At this point our newBlob object should be completed, so we need to compute the SHA1
        // Of it, write the REF_DELTA under the same SHA1 with the newBlob content, not with the
        // content of the REF_DELTA
        baseBlob = newBlob;

        // Compute hash
        std::string dataToComputeSha1 = "blob " + std::to_string(newBlob.size()) + '\x00';
        dataToComputeSha1 += newBlob;
        std::string sha1 = VestFileUtils::computeSHA1(dataToComputeSha1);
        (void)VestObjects::writeObject(
            std::string(rData.begin() + originalOffset, rData.end()), dir, sha1
        );

        // Check if the sha1 is the same as the one we have on queue
        VestTypes::TreeFileLine* treeLine = parent->getCurrentLine();
        PRINT_DELTA("SHA1 TO WRITE: " + sha1);
        if (sha1 != treeLine->sha1()) return; // This indicates that there must be another delta to parse
        // If we reach here, we have found the correct sha1, so we can increment the line

        checkDelta = false;
        parent->incrementIndex();
        std::string path = parent->getPath() + treeLine->fName;
        PRINT_DELTA("PATH TO WRITE: " + path);

        VestFile::saveToFile(path, {fContent.begin(), fContent.end()});

        PRINT_DELTA("DELTA WRITTEN: " + sha1);
        PRINT_SML_SEPARATION;

    }

    void processCommit(
        VestObjects::CommitLinkedList* commitList,
        std::string& fContent,
        std::string& dir
    ) {
        VestTypes::CommitFile* commitFile = VestFile::readCommit(fContent);
        commitList->addNode(commitFile);
        PRINT_COMMIT("COMMIT SHA1 WRITTEN: " + VestObjects::createCommit(fContent, dir));
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

        PRINT_SML_SEPARATION;
        for (VestTypes::TreeFileLine* t : treeFile->tLines) {
            PRINT_TREE("TYPE: " + std::to_string(t->fType) + " NAME: " + t->fName + " SHA1: " + t->sha1());
        }
        PRINT_TREE("TREE SHA1: " + sha1);
        PRINT_SML_SEPARATION;

        if (treeClass->getRoot() == nullptr) {
            treeClass->setRoot(treeFile, dir);
            return;
        }

        VestTypes::TreeFileLine* treeLine = parent->getCurrentLine();
        if (sha1 != treeLine->sha1()) {
            PRINT_ERROR("TREE: NOT CURRENT SHA1: " + treeLine->sha1());
            throw std::runtime_error("");
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
        PRINT_TREE("PATH TO WRITE: " + path);

        std::filesystem::create_directory(path);

        PRINT_TREE("TREE WRITTEN: " + sha1);
        PRINT_SML_SEPARATION;

    }

    void processBlob(
        VestObjects::TreeNode* parent,
        std::string& fContent,
        std::string& dir,
        std::string& lastBlob,
        bool& writeOnFile,
        bool& checkDelta
    ) {

        std::string fileToWrite = "blob " + std::to_string(fContent.size()) + '\x00';
        fileToWrite += fContent;
        lastBlob = fContent;

        std::string sha1 = VestObjects::writeObject(fileToWrite, dir);
        PRINT_BLOB("BLOB SHA1: " + sha1);

        VestTypes::TreeFileLine* treeLine = parent->getCurrentLine();
        if (sha1 != treeLine->sha1()) {
            PRINT_ERROR("BLOB: NOT CURRENT SHA1: " + treeLine->sha1());
            checkDelta = true;
            return;
        }
        parent->incrementIndex();

        if (!writeOnFile) return;

        std::string path = parent->getPath() + treeLine->fName;
        PRINT_BLOB("PATH TO WRITE: " + path);

        // VestFile::saveToFile(path, {fContent.begin(), fContent.end()});

        PRINT_BLOB("BLOB WRITTEN: " + sha1);
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
        bool mustBeDelta {};

        for (uint32_t i {}; i < nObjects; i++) {

            ObjectHeader objHeader = parseObjectHeader(rData, _offset);

            if (objHeader.type == VestTypes::REF_DELTA) {
                PRINT_DELTA("PROCESSING REF_DELTA");
                (void)processRefDelta(tree->getIndex(), _offset, dir, lastBlob, rData, isHead, mustBeDelta);
                continue;
            }

            if (mustBeDelta) {
                PRINT_ERROR("NEXT FILE MUST BE DELTA AND IS NOT!");
                throw std::runtime_error("");
            }

            std::string fContent {};
            (void)setFileContent(_offset, rData, _offset, fContent);

            switch (objHeader.type) {
                case VestTypes::COMMIT:
                    PRINT_COMMIT("PROCESSING COMMIT");
                    (void)processCommit(commitList, fContent, dir);
                    break;

                case VestTypes::TREE:
                    PRINT_TREE("PROCESSING TREE");
                    (void)processTree(tree, tree->getIndex(), fContent, dir, isHead);
                    break;

                case VestTypes::BLOB:
                    PRINT_BLOB("PROCESSING BLOB");
                    (void)processBlob(tree->getIndex(), fContent, dir, lastBlob, isHead, mustBeDelta);
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