#ifndef __ITERATOR_H__
#define __ITERATOR_H__

#include <iostream>
#include <stdexcept>
#include <algorithm> // para std::swap

template <typename T>
class Iterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using difference_type   = std::ptrdiff_t;
        using value_type        = T;
        using pointer           = value_type*;
        using reference         = value_type&;
        
    protected:
        pointer m_ptr;
        
    public:
        Iterator(pointer ptr) : m_ptr(ptr) {}
        
        value_type& operator*() const { return *m_ptr; }
        friend bool operator== (const Iterator& a, const Iterator& b) { return a.m_ptr == b.m_ptr; };
        friend bool operator!= (const Iterator& a, const Iterator& b) { return a.m_ptr != b.m_ptr; };
};

template <typename T>
class ConstIterator : public Iterator<const T> {
    public:
    using MySelf = ConstIterator<T>;
    using Parent = Iterator<const T>;
    using Parent::Parent;
    
    ConstIterator(const Parent::pointer ptr) : Parent(ptr) {}
};

template <typename T>
class ForwardIterator : public Iterator<T> {
    public:
    using MySelf = ForwardIterator<T>;
    using Parent = Iterator<T>;
    using Parent::Parent;

    virtual MySelf& operator++() = 0;
};

template <typename T>
class ConstForwardIterator : public ConstIterator<T> {
    public:
    using MySelf = ConstForwardIterator<T>;
    using Parent = ConstIterator<T>;
    using Parent::Parent;

    virtual MySelf& operator++() = 0;
};

template <typename T>
class BackwardIterator : public Iterator<T> {
    public:
    using MySelf = BackwardIterator<T>;
    using Parent = Iterator<T>;
    using Parent::Parent;

    virtual MySelf& operator++() = 0;
};

template <typename T>
class ConstBackwardIterator : public ConstIterator<T> {
    public:
    using MySelf = ConstBackwardIterator<T>;
    using Parent = ConstIterator<T>;
    using Parent::Parent;

    virtual MySelf& operator++() = 0;
};

#endif // __ITERATOR_H__