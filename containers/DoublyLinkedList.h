#ifndef __DOUBLYLINKEDLIST_H__
#define __DOUBLYLINKEDLIST_H__

#include "GeneralNode.h"
#include "GeneralIterator.h"
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

template <typename T>
class DoublyLinkedListForwardIterator : public GeneralIterator<DoublyLinkedListForwardIterator<T>, DoublyLinkedListNode<T>> {
public:
    using MySelf = DoublyLinkedListForwardIterator<T>;
    using Parent = GeneralIterator<MySelf, DoublyLinkedListNode<T>>;
    using Parent::Parent;

    MySelf& operator++() {
        Parent::m_ptr = Parent::m_ptr->m_pNext;
        return *this;
    }
};

template <typename T>
class DoublyLinkedListBackwardIterator : public GeneralIterator<DoublyLinkedListBackwardIterator<T>, DoublyLinkedListNode<T>> {
public:
    using MySelf = DoublyLinkedListBackwardIterator<T>;
    using Parent = GeneralIterator<MySelf, DoublyLinkedListNode<T>>;
    using Parent::Parent;

    MySelf& operator++() {
        Parent::m_ptr = Parent::m_ptr->m_pPrev;
        return *this;
    }
};

template <typename Traits>
struct DoublyLinkedListTraits {
    using value_type        = typename Traits::value_type;
    using Compare           = typename Traits::Compare;
    using Node              = DoublyLinkedListNode<value_type>;
    using ForwardIterator   = DoublyLinkedListForwardIterator<value_type>;
    using BackwardIterator  = DoublyLinkedListBackwardIterator<value_type>;
};

template <typename T>
struct DoublyLinkedListAscTraits : DoublyLinkedListTraits<AscendingTraits<T>> {};

template <typename T>
struct DoublyLinkedListDescTraits : DoublyLinkedListTraits<DescendingTraits<T>> {};

template <typename Traits>
class DoublyLinkedList : public LinkedList<Traits> {
public:
    using Parent            = LinkedList<Traits>;
    using Node              = typename Traits::Node;
    using NodePtr           = typename Traits::Node*;
    using BackwardIterator  = typename Traits::BackwardIterator;

protected:
    void push_backUnlocked(NodePtr newNode) override {
        Parent::push_backUnlocked(newNode);
        newNode->m_pPrev = this->sentinel.m_pPrev;
        this->sentinel.m_pPrev = newNode;
    }

    void internalInsert(NodePtr newNode, NodePtr& currentPtr) override {
        Parent::internalInsert(newNode,currentPtr);
        newNode->m_pPrev = newNode->m_pNext->m_pPrev;
        newNode->m_pNext->m_pPrev = newNode;
    }

    void clearUnlocked() override {
        Parent::clearUnlocked();
        this->sentinel.m_pPrev = nullptr;
    }

public:
    BackwardIterator rbegin() { return BackwardIterator(this->m_pTail); }

    BackwardIterator rend() { return BackwardIterator(nullptr); }

    template <typename Func, typename... Args>
    decltype(auto) rcall(Func func, Args&&... args) {
        if constexpr (is_void_v<invoke_result_t<Func, Node&, Args...>>) {
            ::call(rbegin(), rend(), std::forward<Func>(func), std::forward<Args>(args)...);
        } else {
            return ::call(rbegin(), rend(), std::forward<Func>(func), std::forward<Args>(args)...);
        }
    }
};

#endif // __DOUBLYLINKEDLIST_H__
