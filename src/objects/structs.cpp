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
     *                             Tree
     * ===================================================================
    */

    TreeNode::TreeNode(VestTypes::TreeFile* treeFile) : TreeNode(treeFile, nullptr) {}

    TreeNode::TreeNode(VestTypes::TreeFile* treeFile, TreeNode* parent)
    : treeFile {treeFile}, parent {parent} {}

    void TreeNode::addChild(TreeNode* node) {
        children.push_back(node);
    }

    void TreeNode::addChild(VestTypes::TreeFile* treeFile) {
        addChild(new TreeNode(treeFile));
    }

    Tree::Tree() : root {nullptr} {};
    Tree::~Tree() {};


}