#ifndef VEST_PACK_BLOB_H
#define VEST_PACK_BLOB_H

#include <cstdint>
#include <vector>
#include <unordered_map>

#include <objects/structs.h>


namespace VestPack {

    void processBlob(
        VestObjects::Tree* treeClass,
        VestObjects::TreeNode* parent,
        VestObjects::PackIndex& packIndex,
        std::string& fContent,
        std::string& dir,
        bool& writeOnFile,
        bool& checkDelta
    );

}

#endif // VEST_PACK_BLOB_H