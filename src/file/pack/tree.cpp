#include <string>
#include <vector>

#include <utils.h>

#include <objects/helpers.h>
#include <objects/structs.h>

#include <file/file.h>
#include <file/pack/tree.h>


namespace VestPack {

    void processTree(
        VestObjects::CommitLinkedList* commitList,
        VestObjects::Tree*& treeClass,
        VestObjects::TreeNode* parent,
        VestObjects::PackIndex& packIndex,
        std::string& fContent,
        std::string& dir,
        bool& writeOnFile
    ) {

        VestTypes::TreeFile* treeFile = VestFile::readTreeFile(fContent);

        // We write the tree on the files
        std::string fileToWrite = "tree " + std::to_string(fContent.size()) + '\x00';
        fileToWrite += fContent;
        std::string sha1 = VestObjects::writeObject(fileToWrite, dir);
        packIndex.addSha1(sha1);

        PRINT_SML_SEPARATION;
        for (VestTypes::TreeFileLine* t : treeFile->tLines) {
            PRINT_TREE("TYPE: " + std::to_string(t->fType) + " NAME: " + t->fName + " SHA1: " + t->sha1());
        }
        PRINT_TREE("TREE SHA1: " + sha1 + " | SIZE: " + std::to_string(treeFile->tLines.size()));
        PRINT_SML_SEPARATION;

        PRINT_COMMIT("SHA1 : " + commitList->getCurrent()->commit->tSha1 + " | " + sha1);

        if (commitList->getCurrent()->commit->tSha1 == sha1 || parent == nullptr) {

            if (commitList->getCurrent()->commit->tSha1 == sha1) {

                // We gotta reset the tree
                PRINT_HIGHLIGHT("RESET TREE");
                delete treeClass;
                if (!commitList->isHead()) {
                    PRINT_WARNING("CHANGING WRITING");
                    writeOnFile = false;
                }

                treeClass = new VestObjects::Tree();
                commitList->incrementIndex();

            } else if (parent == nullptr) return;
        }

        PRINT_HIGHLIGHT("CHECKING IF ROOT");
        if (treeClass->getRoot() == nullptr) {
            treeClass->setRoot(treeFile, dir);
            return;
        }

        PRINT_HIGHLIGHT("GETTING THE CURRENT LINE");
        bool reRun {};
        VestTypes::TreeFileLine* treeLine = parent->getCurrentLine();
        if (sha1 != treeLine->sha1()) {

            if (!packIndex.exists(treeLine->sha1())) {
                PRINT_ERROR("TREE: NOT CURRENT SHA1: " + treeLine->sha1());
                throw std::runtime_error("");
            }

            // Modify this to add a better sha1 handleing
            PRINT_SUCCESS("TREE: SHA1 FOUND IN PACK_INDEX: " + treeLine->sha1());
            reRun = true;
        }

        PRINT_HIGHLIGHT("INCREMENTING INDEX");
        parent->incrementIndex();
        if (parent->isCompleted()) VestObjects::Tree::calculateAndSetIndex(treeClass, parent);

        if (reRun) {
            PRINT_TREE("RE-RUNNING");
            processTree(commitList, treeClass, treeClass->getIndex(), packIndex, fContent, dir, writeOnFile);
            return;
        }


        // We have to check the parent at this point, and mark that line as read
        VestObjects::TreeNode* currentNode = new VestObjects::TreeNode(treeFile, treeLine->fName, parent);
        parent->addChild(currentNode);

        // And we set the parent to be the current node;
        // Set here the Tree->index
        treeClass->setIndex(currentNode);

        // At this point we should create the folder if we are on the head:
        // Otherwise we should only return
        if (!writeOnFile) return;

        std::filesystem::path absPath = std::filesystem::absolute(currentNode->getPath());
        std::string path = absPath.string();

        bool isEmpty = true;

        if (std::filesystem::exists(path)) {
            isEmpty = std::filesystem::remove(path);
        }
        if (isEmpty) std::filesystem::create_directory(absPath);

        std::vector<std::string> entries {};

        // PRINT_TREE("TREE WRITTEN: " + sha1);
        // PRINT_SML_SEPARATION;
        // if (parent->isCompleted()) treeClass->setIndex(parent->parent);

    }

}