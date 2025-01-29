#include <string>

#include <utils.h>

#include <objects/initializers.h>
#include <objects/structs.h>

#include <file/file.h>

#include <file/pack/commit.h>


namespace VestPack {

    void processCommit(
        VestObjects::CommitLinkedList* commitList,
        VestObjects::PackIndex& packIndex,
        std::string& fContent,
        std::string& dir
    ) {
        VestTypes::CommitFile* commitFile = VestFile::readCommit(fContent);
        commitList->addNode(commitFile);

        std::string sha1 = VestObjects::createCommit(fContent, dir);
        packIndex.addSha1(sha1);

        PRINT_COMMIT("COMMIT SHA1 WRITTEN: " + sha1);
    }

}
