#ifndef VEST_OBJECTS_STRUCTS_H
#define VEST_OBJECTS_STRUCTS_H

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

        TreeNode(VestTypes::TreeFile* treeFile, std::string folderName);
        TreeNode(VestTypes::TreeFile* treeFile, std::string folderName, TreeNode* parent);

        void addChild(TreeNode* node);
        void addChild(VestTypes::TreeFile* treeFile, std::string folderName);

        void incrementIndex();

        bool isCompleted();

        std::string getFolderName();
        std::string getPath();

        VestTypes::TreeFileLine* getCurrentLine();

        VestTypes::TreeFile* treeFile;
        TreeNode* parent;
        std::vector<TreeNode*> children;

        private:

        std::string folderName;

        bool completed;
        uint8_t index;
    };

    class Tree {
        public:

        Tree();
        ~Tree();

        TreeNode* getRoot();
        TreeNode* getIndex();

        void setRoot(TreeNode* node);
        void setIndex(TreeNode* node);
        void setRoot(VestTypes::TreeFile* treeFile, std::string folderName);

        private:

        TreeNode* index;
        TreeNode* root;
    };

}

#endif