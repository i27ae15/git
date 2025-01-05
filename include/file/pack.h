#ifndef VEST_PACK_H
#define VEST_PACK_H

#include <fstream>
#include <cstdint>
#include <vector>
#include <unordered_map>

#include <objects/structs.h>


namespace VestPack {

    struct PackHeader {
        char magic[4]; // Should be "PACK"
        uint32_t version;
        uint32_t numObjects;
    };

    struct ObjectHeader {
        uint8_t type;
        size_t start;
        size_t size;
    };

    ObjectHeader parseObjectHeader(const std::vector<uint8_t>& rData, size_t& offset);

    uint32_t parsePackHeader(const std::vector<uint8_t>& rData, size_t& offset);

    void setFileContent(
        const size_t& startAt,
        std::vector<uint8_t>& rData,
        size_t& offset,
        std::string& fContent
    );

    void copyDelta(
        std::string& baseBlob,
        std::string& objBlob,
        std::vector<uint8_t>& rData,
        size_t& offset
    );

    void addDelta(
        std::string& blob,
        std::vector<uint8_t>& rData,
        size_t& offset
    );

    void processRefDelta(
        size_t& offset,
        std::string& baseBlob,
        std::vector<uint8_t>& rData
    );

    void processCommit(
        VestObjects::CommitLinkedList* commitList,
        std::string& fContent,
        std::string& dir
    );

    void processTree(
        VestObjects::Tree* treeClass,
        VestObjects::TreeNode* parent,
        std::string& fContent,
        std::string& dir,
        bool& writeOnFile
    );

    void processBlob(
        VestObjects::TreeNode* parent,
        std::string& fContent,
        std::string& dir,
        std::string& lastBlob,
        bool& writeOnFile
    );

    void processPack(
        std::vector<uint8_t>& rData,
        size_t offset,
        std::string& dir
    );

}

#endif // VEST_PACK_H