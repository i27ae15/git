#ifndef VEST_READERS_H
#define VEST_READERS_H

#include <string>
#include <objects/structs.h>

namespace VestObjects {

    ObjectRead readObject(std::string& sha1);
    ObjectRead readObject(std::string& sha1, std::string& dir);

    VestTypes::TreeFile* readTreeFile(std::string& fContent);
    VestTypes::CommitFile* readCommit(std::string& fContent, bool fromPack = true);

}

#endif // VEST_READERS_H