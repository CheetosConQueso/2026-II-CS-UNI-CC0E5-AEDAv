#ifndef __GENERAL_ITERATOR_H__
#define __GENERAL_ITERATOR_H__

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
class ForwardIterator : public Iterator<T> {
    public:
    using Parent = Iterator<T>;
    using Parent::Parent;
    
    virtual ForwardIterator<T>& operator++() = 0;
};

template <typename T>
class BackwardIterator : public Iterator<T> {
    public:
    using Parent = Iterator<T>;
    using Parent::Parent;

    virtual BackwardIterator<T>& operator++() = 0;
};

#endif // __GENERAL_ITERATOR_H__