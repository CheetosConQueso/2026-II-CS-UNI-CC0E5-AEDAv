#ifndef __GENERAL_ITERATOR_H__
#define __GENERAL_ITERATOR_H__

#include <iostream>
#include <stdexcept>
#include <algorithm>
#include "../foreach.h"

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

template <typename Traits>
class ForwardIterable {
public:
    using ForwardIterator       = typename Traits::ForwardIterator;
    using ConstForwardIterator  = typename Traits::ConstForwardIterator;
    using Node                  = typename Traits::Node;

protected:
    virtual ForwardIterator         begin()         = 0;
    virtual ForwardIterator         end()           = 0;
    virtual ConstForwardIterator    cbegin () const = 0;
    virtual ConstForwardIterator    cend () const   = 0;
    
public:
    template <typename Func, typename... Args> 
    void ApplyFunction(Func func, Args... args) { call(func, std::forward<Args>(args)...); }
    
    template <typename Func, typename... Args> 
    void cApplyFunction(Func func, Args... args) const { ccall(func, std::forward<Args>(args)...); }

    template <typename Func, typename... Args> 
    Node& FirstThat(Func func, Args... args) { return call(func, std::forward<Args>(args)...); }    

    template <typename Func, typename... Args> 
    const Node& cFirstThat(Func func, Args... args) const { return ccall(func, std::forward<Args>(args)...); }  

    template <typename Func, typename... Args> 
    decltype(auto) call(Func func, Args&&... args) { return ::call(begin(), end(), std::forward<Func>(func), std::forward<Args>(args)...); }

    template <typename Func, typename... Args> 
    decltype(auto) ccall(Func func, Args&&... args) const { return ::ccall(cbegin(), cend(), std::forward<Func>(func), std::forward<Args>(args)...); }
};

template <typename Traits>
class BackwardIterable {
public:
    using BackwardIterator      = typename Traits::BackwardIterator;
    using ConstBackwardIterator = typename Traits::ConstBackwardIterator;
    using Node                  = typename Traits::Node;

    virtual BackwardIterator rbegin()             = 0;
    virtual BackwardIterator rend()               = 0;
    virtual ConstBackwardIterator crbegin() const = 0;
    virtual ConstBackwardIterator crend() const   = 0;

    template <typename Func, typename... Args>
    decltype(auto) rcall(Func func, Args&&... args) {
        if constexpr (is_void_v<invoke_result_t<Func, Node&, Args...>>) {
            ::call(rbegin(), rend(), std::forward<Func>(func), std::forward<Args>(args)...);
        } else {
            return ::call(rbegin(), rend(), std::forward<Func>(func), std::forward<Args>(args)...);
        }
    }
};
#endif // __GENERAL_ITERATOR_H__