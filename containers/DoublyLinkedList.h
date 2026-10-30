#ifndef __DOUBLYLINKEDLIST_H__
#define __DOUBLYLINKEDLIST_H__

#include "GeneralNode.h"
#include "Iterator.h"
#include "LinkedList.h"

template <typename T>
class DoublyLinkedListNode : public GeneralNode<T> {
    using Parent = GeneralNode<T>;

public:
    using NodePtr = DoublyLinkedListNode<T>*;

    NodePtr m_pPrev = nullptr;
    NodePtr m_pNext = nullptr;

    DoublyLinkedListNode() = default;
    DoublyLinkedListNode(const T& value, Ref ref, NodePtr pNext = nullptr) : Parent(value, ref), m_pNext(pNext) {}
};

template <typename T, bool IsConst = false>
class DoublyLinkedListBackwardIterator : public Iterator<DoublyLinkedListNode<T>,IsConst> {
public:
    using MySelf    = DoublyLinkedListBackwardIterator<T,IsConst>;
    using Node      = DoublyLinkedListNode<T>;
    using Parent    = Iterator<Node,IsConst>;
    using Parent::Parent;
    using Parent::set;

    MySelf& operator++() {
        set((*this)->m_pPrev);
        return *this;
    }
};

template <typename Traits>
struct DoublyLinkedListTraits {
    using value_type            = typename Traits::value_type;
    using Compare               = typename Traits::Compare;
    using Node                  = DoublyLinkedListNode<value_type>;
    using ForwardIterator       = LinkedListForwardIterator<Node>;
    using ConstForwardIterator  = LinkedListForwardIterator<Node,true>;
    using BackwardIterator      = DoublyLinkedListBackwardIterator<value_type>;
    using ConstBackwardIterator = DoublyLinkedListBackwardIterator<value_type,true>;
};

template <typename T>
struct DoublyLinkedListAscTraits : DoublyLinkedListTraits<AscendingTraits<T>> {};

template <typename T>
struct DoublyLinkedListDescTraits : DoublyLinkedListTraits<DescendingTraits<T>> {};

template <typename Traits>
class DoublyLinkedList : 
    public LinkedList<Traits>, 
    public BackwardIterable<Traits> {
public:
    using Parent                = LinkedList<Traits>;
    using Node                  = typename Traits::Node;
    using NodePtr               = typename Traits::Node*;
    
    using BackwardIterator      = typename BackwardIterable<Traits>::BackwardIterator;
    using ConstBackwardIterator = typename BackwardIterable<Traits>::ConstBackwardIterator;

protected:
    void push_backUnlocked(NodePtr newNode) override {
        Parent::push_backUnlocked(newNode);
        newNode->m_pPrev = this->sentinel.m_pPrev;
        this->sentinel.m_pPrev = newNode;
    }

    void internalInsert(NodePtr newNode) override {
        Parent::internalInsert(newNode);
        if (this->m_pTail != newNode) {
            newNode->m_pPrev = newNode->m_pNext->m_pPrev;
            newNode->m_pNext->m_pPrev = newNode;
        }
    }

    void clearUnlocked() override {
        Parent::clearUnlocked();
        this->sentinel.m_pPrev = nullptr;
    }

public:
    BackwardIterator rbegin() override              { return BackwardIterator(this->m_pTail); }
    BackwardIterator rend() override                { return BackwardIterator(nullptr); }
    ConstBackwardIterator crbegin() const override  { return ConstBackwardIterator(this->m_pTail); }
    ConstBackwardIterator crend() const override    { return ConstBackwardIterator(nullptr); }
};

#endif // __DOUBLYLINKEDLIST_H__
