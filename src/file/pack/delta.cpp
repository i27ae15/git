#include <cstdint>

#include <file/pack/delta.h>
#include <file/file.h>
#include <file/utils.h>

#include <objects/helpers.h>
#include <objects/readers.h>

#include <file/pack/tree.h>
#include <file/pack/delta.h>
#include <file/pack/helpers.h>


namespace VestPack {

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
        VestObjects::CommitLinkedList* commitList,
        VestObjects::Tree*& treeClass,
        VestObjects::TreeNode* parent,
        VestObjects::PackIndex& packIndex,
        size_t& offset,
        std::string& dir,
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

        // PRINT_DELTA("SHA1_PARENT: " + refDeltaSha1);
        VestObjects::ObjectRead objRead = VestObjects::readObject(refDeltaSha1, dir);

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

        std::string newFile {};
        bool inRef {true};

        // Now, we have to start copying from the blob or add the new values
        while (inRef && rSize > newFile.size()) {

            uint8_t cByte = fContent[internalOffset];

            switch (cByte & 0x80) {
                case 0x80:
                    (void)copyDelta(objRead.fContent, newFile, fContent, internalOffset);
                    break;

                case 0x00:
                    (void)addDelta(newFile, fContent, internalOffset);
                    break;

                default:
                    inRef = false;
                    break;
            }

        }

        // At this point our file object should be completed, so we need to compute the SHA1
        // Of it, write the REF_DELTA under the same SHA1 with the newBlob content, not with the
        // content of the REF_DELTA

        switch (objRead.getType()) {
            case VestTypes::TREE:
                processTree(commitList, treeClass, parent, packIndex, newFile, dir, writeOnFile);
                return;
            default:
                break;
        }
        // Compute hash
        std::string dataToComputeSha1 = objRead.getStrType() + " " + std::to_string(newFile.size()) + '\x00';
        dataToComputeSha1 += newFile;
        std::string sha1 = VestFileUtils::computeSHA1(dataToComputeSha1);
        packIndex.addSha1(sha1);

        (void)VestObjects::writeObject(dataToComputeSha1, dir, sha1);

        // Check if the sha1 is the same as the one we have on queue
        // The tree must be reset.
        VestTypes::TreeFileLine* treeLine = parent->getCurrentLine();

        if (sha1 != treeLine->sha1()) return; // This indicates that there must be another delta to parse
        // If we reach here, we have found the correct sha1, so we can increment the line
        parent->incrementIndex();
        if (parent->isCompleted()) treeClass->setIndex(parent->parent);

        checkDelta = false;

        if (!writeOnFile) return;

        std::string path = parent->getPath() + treeLine->fName;
        VestFile::saveToFile(path, {fContent.begin(), fContent.end()});
    }

}