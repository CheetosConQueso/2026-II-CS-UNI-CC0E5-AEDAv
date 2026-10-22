#ifndef __LISTIOSTREAM__
#define __LISTIOSTREAM__

#include <iostream>
#include <mutex>
#include "Container.h"

template <typename Traits>
std::ostream& operator<<(std::ostream& os, const Container<Traits>& container) {
    std::scoped_lock lock(container.m_mutex);

    os << '[';
    auto currentPtr = list.m_pRoot;
    while (currentPtr && currentPtr != &list.sentinel) {
        os << *currentPtr;
        currentPtr = currentPtr->m_pNext;
        if (currentPtr && currentPtr != &list.sentinel) os << ',';
    }
    return os << ']';
}

template <typename Traits>
std::istream& operator>>(std::istream& is, LinkedList<Traits>& list) {
    using List = LinkedList<Traits>;
    using Delim = typename List::Delim;
    using Node = typename List::Node;

    Delim d;
    Node node;
    
    list.clear();
    is >> d;
    if (is >> d && d != ']'){
        is.unget();
        while(is >> node >> d) {
            list.push_back(node.getValue(), node.getRef());
            if (d == ']') break;
        }
    }
    return is;
}
#endif // __LISTIOSTREAM__