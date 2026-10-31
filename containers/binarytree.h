#ifndef __BINARYTREE_H__
#define __BINARYTREE_H__

#include "vector.h"
#include "../types.h"

template <typename T>
class BinaryTreeNode : public GeneralNode<T> {
public:
    using value_type        = T;
    using MySelf            = BinaryTreeNode<T>;
    using Node              = MySelf;
    using NodePtr           = Node*;
    using Parent            = GeneralNode<T>;
    using Parent::Parent;

    NodePtr m_pChild[2] = {nullptr, nullptr}; // left and right children

    BinaryTreeNode() = default;
    BinaryTreeNode(NodePtr left, NodePtr right) : m_pChild{left, right} {}
    BinaryTreeNode(const value_type& value, Ref ref, NodePtr left, NodePtr right) : Parent(value, ref), m_pChild{left, right} {}
    
    bool hasLeft() const  { return m_pChild[0] != nullptr; }
    bool hasRight() const { return m_pChild[1] != nullptr; }

    NodePtr& getLeft() { return m_pChild[0]; }
    NodePtr& getRight() { return m_pChild[1]; }    
    
    const Node* getLeft() const { return m_pChild[0]; }
    const Node* getRight() const { return m_pChild[1]; }

    void setLeft(NodePtr ptr) { m_pChild[0] = ptr; }
    void setRight(NodePtr ptr) { m_pChild[1] = ptr; }

    virtual ~BinaryTreeNode() = default;
};

template <typename BTIterator>
class Range {
public:
    using NodePtr = typename BTIterator::NodePtr;

    Range(NodePtr root) : m_root(root) {}

    BTIterator begin() const {
        return BTIterator(m_root);
    }

    BTIterator end() const {
        return BTIterator(nullptr);
    }

private:
    NodePtr m_root;
};

template <typename NodePtr, bool Reverse>
struct Traversal {
    static NodePtr getFirst(NodePtr ptr) {
        if constexpr (Reverse) return ptr->getRight();
        else return ptr->getLeft();
    }

    static NodePtr getLast(NodePtr ptr) {
        if constexpr (Reverse) return ptr->getLeft();
        else return ptr->getRight();
    }

    static bool hasFirst(NodePtr ptr) {
        if constexpr (Reverse) return ptr->hasRight();
        else return ptr->hasLeft();
    }

    static bool hasLast(NodePtr ptr) {
        if constexpr (Reverse) return ptr->hasLeft();
        else return ptr->hasRight();
    }
};


template <typename T, bool Reverse = false, bool IsConst = false>
class BTInOrderIterator : public Iterator<BinaryTreeNode<T>, IsConst> {

public:
    using MySelf    = BTInOrderIterator<T, Reverse, IsConst>;
    using Node      = BinaryTreeNode<T>;
    using Parent    = Iterator<Node, IsConst>;
    using NodePtr   = typename Parent::pointer;
    using Trav      = Traversal<NodePtr, Reverse>;
    using Parent::set;

    BTInOrderIterator(NodePtr ptr) : Parent(ptr) { if (ptr) descendFirst(ptr); }

    MySelf& operator++() override {
        if (!this->m_ptr) return *this;

        if (Trav::hasLast(this->m_ptr)) {
            descendFirst(Trav::getLast(this->m_ptr));
            return *this;
        }

        if (stack.empty()) {
            set(nullptr);
            return *this;
        }

        set(stack.pop_back().getValue());
        return *this;
    }

private:
    Vector<VectorAscTraits<NodePtr>> stack;

    void descendFirst(NodePtr ptr) {
        while (Trav::hasFirst(ptr)) {
            stack.push_back(ptr, 0);
            ptr = Trav::getFirst(ptr);
        }
        set(ptr);
    }
};


template <typename T, bool Reverse = false, bool IsConst = false>
class BTPreorderIterator : public Iterator<BinaryTreeNode<T>,IsConst> {

public:
    using MySelf    = BTPreorderIterator<T, Reverse, IsConst>;
    using Node      = BinaryTreeNode<T>;
    using Parent    = Iterator<Node,IsConst>;
    using NodePtr   = typename Parent::pointer;
    using Trav      = Traversal<NodePtr, Reverse>;
    using Parent::set;
    
    BTPreorderIterator(NodePtr ptr) : Parent(ptr) { if (ptr) pushChildren(ptr); }

    MySelf& operator++() override {
        if (stack.empty()) {
            set(nullptr);
            return *this;
        }

        NodePtr next = stack.pop_back().getValue();

        pushChildren(next);
        set(next);
        return *this;
    }

private:
    Vector<VectorAscTraits<NodePtr>> stack;

    void pushChildren(NodePtr ptr) {
            if (Trav::hasLast(ptr)) stack.push_back(Trav::getLast(ptr), 0);
            if (Trav::hasFirst(ptr)) stack.push_back(Trav::getFirst(ptr), 0);
    }
};

template <typename T, bool Reverse, bool IsConst = false>
class BTPostorderIterator : public Iterator<BinaryTreeNode<T>,IsConst> {

public:
    using MySelf    = BTPostorderIterator<T, Reverse, IsConst>;
    using Node      = BinaryTreeNode<T>;
    using Parent    = Iterator<Node,IsConst>;
    using NodePtr   = typename Parent::pointer;
    using Parent::set;

    using Trav = Traversal<NodePtr, Reverse>;
    
    BTPostorderIterator(NodePtr ptr) : Parent(ptr) {
        if (!ptr) return;
        
        NodePtr curr = ptr;

        while (Trav::hasFirst(curr) || Trav::hasLast(curr)) {
            stack.push_back(curr, 0);
            if (Trav::hasFirst(curr)) curr = Trav::getFirst(curr);
            else curr = Trav::getLast(curr);
        }
        set(curr);
    }
    
    MySelf& operator++() override {
        if (stack.empty()) {
            set(nullptr);
            return *this;
        }
        
        NodePtr parent = stack.pop_back().getValue();

        if (Trav::hasLast(parent) && Trav::getLast(parent) != this->m_ptr) {
            stack.push_back(parent, 0);
            
            NodePtr curr = Trav::getLast(parent);
            
            while (Trav::hasFirst(curr) || Trav::hasLast(curr)) {
                stack.push_back(curr, 0);
                if (Trav::hasFirst(curr)) curr = Trav::getFirst(curr);
                else curr = Trav::getLast(curr);
            }

            set(curr);
            return *this;
        }
        set(parent);
        return *this;
    }

private:
    Vector<VectorAscTraits<NodePtr>> stack;
};

template <typename CompareTraits>
struct BinaryTreeTraits {
        using value_type                = typename CompareTraits::value_type;
        using Compare                   = typename CompareTraits::Compare;
        using Node                      = BinaryTreeNode<value_type>;
        using Delim                     = typename Node::Delim;

        using ForwardIterator           = BTPreorderIterator<value_type>;
        using ConstForwardIterator      = BTPreorderIterator<value_type, false, true>;
        using BackwardIterator          = BTPostorderIterator<value_type, true>;
        using ConstBackwardIterator     = BTPostorderIterator<value_type, true, true>;

        using InOrderIterator           = BTInOrderIterator<value_type, false>;
        using ReverseInOrderIterator    = BTInOrderIterator<value_type, true>;

        using PreOrderIterator          = BTPreorderIterator<value_type, false>;
        using ReversePreOrderIterator   = BTPreorderIterator<value_type, true>;

        using PostOrderIterator         = BTPostorderIterator<value_type, false>;
        using ReversePostOrderIterator  = BTPostorderIterator<value_type, true>;
        
};

template <typename T>
struct BinaryTreeAscTraits : BinaryTreeTraits<AscendingTraits<T>> {};

template <typename T>
struct BinaryTreeDescTraits : BinaryTreeTraits<DescendingTraits<T>> {};

template <typename Traits>
class BinaryTree : 
    public Container<Traits>,
    public BackwardIterable<Traits> {

public:
    using Parent                = Container<Traits>;
    
    using value_type            = typename Parent::value_type;
    using Node                  = typename Parent::Node;
    
    using ForwardIterator       = typename ForwardIterable<Traits>::ForwardIterator;
    using ConstForwardIterator  = typename ForwardIterable<Traits>::ConstForwardIterator;
    
    using BackwardIterator      = typename BackwardIterable<Traits>::BackwardIterator;
    using ConstBackwardIterator = typename BackwardIterable<Traits>::ConstBackwardIterator;
    
    using InOrderIterator           = typename Traits::InOrderIterator;
    using ReverseInOrderIterator    = typename Traits::ReverseInOrderIterator;
    
    using PreOrderIterator          = typename Traits::PreOrderIterator;
    using ReversePreOrderIterator   = typename Traits::ReversePreOrderIterator;
    
    using PostOrderIterator         = typename Traits::PostOrderIterator;
    using ReversePostOrderIterator  = typename Traits::ReversePostOrderIterator;

    using NodePtr               = Node*;
    
    using Parent::empty;
    using Parent::clear;

protected:
    using Parent::m_comp;
    using Parent::m_size;
    using Parent::m_mutex;

    NodePtr m_pRoot = nullptr;

public:
    BinaryTree() = default;

    BinaryTree(initializer_list<pair<value_type, Ref>> values) { for (const auto& value : values) insert(value.first, value.second); }

    BinaryTree(BinaryTree&& another) : Parent() {
        std::scoped_lock lock(another.m_mutex);

        m_comp  = std::move(another.m_comp);
        m_pRoot = std::exchange(another.m_pRoot, nullptr);
        m_size  = std::exchange(another.m_size, 0);
    }

    BinaryTree(const BinaryTree& another) : Parent() {
        std::scoped_lock lock(another.m_mutex);
        copy(another);
    }

    ~BinaryTree() override { clear(); }

    void insert(const value_type& value, Ref ref) {
        scoped_lock lock(m_mutex);
        internalInsert(value, ref);
    }

    void remove(const value_type& value, Ref ref) {
        scoped_lock lock (m_mutex);

        NodePtr* curr = &m_pRoot;

        while (*curr && ((*curr)->getValue() != value || (*curr)->getRef() != ref)) {
            if (m_comp(value, (*curr)->getValue())) 
                curr = &(*curr)->getLeft();
            else 
                curr = &(*curr)->getRight();
        }
        
        if (!*curr) return;

        NodePtr* toReplace = curr;
        NodePtr toDelete  = *toReplace;

        if (!(*toReplace)->hasLeft()) { *curr = (*toReplace)->getRight(); }
        else if (!(*toReplace)->hasRight()) { *curr = (*toReplace)->getLeft(); }
        else {
            curr = &(*curr)->getLeft();
            while ((*curr)->hasRight()) curr = &(*curr)->getRight();

            NodePtr replacement = *curr;
            *curr = replacement->getLeft();

            replacement->setLeft((*toReplace)->getLeft());
            replacement->setRight((*toReplace)->getRight());

            *toReplace = replacement;
        }

        delete toDelete;
        --m_size;
    }

    ForwardIterator  begin() override  { return ForwardIterator(m_pRoot); }
    ForwardIterator  end() override    { return ForwardIterator(nullptr); }
    ConstForwardIterator cbegin() const override  { return ConstForwardIterator(m_pRoot); }
    ConstForwardIterator cend() const override    { return ConstForwardIterator(nullptr); }
    
    BackwardIterator rbegin() override { return BackwardIterator(m_pRoot); }
    BackwardIterator rend() override   { return BackwardIterator(nullptr); }
    ConstBackwardIterator crbegin() const override { return ConstBackwardIterator(m_pRoot); }
    ConstBackwardIterator crend() const override   { return ConstBackwardIterator(nullptr); }
    
    auto inOrder()          { return Range<InOrderIterator>(m_pRoot); }
    auto reverseInOrder()   { return Range<ReverseInOrderIterator>(m_pRoot); }
    auto preOrder()         { return Range<PreOrderIterator>(m_pRoot); }
    auto reversePreOrder()  { return Range<ReversePreOrderIterator>(m_pRoot); }
    auto postOrder()        { return Range<PostOrderIterator>(m_pRoot); }
    auto reversePostOrder() { return Range<ReversePostOrderIterator>(m_pRoot); }
        
    friend std::ostream& operator<<(std::ostream& os, const BinaryTree<Traits>& tree) {
        std::scoped_lock lock(tree.m_mutex);

        if(tree.m_pRoot == nullptr) return os;
        
        Vector<VectorAscTraits<NodePtr>> queue;
        queue.push_back(tree.m_pRoot, 0);
        size_t lvlEnd = 0;
        
        os << *tree.m_pRoot <<endl;

        for (size_t i = 0; i < queue.size(); i++) {
            NodePtr curr = queue[i].getValue();
            
            os <<"[";
            if (curr->hasLeft()) {
                NodePtr left = curr->getLeft();
                os <<*left;
                queue.push_back(left, 0);
            }    
            if (curr->hasRight()) {
                NodePtr right = curr->getRight();
                os <<*right;
                queue.push_back(right, 0);
            }
            os << "]";
            if (i == lvlEnd) {
                os <<endl;
                lvlEnd = queue.size() - 1;
                if (queue.size() == tree.size()) break;
            }
        }
        return os;
    }

    BinaryTree& operator=(const BinaryTree& another) { 
        scoped_lock lock(m_mutex);
        return copy(another); 
    }

protected:
    void push_backUnlocked(const value_type& value, Ref ref) override {
        NodePtr nodePtr = createNode(value, ref);
        push_backUnlocked(nodePtr);
    };

    void push_backUnlocked(NodePtr nodePtr) override {
        internalInsert(nodePtr);
    };
    
    void internalInsert(const value_type& value, Ref ref) {
        NodePtr nodePtr = createNode(value, ref);
        internalInsert(nodePtr);
    }
    
    virtual void internalInsert(NodePtr nodePtr) {
        NodePtr* currentPtr = &m_pRoot;
        
        while (*currentPtr != nullptr) {
            if(m_comp(nodePtr->getValue(), (*currentPtr)->getValue()))
                currentPtr = &(*currentPtr)->getLeft();
            else
                currentPtr = &(*currentPtr)->getRight();
        }

       *currentPtr = nodePtr;
       ++m_size;
    }

    void clearUnlocked() override {
        destroyTree(m_pRoot);
        m_pRoot = nullptr;
        m_size = 0;
    }

    void destroyTree(NodePtr node) {
        if (node == nullptr) return;

        destroyTree(node->getLeft());
        destroyTree(node->getRight());
        delete node;
    }

    virtual NodePtr createNode(const value_type& value, Ref ref) {
        return new Node(value, ref);
    }

    BinaryTree& copy(const BinaryTree& another) { 
        if (this == &another) return *this;
        
        clearUnlocked();

        m_pRoot = cloneTree(another.m_pRoot);
        m_size = another.m_size;
        m_comp = another.m_comp;
        
        return *this;
    }

    virtual NodePtr cloneTree(NodePtr node) {
        if (!node) return nullptr;

        NodePtr copy = createNode(node->getValue(), node->getRef());
        copy->getLeft() = cloneTree(node->getLeft());
        copy->getRight() = cloneTree(node->getRight());

        return copy;
    }
};
#endif // __BINARYTREE_H__