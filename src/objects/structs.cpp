#include <file/types.h>
#include <objects/structs.h>

#include <utils.h>

namespace VestObjects {

    Node::Node(VestTypes::CommitFile* commit) :
        commit {commit}, prev {nullptr}, next {nullptr} {}

    Node::~Node() {
        if (prev != nullptr) prev->next = next;
        if (next != nullptr) next->prev = prev;

        prev = nullptr;
        next = nullptr;
        commit = nullptr;
    }

    void Node::addNext(Node* node) {
        next = node;
        node->addPrev(this);
    }

    void Node::addNext(VestTypes::CommitFile* commit) {
        addNext(new Node(commit));
    }

    void Node::addPrev(Node* node) {
        prev = node;
        node->addNext(this);
    }

    void Node::addPrev(VestTypes::CommitFile* commit) {
        addPrev(new Node(commit));
    }

    /**
     * ===================================================================
     *                             LinkedList
     * ===================================================================
    */

    CommitLinkedList::CommitLinkedList() :
        head {nullptr}, current {nullptr}, tail {nullptr} {}

    CommitLinkedList::~CommitLinkedList() {}

    void CommitLinkedList::addNode(Node* node) {

        if (head == nullptr) setHead(node);

        setTail(node);
        current = node;
    }

    void CommitLinkedList::addNode(VestTypes::CommitFile* commit) {
        addNode(new Node(commit));
    }

    void CommitLinkedList::setHead(Node* node) {

        if (head == nullptr) {head = node; return;}

        node->next = head;
        head->prev = node;

        head = node;
    }

    void CommitLinkedList::setTail(Node* node) {
        if (tail == nullptr) {tail = node; return;}

        tail->next = node;
        node->prev = tail;

        tail = node;
    }

    void CommitLinkedList::printCommits() {

        Node* currentNode = head;

        while (currentNode != nullptr) {
            currentNode->commit->printCommitFile();
            PRINT_SML_SEPARATION;
            currentNode = currentNode->next;
        }

    }



}