#ifndef __GENERALNODE_H__
#define __GENERALNODE_H__
#include <iostream>
#include "../types.h" // Ref
template <typename T>
class GeneralNode{
private:
    T   m_value;
    Ref m_ref; //identificador para el valor

public:
    GeneralNode() = default; // requerido por resize(): new Node[new_cap]
    GeneralNode(const T& value, Ref ref) : m_value(value), m_ref(ref) {}
    T    getValue() const { return m_value; }
    Ref  getRef()   const { return m_ref;   }
    T&   value()          { return m_value; } // acceso mutable para ApplyFunction

    using Delim = std::istream::char_type;

    friend std::ostream &operator <<(std::ostream &os, const GeneralNode<T> &node) {
        os << "(" << node.getValue() << "," << node.getRef() << ")";
        return os;
    }

    friend std::istream &operator >>(std::istream &is, GeneralNode<T> &node){
        Delim d;
        T value;
        Ref ref;

        if (!(is >> std::ws >> d) || d != '(') {
            is.setstate(std::ios::failbit);
            return is;
        }

        if (!(is >> std::ws >> value >> std::ws >> d) || d != ',') {
            is.setstate(std::ios::failbit);
            return is;
        }

        if (!(is >> std::ws >> ref >> std::ws >> d) || d != ')') {
            is.setstate(std::ios::failbit);
            return is;
        }

        node.m_value = value;
        node.m_ref = ref;

        return is;
    }
};
#endif // __GENERALNODE_H__