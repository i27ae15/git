#ifndef VEST_PACK_COMMIT_H
#define VEST_PACK_COMMIT_H

#include <cstdint>

#include <objects/structs.h>


namespace VestPack {

    void processCommit(
        VestObjects::CommitLinkedList* commitList,
        VestObjects::PackIndex& packIndex,
        std::string& fContent,
        std::string& dir
    );

}

#endif // VEST_PACK_COMMIT_H