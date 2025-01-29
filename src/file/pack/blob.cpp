#include <string>
#include <vector>

#include <utils.h>

#include <objects/helpers.h>
#include <objects/structs.h>

#include <file/file.h>
#include <file/pack/blob.h>


namespace VestPack {

    void processBlob(
        VestObjects::Tree* treeClass,
        VestObjects::TreeNode* parent,
        VestObjects::PackIndex& packIndex,
        std::string& fContent,
        std::string& dir,
        bool& writeOnFile,
        bool& checkDelta
    ) {

        std::string fileToWrite = "blob " + std::to_string(fContent.size()) + '\x00';
        fileToWrite += fContent;

        std::vector<uint8_t> vContent {};
        std::string sha1 = VestObjects::writeObject(fileToWrite, dir);

        VestTypes::TreeFileLine* treeLine = parent->getCurrentLine();
        bool reRun {};
        if (sha1 != treeLine->sha1()) {

            if (!packIndex.exists(treeLine->sha1())) {
                // PRINT_ERROR("BLOB: NOT CURRENT SHA1: " + treeLine->sha1() + " | AND BLOB: " + sha1);
                checkDelta = true;
                return;
            }

            // Modify this to add a better sha1 handleing
            // PRINT_SUCCESS("BLOB: SHA1 FOUND IN PACK_INDEX: " + treeLine->sha1());
            std::string path = parent->getPath() + treeLine->fName;

            std::string fContent = treeLine->sha1();
            if (writeOnFile) VestFile::saveToFile(path, {fContent.begin(), fContent.end()});
            reRun = true;
        }

        parent->incrementIndex();
        if (reRun) {
            processBlob(treeClass, parent, packIndex, fContent, dir, writeOnFile, checkDelta);
            return;
        }

        packIndex.addSha1(sha1);

        // PRINT_SML_SEPARATION;
        if (parent->isCompleted()) VestObjects::Tree::calculateAndSetIndex(treeClass, parent);

        if (!writeOnFile) return;
        PRINT_BLOB("BLOB WRITTEN: " + sha1);
        std::string path = parent->getPath() + treeLine->fName;
        VestFile::saveToFile(path, {fContent.begin(), fContent.end()});
    }
}
