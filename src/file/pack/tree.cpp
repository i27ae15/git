#include <string>
#include <vector>

#include <utils.h>

#include <objects/helpers.h>
#include <objects/structs.h>
#include <objects/readers.h>

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

        VestTypes::TreeFile* treeFile = VestObjects::readTreeFile(fContent);
        std::string fileToWrite = VestObjects::prepareTree(fContent);
        std::string sha1 = VestObjects::writeObject(fileToWrite, dir);

        (void)packIndex.addSha1(sha1);
        treeFile->sha1 = sha1;

        bool isCommitTree { commitList->getCurrent()->commit->tSha1 == sha1 };
        if (isCommitTree) {

            // We gotta reset the tree
            delete treeClass;
            if (!commitList->isHead()) writeOnFile = false;

            treeClass = new VestObjects::Tree();
            (void)commitList->incrementIndex();

        } else if (parent == nullptr) { return; }

        if (treeClass->getRoot() == nullptr) {
            (void)treeClass->setRoot(treeFile, dir);
            return;
        }

        bool reRun {};
        VestTypes::TreeFileLine* treeLine = parent->getCurrentLine();
        if (sha1 != treeLine->sha1()) {

            if (!packIndex.exists(treeLine->sha1())) {
                throw std::runtime_error("");
            }
            // Modify this to add a better sha1 handleing
            reRun = true;
        }

        (void)parent->incrementIndex();
        if (parent->isCompleted()) (void)VestObjects::Tree::calculateAndSetIndex(treeClass, parent);
        if (reRun) {
            (void)processTree(commitList, treeClass, treeClass->getIndex(), packIndex, fContent, dir, writeOnFile);
            return;
        }

        // We have to check the parent at this point, and mark that line as read
        VestObjects::TreeNode* currentNode = new VestObjects::TreeNode(treeFile, treeLine->fName, parent);
        (void)parent->addChild(currentNode);

        // And we set the parent to be the current node;
        // Set here the Tree->index
        (void)treeClass->setIndex(currentNode);

        // At this point we should create the folder if we are on the head:
        // Otherwise we should only return
        if (!writeOnFile) return;

        std::filesystem::path absPath = std::filesystem::absolute(currentNode->getPath());
        if (!std::filesystem::exists(absPath)) (void)std::filesystem::create_directory(absPath);
    }

}