#ifndef __CIRCULARLINKEDLIST_H__
#define __CIRCULARLINKEDLIST_H__
#include "LinkedList.h"

template <typename Traits>
class CircularLinkedList : public LinkedList<Traits> {
    public:
        using Parent    = LinkedList<Traits>;

    protected:
        void push_backUnlocked(Parent::NodePtr newNode) override {
            Parent::push_backUnlocked(newNode);
            if(this->size() == 1) 
                this->sentinel.m_pNext = this->m_pRoot;
        }

        void internalInsert(Parent::NodePtr newNode) override {
            Parent::internalInsert(newNode);
            this->sentinel.m_pNext = this->m_pRoot;
        }

        void clearUnlocked() override {
            Parent::clearUnlocked();
            this->sentinel.m_pNext = nullptr;
        };
};
#endif // __CIRCULARLINKEDLIST_H__
