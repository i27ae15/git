#include <file/types.h>
#include <objects/structs.h>

#include <utils.h>

namespace VestObjects {

    CommitNode::CommitNode(VestTypes::CommitFile* commit) :
        commit {commit}, prev {nullptr}, next {nullptr} {}

    CommitNode::~CommitNode() {
        if (prev != nullptr) prev->next = next;
        if (next != nullptr) next->prev = prev;

        prev = nullptr;
        next = nullptr;
        commit = nullptr;
    }

    void CommitNode::addNext(CommitNode* node) {
        next = node;
        node->addPrev(this);
    }

    void CommitNode::addNext(VestTypes::CommitFile* commit) {
        addNext(new CommitNode(commit));
    }

    void CommitNode::addPrev(CommitNode* node) {
        prev = node;
        node->addNext(this);
    }

    void CommitNode::addPrev(VestTypes::CommitFile* commit) {
        addPrev(new CommitNode(commit));
    }

    /**
     * ===================================================================
     *                             LinkedList
     * ===================================================================
    */

    CommitLinkedList::CommitLinkedList() :
        head {nullptr}, current {nullptr}, tail {nullptr} {}

    CommitLinkedList::~CommitLinkedList() {}

    void CommitLinkedList::addNode(CommitNode* node) {

        if (head == nullptr) setHead(node);

        setTail(node);
        current = node;
    }

    void CommitLinkedList::addNode(VestTypes::CommitFile* commit) {
        addNode(new CommitNode(commit));
    }

    void CommitLinkedList::setHead(CommitNode* node) {

        if (head == nullptr) {head = node; return;}

        node->next = head;
        head->prev = node;

        head = node;
    }

    void CommitLinkedList::setTail(CommitNode* node) {
        if (tail == nullptr) {tail = node; return;}

        tail->next = node;
        node->prev = tail;

        tail = node;
    }

    void CommitLinkedList::printCommits() {

        CommitNode* currentNode = head;

        while (currentNode != nullptr) {
            currentNode->commit->printCommitFile();
            PRINT_SML_SEPARATION;
            currentNode = currentNode->next;
        }

    }

    /**
     * ===================================================================
     *                           Tree Node
     * ===================================================================
    */

    TreeNode::TreeNode(VestTypes::TreeFile* treeFile, std::string folderName)
    : TreeNode(treeFile, folderName, nullptr) {}

    TreeNode::TreeNode(VestTypes::TreeFile* treeFile, std::string folderName, TreeNode* parent)
    : treeFile {treeFile}, parent {parent}, index {}, completed {}, folderName {folderName} {}

    void TreeNode::addChild(TreeNode* node) {
        children.push_back(node);
    }

    void TreeNode::addChild(VestTypes::TreeFile* treeFile, std::string folderName) {
        addChild(new TreeNode(treeFile, folderName));
    }

    void TreeNode::incrementIndex() {
        if (treeFile->tLines.size() > index) {
            index++;
        } else {
            completed = true;
        }
    };

    bool TreeNode::isCompleted() {return completed;}

    std::string TreeNode::getFolderName() {return folderName;}

    std::string TreeNode::getPath() {

        TreeNode* currentNode {this};
        std::string path {};

        while (currentNode != nullptr) {
            path = currentNode->getFolderName() + "/" + path;
            currentNode = currentNode->parent;
        }

        return path;
    }

    VestTypes::TreeFileLine* TreeNode::getCurrentLine() {return treeFile->tLines[index];}

    /**
     * ===================================================================
     *                           Tree
     * ===================================================================
    */

    Tree::Tree() : root {nullptr} {};
    Tree::~Tree() {};

    TreeNode* Tree::getIndex() {return index;}
    TreeNode* Tree::getRoot() {return root;}

    void Tree::setIndex(TreeNode* node) {index = node;}

    void Tree::setRoot(TreeNode* node) {
        if (root == nullptr) setIndex(node);
        root = node;
    }

    void Tree::setRoot(VestTypes::TreeFile* treeFile, std::string folderName) {
        if (folderName[folderName.size() - 1] == '/') {
            folderName = folderName.substr(0, folderName.size() - 1);
        }
        setRoot(new TreeNode(treeFile, folderName));
    }

}