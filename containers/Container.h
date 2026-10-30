#ifndef __CONTAINER__
#define __CONTAINER__

#include <Utility>
#include <mutex>
#include "Iterator.h"
#include "../foreach.h"
#include "../types.h"

template <typename T, typename CompareT> 
struct CompareTraits {
    using value_type = T;
    using Compare    = CompareT;
};

template <typename T> 
struct AscendingTraits : public CompareTraits<T, std::less<T>> {};

template <typename T> 
struct DescendingTraits : public CompareTraits<T, std::greater<T>> {};

template <typename Traits>
class Container;

template <typename Traits>
std::ostream& operator<<(std::ostream& os, const Container<Traits>& container);

template <typename Traits>
std::istream& operator>>(std::istream& is, Container<Traits>& container);

template <typename Traits>
class Container : public ForwardIterable<Traits> {
public:
    using value_type            = typename Traits::value_type;
    using Node                  = typename Traits::Node;
    using Compare               = typename Traits::Compare;
    using Delim                 = typename Node::Delim;
    using NodePtr               = Node*;

protected:
    mutable std::mutex m_mutex;
    size_t m_size = 0;
    Compare m_comp;

    virtual void clearUnlocked()                            = 0;
    virtual void push_backUnlocked(const value_type&, Ref)  = 0;
    virtual void push_backUnlocked(NodePtr)                 = 0;
    
public:
    Container() = default;

    Container(const Container& another) { *this = another; }
    
    virtual ~Container() = default ;

    virtual size_t size() const { return m_size; } 
    
    bool empty() const  { return size() == 0; }

    void push_back(const value_type& v, Ref r) {
        scoped_lock lock(m_mutex);
        push_backUnlocked(v, r);
    }

    void clear() {
        scoped_lock lock(m_mutex);
        clearUnlocked();
    };

    friend std::ostream& operator<< <Traits>(std::ostream& os,const Container& container);
    friend std::istream& operator>> <Traits>(std::istream& is, Container& container);

    std::ostream& write(std::ostream& os) { return os << *this; }
    std::istream& read(std::istream& is) { return is >> *this; }
};
#endif // __CONTAINER__