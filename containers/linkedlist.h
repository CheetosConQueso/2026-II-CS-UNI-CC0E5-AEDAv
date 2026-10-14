#ifndef __LINKEDLIST_H__
#define __LINKEDLIST_H__
#include <mutex>
#include <utility>
#include "GeneralNode.h"
#include "GeneralIterator.h"
#include "../foreach.h"


template <typename T>
class LinkedListNode : public GeneralNode<T> {
    using Node      = LinkedListNode<T>;
    using NodePtr   = Node*;
public:
    NodePtr m_pNext = nullptr; // puntero al siguiente nodo, npara acceso directo necesita ser publico
    LinkedListNode() : GeneralNode<T>(T{}, Ref{}), m_pNext(nullptr) {}
    // añadir friend class añade otro typename al template
    LinkedListNode(const T& value, Ref ref, NodePtr pNext) : GeneralNode<T>(value, ref), m_pNext(pNext) {}
};


template <typename T>
class LinkedListForwardIterator : public GeneralIterator<LinkedListForwardIterator<T>, LinkedListNode<T>> {
public:
    using MySelf    = LinkedListForwardIterator<T>;
    using Parent    = GeneralIterator<MySelf, LinkedListNode<T>>;
    using Parent::Parent;

    MySelf& operator++() { 
        Parent::m_ptr = Parent::m_ptr->m_pNext; 
        return *this; 
    }
};


template <typename T, typename CompareT>
struct DefaultTraits {
    using value_type        = T;
    using Compare           = CompareT;
};


template <typename T>
struct AscendingTraits : public DefaultTraits<T, std::less<T>> {
};


template <typename T>
struct DescendingTraits : public DefaultTraits<T, std::greater<T>> {
};

template <typename Traits>
struct LinkedListTraits {
    using value_type        = typename Traits::value_type;
    using Compare           = typename Traits::Compare;
    using Node              = LinkedListNode<value_type>;
    using ForwardIterator   = LinkedListForwardIterator<value_type>;  // itera sobre Node, no sobre T
};


template <typename T>
struct LinkedListAscTraits : public LinkedListTraits<AscendingTraits<T>> {
};


template <typename T>
struct LinkedListDescTraits : public LinkedListTraits<DescendingTraits<T>> {
};


template <typename Traits>
class LinkedList {
    public:
        using value_type        = typename Traits::value_type;
        using Node              = typename Traits::Node;
        using NodePtr           = Node *;
        using ForwardIterator   = typename Traits::ForwardIterator;
        using Compare           = typename Traits::Compare;
        using Delim             = typename Node::Delim;
    
    protected:
        NodePtr m_pRoot = nullptr; // puntero al primer nodo de la lista enlazada
        NodePtr m_pTail = nullptr; // puntero al último nodo de la lista enlazada
        size_t m_size = 0; // cantidad de nodos en la lista enlazada
        Compare m_comp; // comparador para ordenar los nodos de la lista
        Node sentinel;
        mutable std::mutex m_mutex; // mutex para sincronización

        virtual void push_backUnlocked(NodePtr newNode) {
            if (empty()) {
                m_pRoot = m_pTail = newNode;
            } else {
                m_pTail->m_pNext = newNode;
                m_pTail = newNode;
            }
            ++m_size;
        }

        virtual void internalInsert(NodePtr newNode, NodePtr&currentPtr) {
            if(empty() || currentPtr == &sentinel) {
                newNode->m_pNext = &sentinel;
                currentPtr  = newNode;
                m_pTail     = newNode;  
                ++m_size;
                return;
            } else if(m_comp(newNode->getValue(), currentPtr->getValue())) {
                newNode->m_pNext = currentPtr;
                currentPtr = newNode;
                ++m_size;
                return;
            }
            LinkedList::internalInsert(newNode, currentPtr->m_pNext);
        }

        virtual void clearUnlocked() {
            ApplyFunction([](Node& node) { delete &node; });
            m_pRoot = nullptr;
            m_pTail = nullptr;
            m_size  = 0;
        };

    public:
        LinkedList() {}
        LinkedList(const LinkedList& another){ *this = another; } // copia profunda de la lista enlazada
        LinkedList(initializer_list<pair<value_type, Ref>> values) {
            for (const auto &value : values) 
                push_back(value.first, value.second);
        }

        virtual ~LinkedList() { clear(); }
        
        bool empty() const { return size() == 0; }
        
        size_t size() const { return m_size; }
        
        void push_back(const value_type& value, Ref ref) {
            scoped_lock lock(m_mutex);
            NodePtr newNode = new Node(value, ref, &sentinel);
            push_backUnlocked(newNode);
        }
    
        void insert(const value_type& value, Ref ref) { 
            scoped_lock lock(m_mutex); 
            NodePtr newNode = new Node(value, ref, nullptr);
            internalInsert(newNode, m_pRoot);
        }

        void clear() {
            scoped_lock lock(m_mutex);
            clearUnlocked();
        };

        ForwardIterator begin() { return ForwardIterator(m_pRoot ? m_pRoot : &sentinel); }
        
        ForwardIterator end() { return ForwardIterator(&sentinel); }

        template <typename Func, typename... Args>
        void ApplyFunction(Func func, Args... args) { call(func, std::forward<Args>(args)...); }
        
        template <typename Func, typename... Args>
        Node& FirstThat(Func func, Args... args) { return call(func, std::forward<Args>(args)...); }

        template<typename Func, typename... Args>
        decltype(auto) call(Func func, Args&&... args) {
            if constexpr(is_void_v<invoke_result_t<Func, Node&, Args...>>)
                ::call(begin(), end(), std::forward<Func>(func), std::forward<Args>(args)...);
            else // return type is not void:
                return ::call(begin(), end(), std::forward<Func>(func), std::forward<Args>(args)...);
        };

        std::ostream& write(std::ostream& os) { return os << *this; }
        std::istream& read(std::istream& is) { return is >> *this; }
        LinkedList& copy(LinkedList& another) { return *this = another; }

        friend std::ostream& operator <<(std::ostream& os, const LinkedList<Traits>& list) {
            scoped_lock lock(list.m_mutex);
            if(list.empty()) 
                return os;
            
            NodePtr current = list.m_pRoot;
            os << "[";
            while (current != &(list.sentinel)) {
                os << *current;
                current = current->m_pNext;
                if (current != &(list.sentinel)) os << ",";
            }
            return os << "]";
        }
        
        friend std::istream &operator >>(std::istream &is, LinkedList<Traits> &list) {
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

        LinkedList& operator=(LinkedList& another) {
            if (this == &another) return *this;
            scoped_lock lock(m_mutex, another.m_mutex);  
            clearUnlocked();
            another.ApplyFunction([this](const Node& node) {
                push_backUnlocked(node.getValue(), node.getRef());
            });
            m_comp = another.m_comp;
            return *this;
        }
};
#endif // __LINKEDLIST_H__
