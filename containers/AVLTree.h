
#ifndef __AVLTREE_H__
#define __AVLTREE_H__

#include "BinaryTree.h"
#include <algorithm>
#include <initializer_list>
#include <mutex>
#include <utility>

template <typename T>
class AVLTreeNode : public BinaryTreeNode<T> {
public:
    using Parent = BinaryTreeNode<T>;
    using Parent::Parent;

    int m_height = 1;
};

template <typename T>
struct AVLTreeAscTraits : BinaryTreeAscTraits<T> {};

template <typename T>
struct AVLTreeDescTraits : BinaryTreeDescTraits<T> {};

template <typename Traits>
class AVLTree : public BinaryTree<Traits> {

public:
    using Parent     = BinaryTree<Traits>;
    using value_type = typename Parent::value_type;
    using Node       = AVLTreeNode<value_type>;
    using NodePtr    = Node*;

    AVLTree() = default;

    AVLTree(std::initializer_list<std::pair<value_type, Ref>> values) { for (const auto& [value, ref] : values) this->insert(value, ref); }

    AVLTree(const AVLTree& another) : Parent() {
        std::scoped_lock lock(another.m_mutex);
        this->copy(another);
    }

    AVLTree& operator=(const AVLTree& another) {
        if (this == &another) return *this;

        std::scoped_lock lock(this->m_mutex, another.m_mutex);
        this->copy(another);

        return *this;
    }

protected:
    using Parent::m_size;

    NodePtr createNode(const value_type& value, Ref ref) override {
        return new Node(value, ref);
    }

    void internalInsert(NodePtr node) override {
        insertRecursive(this->m_pRoot, node);
    }

    NodePtr cloneTree(NodePtr node) override {
        if (!node) return nullptr;

        NodePtr copy = createNode(node->getValue(), node->getRef());

        copy->m_height = node->m_height;

        copy->getLeft() = cloneTree(node->getLeft());
        copy->getRight() = cloneTree(node->getRight());

        return copy;
    }

private:

    static void updateHeight(NodePtr node) { node->m_height = 1 + std::max(node->getLeft()->m_height,node->getRight()->m_height); }

    static int bFactor(NodePtr node) { return node->getLeft()->m_height - node->getRight()->m_height; }

    static void rotate(NodePtr& root, int dir) {
        NodePtr pivot = (dir == 0) ? root->getRight() : root->getLeft();

        if (dir == 0) {
            root->getRight() = pivot->getLeft();
            pivot->getLeft() = root;
        } else {
            root->getLeft() = pivot->getRight();
            pivot->getRight() = root;
        }

        updateHeight(root);
        updateHeight(pivot);

        root = pivot;
    }

    static void rebalance(NodePtr& root) {
        if (!root) return;

        updateHeight(root);
        int balance = bFactor(root);
        
        if (balance > 1) {
            if (bFactor(root->getLeft()) < 0) rotate(root->getLeft(), 0);
            rotate(root, 1);
        } else if (balance < -1) {
            if (bFactor(root->getRight()) > 0) rotate(root->getRight(), 1);
            rotate(root, 0);
        }
    }

    void insertRecursive(NodePtr& root, NodePtr node) {
        if (!root) {
            root = node;
            ++m_size;
            return;
        }

        if (this->m_comp(node->getValue(), root->getValue())) insertRecursive(root->getLeft(), node);
        else insertRecursive(root->getRight(), node);
        rebalance(root);
    }
};
#endif // __AVLTREE_H__
