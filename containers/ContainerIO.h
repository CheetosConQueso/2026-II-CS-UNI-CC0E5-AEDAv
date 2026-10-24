#ifndef __CONTAINERIO_H__
#define __CONTAINERIO_H__

#include "Container.h"
#include <istream>
#include <ostream>
#include <mutex>

template <typename Traits>
std::ostream& operator<<(std::ostream& os, const Container<Traits>& container) {
    using Node = typename Container<Traits>::Node;

    std::scoped_lock lock(container.m_mutex);

    bool first = true;

    os << '[';
    container.cApplyFunction([&](const Node& node) {
        if (!first) os << ',';
        os << node;
        first = false;
    });

    return os << ']';
}

template <typename Traits>
std::istream& operator>>(std::istream& is, Container<Traits>& container) {
    using Node  = typename Container<Traits>::Node;
    using Delim = typename Container<Traits>::Delim;

    Node node;
    Delim d;

    is >> std::ws >> d;
    if (d != '[') return is;
    std::scoped_lock lock(container.m_mutex);
    container.clearUnlocked();
    is >> std::ws;
    if (is.peek() == ']') return is >> d;

    while (is >> node) {
        container.push_backUnlocked(node.getValue(), node.getRef());
        is >> std::ws >> d;
        if (d == ']') break;
    }

    return is;
}
#endif // __CONTAINERIO_H__