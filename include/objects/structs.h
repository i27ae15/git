#include <file/types.h>

namespace VestObjects {

    class Node {
        public:
        VestTypes::CommitFile* commit;
        Node* prev;
        Node* next;

        Node(VestTypes::CommitFile* commit);
        ~Node();

        void addNext(Node* node);
        void addNext(VestTypes::CommitFile* commit);

        void addPrev(Node* node);
        void addPrev(VestTypes::CommitFile* commit);
    };

    class CommitLinkedList {

        public:

        CommitLinkedList();
        ~CommitLinkedList();

        void addNode(Node* node);
        void addNode(VestTypes::CommitFile* commit);

        void printCommits();

        private:

        void setHead(Node* node);
        void setTail(Node* node);

        Node* head;
        Node* current;
        Node* tail;

    };
}