#ifndef __LISTIOSTREAM__
#define __LISTIOSTREAM__

#include <iostream>
#include <mutex>
#include "containers/LinkedList.h"

template <typename Traits>
std::ostream& operator <<(std::ostream& os, const LinkedList<Traits>& list) {
    scoped_lock lock(list.getMutex());
    
    if(list.empty()) return os << "[]";

    auto currentPtr = list.getRoot();

    os << "[";
    while (currentPtr != &(list.getSentinel())) {
        os << *currentPtr;
        currentPtr = currentPtr->getNext();
        if (currentPtr != &(list.getSentinel())) os << ",";
    }
    os << "]";

    return os;
}

template <typename Traits>
std::istream &operator >>(std::istream &is, LinkedList<Traits>& list) {
    typename LinkedList::Delim d;
    typename LinkedList::Node node;

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