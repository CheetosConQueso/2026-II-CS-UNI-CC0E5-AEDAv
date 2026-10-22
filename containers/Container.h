#ifndef __CONTAINER__
#define __CONTAINER__

#include <Utility>
#include "Iterator.h"
#include "../foreach.h"
#include "../types.h"

template <typename T, typename CompareT> struct CompareTraits {
    using value_type        = T;
    using Compare           = CompareT;
};

template <typename T> struct AscendingTraits : public CompareTraits<T, std::less<T>> {};
template <typename T> struct DescendingTraits : public CompareTraits<T, std::greater<T>> {};

template <typename Traits>
class Container {
    public:
        using value_type            = typename Traits::value_type;
        using Node                  = typename Traits::Node;
        using Compare               = typename Traits::Compare;
        using ForwardIterator       = typename Traits::ForwardIterator;
        using ConstForwardIterator  = typename Traits::ConstForwardIterator;
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
        Container() {}
        Container(const Container& another) { *this = another; }
        
        virtual ~Container() = default ;

        virtual size_t size() const { return m_size; } 
        bool empty() const  { return size() == 0; }

        virtual ForwardIterator         begin()         = 0;
        virtual ForwardIterator         end()           = 0;
        virtual ConstForwardIterator    cbegin () const = 0;
        virtual ConstForwardIterator    cend () const   = 0;

        void push_back(const value_type& v, Ref r) {
            scoped_lock lock(m_mutex);
            push_backUnlocked(v, r);
        }

        void clear() {
            scoped_lock lock(m_mutex);
            clearUnlocked();
        };

        template <typename Func, typename... Args> 
        void ApplyFunction(Func func, Args... args) { 
            call(func, std::forward<Args>(args)...); 
        }
        
        template <typename Func, typename... Args> 
        void cApplyFunction(Func func, Args... args) const { 
            ccall(func, std::forward<Args>(args)...); 
        }

        template <typename Func, typename... Args> 
        Node& FirstThat(Func func, Args... args) { 
            return call(func, std::forward<Args>(args)...); 
        }    

        template <typename Func, typename... Args> 
        const Node& cFirstThat(Func func, Args... args) const { 
            return ccall(func, std::forward<Args>(args)...); 
        }  

        template <typename Func, typename... Args> 
        decltype(auto) call(Func func, Args&&... args) { 
            return ::call(begin(), end(), std::forward<Func>(func), std::forward<Args>(args)...); 
        }

        template <typename Func, typename... Args> 
        decltype(auto) ccall(Func func, Args&&... args) const { 
            return ::ccall(cbegin(), cend(), std::forward<Func>(func), std::forward<Args>(args)...); 
        }

        friend std::ostream& operator<<(std::ostream& os, const Container<Traits>& container) {
            std::scoped_lock lock(container.m_mutex);
            
            bool first = true;

            os << '[';
            container.cApplyFunction([&](const Node& node) { 
                if (!first) os <<',';
                os << node;
                first = false;
            });
            
            return os << ']';
        }

        friend std::istream& operator>>(std::istream& is, Container<Traits>& container) {
            Node node;
            Delim d;

            is >> std::ws >> d;
            if (d != '[') return is;
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

        Container& operator=(const Container& another) {
            if (this == &another) return *this;

            scoped_lock lock(m_mutex, another.m_mutex);  
            
            clearUnlocked();
            another.cApplyFunction([this](const Node& node) {
                push_backUnlocked(node.getValue(), node.getRef());
            });
            m_comp = another.m_comp;
            return *this;
        }

        std::ostream& write(std::ostream& os) { return os << *this; }
        std::istream& read(std::istream& is) { return is >> *this; }
        Container& copy( Container& another) { return *this = another; }
};
#endif // __CONTAINER__