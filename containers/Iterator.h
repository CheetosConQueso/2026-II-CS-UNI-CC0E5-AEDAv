#ifndef __GENERAL_ITERATOR_H__
#define __GENERAL_ITERATOR_H__

#include <iostream>
#include <stdexcept>
#include <algorithm>

template <typename T, bool IsConst = false>
class Iterator {
public:
    using iterator_category = std::forward_iterator_tag;
    using difference_type   = std::ptrdiff_t;
    using value_type        = T;
    using pointer           = std::conditional_t<IsConst, const T*, T*>;
    using reference         = std::conditional_t<IsConst, const T&, T&>;

    Iterator(pointer ptr) : m_ptr(ptr) {}
    virtual ~Iterator() = default;

    Iterator& set(pointer ptr) { 
        m_ptr = ptr;  
        return *this; 
    }

    reference operator*() const { return *m_ptr; }
    pointer operator->() const { return m_ptr; }

    virtual Iterator& operator++() = 0;
    friend bool operator==(const Iterator& a, const Iterator& b) { return a.m_ptr == b.m_ptr; }
    friend bool operator!=(const Iterator& a, const Iterator& b) { return !(a == b); }

protected:
    pointer m_ptr = nullptr;
};

#endif // __GENERAL_ITERATOR_H__