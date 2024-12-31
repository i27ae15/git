#include <file/types.h>

namespace VestObjects {

    class CommitNode {
        public:
        VestTypes::CommitFile* commit;
        CommitNode* prev;
        CommitNode* next;

        CommitNode(VestTypes::CommitFile* commit);
        ~CommitNode();

        void addNext(CommitNode* node);
        void addNext(VestTypes::CommitFile* commit);

        void addPrev(CommitNode* node);
        void addPrev(VestTypes::CommitFile* commit);
    };

    class CommitLinkedList {

        public:

        CommitLinkedList();
        ~CommitLinkedList();

        void addNode(CommitNode* node);
        void addNode(VestTypes::CommitFile* commit);

        void printCommits();

        private:

        void setHead(CommitNode* node);
        void setTail(CommitNode* node);

        CommitNode* head;
        CommitNode* current;
        CommitNode* tail;

    };

    class TreeNode {
        public:

        TreeNode(VestTypes::TreeFile* treeFile);
        TreeNode(VestTypes::TreeFile* treeFile, TreeNode* parent);

        void addChild(TreeNode* node);
        void addChild(VestTypes::TreeFile* treeFile);

        VestTypes::TreeFile* treeFile;
        TreeNode* parent;
        std::vector<TreeNode*> children;
    };

    class Tree {
        public:

        Tree();
        ~Tree();

        private:

        TreeNode* root;
    };
}