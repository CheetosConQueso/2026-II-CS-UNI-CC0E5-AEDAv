#ifndef __CIRCULARLINKEDLIST_H__
#define __CIRCULARLINKEDLIST_H__
#include <mutex>
#include "GeneralNode.h"
#include "LinkedList.h"
#include "../foreach.h"

template <typename Traits>
class CircularLinkedList : public LinkedList<Traits> {
    public:
        using Parent            = LinkedList<Traits>;
        using value_type        = typename Parent::value_type;
        
        void push_back(const value_type& value, Ref ref) override {
            Parent::push_back(value, ref);
            scoped_lock lock(this->m_mutex);
            if(this->size() == 1) this->sentinel.m_pNext = this->m_pRoot;
        }

        void insert(const value_type& value, Ref ref) override { 
            Parent::insert(value, ref);
            scoped_lock lock(this->m_mutex);
            this->sentinel.m_pNext = this->m_pRoot;
        }

        void clear() override {
            Parent::clear();
            scoped_lock lock(this->m_mutex);
            this->sentinel.m_pNext = nullptr;
        };

        Parent& operator= (Parent& another) override {
            LinkedList<Traits>::operator=(another);
            scoped_lock lock(this->m_mutex);
            this->sentinel.m_pNext = this->m_pRoot;
            return *this;
        }
};
#endif // __CIRCULARLINKEDLIST_H__
