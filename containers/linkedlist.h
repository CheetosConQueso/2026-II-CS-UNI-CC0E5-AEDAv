#ifndef __LINKEDLIST_H__
#define __LINKEDLIST_H__
#include <initializer_list>
#include <iostream>
#include <mutex>
#include "GeneralNode.h"
#include "Iterator.h"
#include "../foreach.h"
#include "Container.h"

template <typename T>
class LinkedListNode : public GeneralNode<T> {
    public:
    using Node      = LinkedListNode<T>;
    using NodePtr   = Node*;

    NodePtr m_pNext = nullptr;
    
    LinkedListNode() : GeneralNode<T>(T{}, Ref{}), m_pNext(nullptr) {}
    LinkedListNode(const T& value, Ref ref, NodePtr pNext) : GeneralNode<T>(value, ref), m_pNext(pNext) {}
};

template <typename T>
class LinkedListForwardIterator : public ForwardIterator<LinkedListNode<T>> {
    public:
    using MySelf    = LinkedListForwardIterator<T>;
    using Parent    = ForwardIterator<LinkedListNode<T>>;
    using Parent::Parent;

    MySelf& operator++() override { Parent::m_ptr = Parent::m_ptr->m_pNext; return *this; }
};

template <typename T>
class LinkedListConstForwardIterator : public ConstForwardIterator<LinkedListNode<T>> {
    public:
    using MySelf    = LinkedListConstForwardIterator<T>;
    using Parent    = ConstForwardIterator<LinkedListNode<T>>;
    using Parent::Parent;
    
    MySelf& operator++() override { Parent::m_ptr = Parent::m_ptr->m_pNext; return *this; }
};

template <typename CompareTraits>
struct LinkedListTraits {
    using value_type            = typename CompareTraits::value_type;
    using Compare               = typename CompareTraits::Compare;
    
    using Node                  = LinkedListNode<value_type>;
    using ForwardIterator       = LinkedListForwardIterator<value_type>;
    using ConstForwardIterator  = LinkedListConstForwardIterator<value_type>;
};

template <typename T> struct LinkedListAscTraits : public LinkedListTraits<AscendingTraits<T>> {};
template <typename T> struct LinkedListDescTraits : public LinkedListTraits<DescendingTraits<T>> {};


template <typename Traits>
class LinkedList : public Container<Traits> {
    public:
    using Parent                = Container<Traits>;
    using value_type            = typename Parent::value_type;
    using Node                  = typename Parent::Node;
    using Compare               = typename Parent::Compare;
    using ForwardIterator       = typename Parent::ForwardIterator;
    using ConstForwardIterator  = typename Parent::ConstForwardIterator;
    using Delim                 = typename Parent::Delim;
    using NodePtr               = Node *;
    using Parent::Parent;

    LinkedList(initializer_list<pair<value_type, Ref>> values) {
        for (const auto& value : values)
            this->push_back(value.first, value.second);
    }

    ~LinkedList() override { this->clear(); }

    void insert(const value_type& v, Ref r) { 
        scoped_lock lock(Parent::m_mutex);
        internalInsert(v, r);
    }

    ForwardIterator      begin()  override       { return ForwardIterator(m_pRoot ? m_pRoot : &sentinel); }
    ForwardIterator      end()    override       { return ForwardIterator(&sentinel); }
    ConstForwardIterator cbegin() const override { return ConstForwardIterator(m_pRoot ? m_pRoot : &sentinel); }
    ConstForwardIterator cend()   const override { return ConstForwardIterator(&sentinel); }

    protected:
    NodePtr m_pRoot = nullptr; 
    NodePtr m_pTail = nullptr;
    Node sentinel;

    void clearUnlocked() override {
        Parent::ApplyFunction([](Node& node) { delete &node; });
        m_pRoot = nullptr;
        m_pTail = nullptr;
        Parent::m_size  = 0;
    };
    
    void push_backUnlocked(const value_type& value, Ref ref) override {
        NodePtr nodePtr = new Node(value, ref, &sentinel);
        push_backUnlocked(nodePtr);
    }

    void push_backUnlocked(NodePtr nodePtr) override {
        nodePtr->m_pNext = &sentinel;
        if (Parent::empty()) {
            m_pRoot = m_pTail = nodePtr;
        } else {
            m_pTail->m_pNext = nodePtr;
            m_pTail = nodePtr;
        }
        ++Parent::m_size;
    }

    virtual void internalInsert(const value_type& value, Ref ref) {
        NodePtr nodePtr = new Node(value, ref, nullptr);
        internalInsert(nodePtr);
    }

    virtual void internalInsert(NodePtr nodePtr) {
        if (Parent::empty()) return push_backUnlocked(nodePtr);

        NodePtr* currentPtr = &m_pRoot;
        while (*currentPtr != &sentinel) {
            if(Parent::m_comp(nodePtr->getValue(), (*currentPtr)->getValue())) {
                nodePtr->m_pNext = *currentPtr;
                *currentPtr = nodePtr;
                ++Parent::m_size;
                return;
            }
            currentPtr = &(*currentPtr)->m_pNext;
        }
        
        push_backUnlocked(nodePtr);
    }
};
#endif // __LINKEDLIST_H__
