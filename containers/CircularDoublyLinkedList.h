#ifndef __CIRCULARDOUBLYLINKEDLIST_H__
#define __CIRCULARDOUBLYLINKEDLIST_H__

#include "DoublyLinkedList.h"

template <typename Traits>
struct CircularDoublyLinkedListTraits : DoublyLinkedListTraits<Traits> {};

template <typename T>
struct CircularDoublyLinkedListAscTraits
    : CircularDoublyLinkedListTraits<AscendingTraits<T>> {};

template <typename T>
struct CircularDoublyLinkedListDescTraits
    : CircularDoublyLinkedListTraits<DescendingTraits<T>> {};

template <typename Traits>
class CircularDoublyLinkedList : public DoublyLinkedList<Traits> {
public:
    using Parent = DoublyLinkedList<Traits>;
    using Node = typename Parent::Node;
    using NodePtr = typename Parent::NodePtr;
    using BackwardIterator = typename Parent::BackwardIterator;

protected:
    void restoreCircularLinks() {
        if (this->m_pRoot == nullptr) {
            this->sentinel.m_pNext = nullptr;
            this->sentinel.m_pPrev = nullptr;
            return;
        }

        this->sentinel.m_pNext = this->m_pRoot;
        this->sentinel.m_pPrev = this->m_pTail;
        this->m_pRoot->m_pPrev = &this->sentinel;
        this->m_pTail->m_pNext = &this->sentinel;
    }

    void push_backUnlocked(NodePtr newNode) override {
        Parent::push_backUnlocked(newNode);
        restoreCircularLinks();
    }

    void internalInsert(NodePtr newNode, NodePtr& currentPtr) override {
        Parent::internalInsert(newNode, currentPtr);
        restoreCircularLinks();
    }

    void clearUnlocked() override {
        Parent::clearUnlocked();
        restoreCircularLinks();
    }

public:
    BackwardIterator rbegin() {
        return BackwardIterator(this->sentinel.m_pPrev
            ? this->sentinel.m_pPrev
            : &this->sentinel);
    }

    BackwardIterator rend() {
        return BackwardIterator(&this->sentinel);
    }

    template <typename Func, typename... Args>
    decltype(auto) rcall(Func func, Args&&... args) {
        if constexpr (is_void_v<invoke_result_t<Func, Node&, Args...>>) {
            ::call(rbegin(), rend(), std::forward<Func>(func), std::forward<Args>(args)...);
        } else {
            return ::call(rbegin(), rend(), std::forward<Func>(func), std::forward<Args>(args)...);
        }
    }
};

#endif // __CIRCULARDOUBLYLINKEDLIST_H__
