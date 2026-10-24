#ifndef __BINARYTREE_H__
#define __BINARYTREE_H__

#include "vector.h"
#include "types.h"

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

    MySelf() = default;
    MySelf(NodePtr left, NodePtr right) : m_pChild{left, right} {}
    MySelf(const value_type& value, Ref ref, NodePtr left, NodePtr right) : Parent(value, ref), m_pChild{left, right} {}
    
    bool hasLeft() const  { return m_pChild[0] != nullptr; }
    bool hasRight() const { return m_pChild[1] != nullptr; }

    NodePtr& getLeft() { return m_pChild[0]; }
    NodePtr& getRight() { return m_pChild[1]; }    
    
    const Node* getLeft() const { return m_pChild[0]; }
    const Node* getRight() const { return m_pChild[1]; }
};

template <typename T, bool IsConst = false>
class BinaryTreePreorderIterator : public Iterator<BinaryTreeNode<T>,IsConst> {

public:
    using MySelf    = BinaryTreePreorderIterator<T,IsConst>;
    using Node      = BinaryTreeNode<T>;
    using Parent    = Iterator<Node,IsConst>;
    using NodePtr   = typename Parent::pointer;
    using Parent::set();

    MySelf(NodePtr ptr) : Parent(ptr) { if (ptr) pushChildren(ptr); }

    MySelf& operator++() override {
        if (stack.empty()) return set(nullptr);

        NodePtr next = stack.pop_back().getValue();

        pushChildren(next);
        return set(next);
    }

private:
    Vector<VectorAscTraits<NodePtr>> stack;

    void pushChildren(NodePtr ptr) {
        if (ptr->hasRight()) stack.push_back(ptr->getRight(), 0);
        if (ptr->hasLeft()) stack.push_back(ptr->getLeft(), 0);
    }
};

template <typename T, bool IsConst = false>
class BinaryTreeRightFirstPostorderIterator : public Iterator<BinaryTreeNode<T>,IsConst> {

public:
    using MySelf    = BinaryTreeRightFirstPostorderIterator<T,IsConst>;
    using Node      = BinaryTreeNode<T>;
    using Parent    = Iterator<Node,IsConst>;
    using NodePtr   = typename Parent::pointer;
    using Parent::set();
    
    MySelf(NodePtr ptr) : Parent(ptr) {
        if (!ptr) return;
        
        NodePtr curr = ptr;

        while (curr->hasRight() || curr->hasLeft()) {
            stack.push_back(curr, 0);
            if (curr->hasRight()) curr = curr->getRight();
            else curr = curr->getLeft();
        }
        set(curr);
    }
    
    friend MySelf& operator++() {
        if (stack.empty()) return set(nullptr);
        
        NodePtr parent = stack.pop_back().getValue();

        if (parent->hasLeft() && parent->getLeft() != this->m_ptr) {
            stack.push_back(parent, 0);
            
            NodePtr curr = parent->getLeft();
            
            while (curr->hasRight() || curr->hasLeft()) {
                stack.push_back(curr, 0);
                if (curr->hasRight()) curr = curr->getRight();
                else curr = curr->getLeft();
            }
            return set(curr);
        }
        return set(parent);
    }

private:
    Vector<VectorAscTraits<NodePtr>> stack;
};

template <typename CompareTraits>
struct BinaryTreeTraits {
        using value_type            = typename CompareTraits::value_type;
        using Compare               = typename CompareTraits::Compare;
        using Node                  = BinaryTreeNode<value_type>;
        using ForwardIterator       = BinaryTreePreorderIterator<value_type>;
        using ConstForwardIterator  = BinaryTreePreorderIterator<value_type,true>;
        using BackwardIterator      = BinaryTreeRightFirstPostorderIterator<value_type>;
        using ConstBackwardIterator = BinaryTreeRightFirstPostorderIterator<value_type,true>;
        using Delim                 = typename Node::Delim;
};

template <typename T>
struct BinaryTreeAscTraits : BinaryTreeTraits<AscendingTraits<T>> {};

template <typename T>
struct BinaryTreeDescTraits : BinaryTreeTraits<DescendingTraits<T>> {};

template <typename Traits>
class BinaryTree : public Container<Traits>, public ReverseIterable<Traits> {
    public:
    using Parent                = Container<Traits>;
    using value_type            = typename Parent::value_type;
    using Node                  = typename Parent::Node;
    using ForwardIterator       = typename Parent::ForwardIterator;
    using ConstForwardIterator  = typename Parent::ConstForwardIterator;
    using BackwardIterator      = typename ReverseIterable<Traits>::BackwardIterator;
    using ConstBackwardIterator = typename ReverseIterable<Traits>::ConstBackwardIterator;
    using NodePtr               = Node*;
    using Parent::empty();
    using Parent::clear();

    BinaryTree() = default;

    BinaryTree(initializer_list<pair<value_type, Ref>> values) {
        for (const auto& value : values) insert(value.first, value.second);
    }

    BinaryTree(BinaryTree&& other) : Parent() {
        std::scoped_lock lock(other.m_mutex);

        m_comp  = std::move(other.m_comp);
        m_pRoot = std::exchange(other.m_pRoot, nullptr);
        m_size  = std::exchange(other.m_size, 0);
    }

    BinaryTree(const BinaryTree& other) : Parent() {
        std::scoped_lock lock(other.m_mutex);

        m_comp = other.m_comp;
        m_pRoot = cloneTree(other.m_pRoot);
        m_size = other.m_size;
    }

    ~BinaryTree() override {
        clear();
    }

    void insert(const value_type& value, Ref ref) {
        scoped_lock lock(m_mutex);
        internalInsert(value, ref);
    }

    ForwardIterator  begin() override  { return ForwardIterator(m_pRoot); }
    ForwardIterator  end() override    { return ForwardIterator(nullptr); }
    BackwardIterator rbegin() override { return BackwardIterator(m_pRoot); }
    BackwardIterator rend() override   { return BackwardIterator(nullptr); }
    ConstForwardIterator cbegin() const override  { return ConstForwardIterator(m_pRoot); }
    ConstForwardIterator cend() const override    { return ConstForwardIterator(nullptr); }
    ConstBackwardIterator rbegin() const override { return BackwardIterator(m_pRoot); }
    ConstBackwardIterator rend() const override   { return BackwardIterator(nullptr); }
    
    friend std::ostream& operator<<(std::ostream& os, const BinaryTree<Traits>& tree) {
        std::scoped_lock lock(tree.m_mutex);

        if(tree.m_pRoot == nullptr) return;
        
        Vector<VectorAscTraits<NodePtr>> queue({tree.m_pRoot, 0});
        size_t lvlEnd = 0;
        
        os << *m_pRoot <<endl;

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
            if (i == levelEnd) {
                os <<endl;
                lvlEnd = queue.size() - 1;
                if (queue.size() == tree.size()) break;
            }
        }
    }

    protected:
    using Parent::m_comp;
    using Parent::m_size;
    using Parent::m_mutex;

    NodePtr m_pRoot = nullptr;
    void push_backUnlocked(const value_type&, Ref) override {
        NodePtr nodePtr = new Node(value, ref);
        push_backUnlocked(nodePtr);
    };

    void push_backUnlocked(NodePtr) override {
        internalInsert(nodePtr);
    };
    
    void internalInsert(const value_type& value, Ref ref) {
        NodePtr nodePtr = new Node(value, ref);
        internalInsert(nodePtr);
    }
    
    void internalInsert(NodePtr nodePtr) {
        NodePtr* currentPtr = &m_pRoot;
        
        while (*currentPtr != nullptr) {
            if(m_comp(nodePtr->getValue(), (*currentPtr)->getValue()))
                currentPtr = &(*currentPtr)->m_pChild[0];
            else
                currentPtr = &(*currentPtr)->m_pChild[1];
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

        destroyTree(node->m_pChild[0]);
        destroyTree(node->m_pChild[1]);
        delete node;
    }

    NodePtr cloneTree(NodePtr node) {
        if (node == nullptr) return nullptr;
        NodePtr copy = new Node(node->getValue(), node->getRef());
            copy->m_pChild[0] = cloneTree(node->m_pChild[0]);
            copy->m_pChild[1] = cloneTree(node->m_pChild[1]);
        return copy;
    }
    
    // Move constructor and move assignment operator
};
#endif // __BINARYTREE_H__