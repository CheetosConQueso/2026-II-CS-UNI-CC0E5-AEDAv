#ifndef __VECTOR_H__
#define __VECTOR_H__
#include <initializer_list>
#include <mutex>
#include "Iterator.h"
#include "GeneralNode.h"
#include "../foreach.h"
#include "../types.h" // Ref
#include "Container.h"

using namespace std;

template <typename Node, bool IsConst = false>
class VectorForwardIterator : public Iterator<Node,IsConst> {
    
public:
    using MySelf            = VectorForwardIterator<Node,IsConst>;
    using Parent            = Iterator<Node,IsConst>;
    using Parent::Parent;

    MySelf& operator++() { 
        ++Parent::m_ptr; 
        return *this; 
    }
};


template <typename Node, bool IsConst = false>
class VectorBackwardIterator : public Iterator<Node,IsConst> {

public:
    using MySelf            = VectorBackwardIterator<Node,IsConst>;
    using Parent            = Iterator<Node,IsConst>;
    using Parent::Parent;

    MySelf& operator++() { 
        --Parent::m_ptr; 
        return *this; 
    }
};

template <typename CompareTraits>
struct VectorTraits {
    using value_type            = typename CompareTraits::value_type;
    using Compare               = typename CompareTraits::Compare;
    using Node                  = GeneralNode<value_type>;
    using ForwardIterator       = VectorForwardIterator<Node>;  // itera sobre Node, no sobre T
    using ConstForwardIterator  = VectorForwardIterator<Node,true>;
    using BackwardIterator      = VectorBackwardIterator<Node>; // itera sobre Node, no sobre T
    using ConstBackwardIterator = VectorBackwardIterator<Node,true>;
};

template <typename T>
struct VectorAscTraits : public VectorTraits<AscendingTraits<T>> {};

template <typename T>
struct VectorDescTraits : public VectorTraits<DescendingTraits<T>> {};

template <typename Traits>
class Vector : public Container<Traits> {
    public:
    using Parent                = Container<Traits>;
    using value_type            = Parent::value_type;
    using Compare               = Parent::Compare;    
    using Node                  = Parent::Node;
    using ForwardIterator       = Parent::ForwardIterator;
    using ConstForwardIterator  = Parent::ConstForwardIterator;
    using BackwardIterator      = Traits::BackwardIterator;
    using ConstBackwardIterator = Traits::ConstBackwardIterator;
    using NodePtr               = Node*;
    using Parent::Parent;

    Vector() = default;

    Vector(initializer_list<pair<value_type, Ref>> values) {
        for (const auto& value : values)
            this->push_back(value.first, value.second);
    }

    Vector(const Vector& other) {
        resize(other.Parent::m_size);
        for (size_t i = 0; i < other.Parent::m_size; ++i)
            m_data[i] = other.m_data[i];
        Parent::m_size = other.Parent::m_size;
    }

    // Move constructor and move assignment operator
    Vector(Vector&& other) noexcept {
        m_data     = std::exchange(other.m_data, nullptr);
        Parent::m_size     = std::exchange(other.Parent::m_size, 0);
        m_capacity = std::exchange(other.m_capacity, 0);
    }

    ~Vector() override { delete[] m_data; }

    protected:
    NodePtr m_data     = nullptr;
    size_t  m_capacity  = 0;
    Compare m_comp;

    void resize(size_t new_cap) {
        if (new_cap <= m_capacity) return;
        Node* new_data = new Node[new_cap];
        for (size_t i = 0; i < Parent::size(); ++i)
            new_data[i] = m_data[i];
        delete[] m_data;
        m_data = new_data;
        m_capacity = new_cap;
    }

    void clearUnlocked() override {
        delete [] m_data;
        m_data     = nullptr;
        Parent::m_size = 0;
        m_capacity = 0;
    }

    void push_backUnlocked(const value_type& value, Ref ref) override {
        NodePtr nodePtr = new Node(value, ref);
        push_backUnlocked(nodePtr);
    }

    void push_backUnlocked(NodePtr nodePtr) override {
        if (Parent::m_size == m_capacity) {
            size_t new_cap = (m_capacity == 0) ? 10 : m_capacity * 2;
            resize(new_cap);
        }
        m_data[Parent::m_size] = *nodePtr;
        ++Parent::m_size;
    }

    public:

    size_t capacity() const { return m_capacity; }

    ForwardIterator       begin() override        { return ForwardIterator(m_data); }
    ForwardIterator       end()   override        { return ForwardIterator(m_data + Parent::m_size); }
    ConstForwardIterator  cbegin() const override { return ConstForwardIterator(m_data); }
    ConstForwardIterator  cend() const override   { return ConstForwardIterator(m_data + Parent::m_size); }
    BackwardIterator      rbegin()                { return BackwardIterator(m_data + Parent::m_size - 1); }
    BackwardIterator      rend()                  { return BackwardIterator(m_data - 1); }
    ConstBackwardIterator crbegin() const         { return ConstBackwardIterator(m_data + Parent::m_size - 1); }
    ConstBackwardIterator crend() const           { return ConstBackwardIterator(m_data - 1); }

    Node pop_back() {
        lock_guard<mutex> lock(Parent::m_mutex);
        if (Parent::empty()) throw std::out_of_range("Vector vacio");
        Node ret = m_data[Parent::m_size-1];
        --Parent::m_size;
        return ret;
    }


    Node& at(size_t index) {
        if (index >= Parent::m_size) throw std::out_of_range("Indice fuera de rango");
        return m_data[index];
    }

    Node& operator[](size_t index) {
        if (index >= Parent::m_size) throw std::out_of_range("Indice fuera de rango");
        return m_data[index];
    }

    Node& operator[] (size_t index) const {
        if (index >= Parent::m_size) throw std::out_of_range("Indice fuera de rango");
        return m_data[index];
    }

    template<typename Func, typename... Args>
    decltype(auto) rcall(Func func, Args&&... args)
    {    lock_guard<mutex> lock(Parent::m_mutex);
        if constexpr(is_void_v<invoke_result_t<Func, Node&, Args...>>)
            ::call(rbegin(), rend(), std::forward<Func>(func), std::forward<Args>(args)...);
        else // return type is not void:
            return ::call(rbegin(), rend(), std::forward<Func>(func), std::forward<Args>(args)...);
    }
};
#endif // __VECTOR_H__