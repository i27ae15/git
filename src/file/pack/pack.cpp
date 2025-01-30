#include <vector>
#include <stdexcept>
#include <cstdint>

#include <utils.h>

#include <objects/initializers.h>
#include <objects/helpers.h>
#include <objects/structs.h>
#include <objects/readers.h>

#include <file/file.h>
#include <file/types.h>
#include <file/utils.h>
#include <file/pack/pack.h>

#include <file/pack/helpers.h>
#include <file/pack/tree.h>
#include <file/pack/blob.h>
#include <file/pack/commit.h>
#include <file/pack/delta.h>
#include <file/pack/pack.h>


namespace VestPack {

    void processPack(
        std::vector<uint8_t>& rData,
        size_t offset,
        std::string& dir
    ) {

        size_t _offset = offset;
        uint32_t nObjects = parsePackHeader(rData, _offset);

        VestObjects::CommitLinkedList* commitList = new VestObjects::CommitLinkedList();
        VestObjects::Tree* tree = new VestObjects::Tree();
        VestObjects::PackIndex packIndex {};
        std::string lastBlob {};

        bool isHead {true};
        bool mustBeDelta {};

        for (uint32_t i {}; i < nObjects; i++) {

            ObjectHeader objHeader = parseObjectHeader(rData, _offset);
            VestObjects::TreeNode* treeIdx = tree->getIndex();

            if (objHeader.type == VestTypes::REF_DELTA) {
                (void)processRefDelta(commitList, tree, treeIdx, packIndex, _offset, dir, rData, isHead, mustBeDelta);
                continue;
            }

            if (mustBeDelta) {
                PRINT_ERROR("NEXT FILE MUST BE DELTA AND IS NOT!");
                throw std::runtime_error("");
            }

            std::string fContent {};
            (void)setFileContent(_offset, rData, _offset, fContent);

            switch (objHeader.type) {
                case VestTypes::COMMIT:
                    (void)processCommit(commitList, packIndex, fContent, dir);
                    break;

                case VestTypes::TREE:
                    (void)processTree(commitList, tree, treeIdx, packIndex, fContent, dir, isHead);
                    break;

                case VestTypes::BLOB:
                    (void)processBlob(tree, treeIdx, packIndex, fContent, dir, isHead, mustBeDelta);
                    break;

                case VestTypes::OFS_DELTA:
                    PRINT_WARNING("OFS_DELTA - DOING NOTHING");
                    break;

                default:
                    PRINT_ERROR("NOT VALID TYPE FOUND");
                    break;
            }
        }

    }
}