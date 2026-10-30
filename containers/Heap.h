#ifndef __HEAP_H__
#define __HEAP_H__

#include <mutex>
#include "vector.h"
template <typename CompareTraits>
struct HeapTraits {
    using value_type            = typename CompareTraits::value_type;
    using Compare               = typename CompareTraits::Compare;
    using Node                  = GeneralNode<value_type>;
    using ForwardIterator       = VectorForwardIterator<Node>;
    using ConstForwardIterator  = VectorForwardIterator<Node,true>;
};

template <typename T>
struct HeapAscTraits : 
    public HeapTraits<AscendingTraits<T>> {};

template <typename T>
struct HeapDescTraits : 
    public HeapTraits<DescendingTraits<T>> {};


template <typename Traits>
class Heap : 
    public Container<Traits> {

public:
    using Parent                = Container<Traits>;
    
    using value_type            = Parent::value_type;
    using Compare               = Parent::Compare;    
    using Node                  = Parent::Node;
    
    using ForwardIterator       = ForwardIterable<Traits>::ForwardIterator;
    using ConstForwardIterator  = ForwardIterable<Traits>::ConstForwardIterator;
    
    using NodePtr               = Node*;
    using Parent::Parent;
    
    using Parent::size;
    using Parent::empty;
    using Parent::call;

protected:
    using Parent::m_mutex;
    using Parent::m_comp;
    using Parent::m_size;

    Vector<VectorAscTraits<value_type>> m_data;

public:
    Node pop() {
        scoped_lock lock(m_mutex);

        if (empty()) throw std::out_of_range("Heap vacio");
        
        std::swap(m_data[0], m_data[size() - 1]);
        Node ret = m_data.pop_back();
        --m_size;
        heapifyDown(0);
        
        return ret;
    }

    template <typename Func, typename... Args>
    void ApplyFunction(Func func, Args... args) {
        scoped_lock lock (m_mutex);
        call(func, std::forward<Args>(args)...);

        for (size_t i = m_data.size() / 2; i > 0; --i) heapifyDown(i - 1);
    }
    
    ForwardIterator      begin() override { return m_data.begin(); }
    ForwardIterator      end() override { return m_data.end(); }
    ConstForwardIterator cbegin() const override { return m_data.cbegin(); }
    ConstForwardIterator cend() const override { return m_data.cend(); }

    Heap& operator=(const Heap& another) { 
        scoped_lock lock(m_mutex);
        return copy(another); 
    }

protected:
    void clearUnlocked () override {
        m_data.clear();
        m_size = 0;
    }
        
    void push_backUnlocked(const value_type &value, Ref ref) {
        NodePtr nodePtr = new Node(value, ref);
        push_backUnlocked(nodePtr);
    }

    void push_backUnlocked(NodePtr nodePtr) {
        m_data.push_back(nodePtr->getValue(),nodePtr->getRef());
        ++m_size;
        heapifyUp(size());
    }

    void heapifyUp(size_t index) {
        if (index >= m_data.size()) return;
        auto insertedValue = m_data[index].getValue();
        
        while (index > 0) {
            size_t parentIndex = (index - 1) / 2;
            auto parentValue = m_data[parentIndex].getValue();

            if (!m_comp(insertedValue,parentValue)) break;
            
            std::swap(m_data[index], m_data[parentIndex]);
            index = parentIndex;
        }
    }

    void heapifyDown(size_t index) {
        if (index >= m_data.size()) return;
        auto insertedValue = m_data[index].getValue();

        while (hasLeftChild(index)) {
            Node left = getLeftChild(index);
            size_t bestIndex = index * 2 + 1;

            if (hasRightChild(index)) {
                Node right = getRightChild(index);
                if (m_comp(right.getValue(), left.getValue())) ++bestIndex;
            }

            if (m_comp(insertedValue, m_data[bestIndex].getValue())) break;

            std::swap(m_data[index], m_data[bestIndex]);
            index = bestIndex;
        }
    }

    bool hasLeftChild(size_t index) const { return index * 2 + 1 < size(); }
    bool hasRightChild(size_t index) const { return index * 2 + 2 < size(); }
    
    Node getLeftChild(size_t index) const { return m_data[index * 2 + 1]; }
    Node getRightChild(size_t index) const { return m_data[index * 2 + 2]; }

    Heap& copy(const Heap& another) {
        if (this == &another) return *this;
        
        m_size = another.m_size;
        m_comp = another.m_comp;
        m_data = another.m_data;
        
        return *this;
    }
};

template <typename T> class MinHeap : public Heap<HeapAscTraits<T>> {};
template <typename T> class MaxHeap : public Heap<HeapDescTraits<T>> {};

#endif // __HEAP_H__