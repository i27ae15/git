#ifndef VEST_PACK_DELTA_H
#define VEST_PACK_DELTA_H

#include <string>
#include <vector>

#include <objects/structs.h>


namespace VestPack {
    void copyDelta(
        std::string& baseBlob,
        std::string& objBlob,
        std::string& rData,
        size_t& offset
    );

    void addDelta(
        std::string& blob,
        std::string& rData,
        size_t& offset
    );

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
    );
}

#endif // VEST_PACK_DELTA_H