#ifndef VEST_PACK_TREE_H
#define VEST_PACK_TREE_H

#include <cstdint>
#include <vector>
#include <unordered_map>

#include <objects/structs.h>


namespace VestPack {

    void printTree(VestTypes::TreeFile* treeFile);

    void processTree(
        VestObjects::CommitLinkedList* commitList,
        VestObjects::Tree*& treeClass,
        VestObjects::TreeNode* parent,
        VestObjects::PackIndex& packIndex,
        std::string& fContent,
        std::string& dir,
        bool& writeOnFile
    );
}

#endif // VEST_PACK_TREE_H