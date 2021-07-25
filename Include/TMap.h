/***************************************************************************************************
 * MIT License
 *
 * Copyright (c) 2021 antmuse@live.cn/antmuse@qq.com
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
***************************************************************************************************/


#ifndef APP_TMAP_H
#define APP_TMAP_H

#include "Config.h"

namespace app {

//! TMap template for associative arrays using a red-black tree
template <class TKey, class TValue>
class TMap {
    //red-black-tree for TMap
    template <class TkeyRB, class TValueRB>
    class RBTree {
    public:
        RBTree(const TkeyRB& k, const TValueRB& v) : mLeftChild(0), mRightChild(0),
            mParent(0), mKey(k),
            mValue(v), IsRed(true) {
        }

        void setLeftChild(RBTree* p) {
            mLeftChild = p;
            if(p) {
                p->setParent(this);
            }
        }

        void setRightChild(RBTree* p) {
            mRightChild = p;
            if(p) {
                p->setParent(this);
            }
        }

        void setParent(RBTree* p) { mParent = p; }

        void setValue(const TValueRB& v) { mValue = v; }

        void setRed() { IsRed = true; }

        void setBlack() { IsRed = false; }

        RBTree* getLeftChild() const { return mLeftChild; }

        RBTree* getRightChild() const { return mRightChild; }

        RBTree* getParent() const { return mParent; }

        const TValueRB& getValue() const {
            return mValue;
        }

        TValueRB& getValue() {
            return mValue;
        }

        const TkeyRB& getKey() const {
            return mKey;
        }

        bool isRoot() const {
            return mParent == 0;
        }

        bool isLeftChild() const {
            return (mParent != 0) && (mParent->getLeftChild() == this);
        }

        bool isRightChild() const {
            return (mParent != 0) && (mParent->getRightChild() == this);
        }

        bool isLeaf() const {
            return (mLeftChild == 0) && (mRightChild == 0);
        }

        usz getLevel() const {
            usz ret = 1; //this
            for(RBTree* nd = getParent(); nd; nd = nd->getParent()) {
                ++ret;
            }
            return ret;
        }

        bool isRed() const {
            return IsRed;
        }

        bool isBlack() const {
            return !IsRed;
        }

    private:
        RBTree();

        RBTree* mLeftChild;
        RBTree* mRightChild;
        RBTree* mParent;
        TkeyRB	mKey;
        TValueRB mValue;
        bool IsRed;
    };// RBTree

public:
    using Node = RBTree<TKey, TValue>;

    // We need the forwad declaration for the friend declaration
    class ConstIterator;

    //! Normal Iterator
    class Iterator {
        friend class ConstIterator;
    public:

        Iterator() : mRoot(0), mCurr(0) { }

        Iterator(Node* root) : mRoot(root) {
            reset();
        }

        Iterator(const Iterator& src) : mRoot(src.mRoot), mCurr(src.mCurr) { }

        void reset(bool atLowest = true) {
            mCurr = atLowest ? getMin(mRoot) : getMax(mRoot);
        }

        bool atEnd() const {
            return mCurr == 0;
        }

        Node* getNode() const {
            return mCurr;
        }

        Iterator& operator=(const Iterator& src) {
            mRoot = src.mRoot;
            mCurr = src.mCurr;
            return (*this);
        }

        void operator++(s32) {
            inc();
        }

        void operator++() {
            inc();
        }

        void operator--(s32) {
            dec();
        }

        void operator--() {
            dec();
        }

        Node* operator->() {
            return getNode();
        }

        Node& operator*() {
            DASSERT(!atEnd());
            return *mCurr;
        }

    private:

        Node* getMin(Node* n) const {
            while(n && n->getLeftChild()) {
                n = n->getLeftChild();
            }
            return n;
        }

        Node* getMax(Node* n) const {
            while(n && n->getRightChild()) {
                n = n->getRightChild();
            }
            return n;
        }

        void inc() {
            // Already at end?
            if(mCurr == 0) {
                return;
            }
            if(mCurr->getRightChild()) {
                // If current node has a right child, the next higher node is the
                // node with lowest key beneath the right child.
                mCurr = getMin(mCurr->getRightChild());
            } else if(mCurr->isLeftChild()) {
                // No right child? Well if current node is a left child then
                // the next higher node is the parent
                mCurr = mCurr->getParent();
            } else {
                // Current node neither is left child nor has a right child.
                // Ie it is either right child or root
                // The next higher node is the parent of the first non-right
                // child (ie either a left child or the root) up in the
                // hierarchy. mRoot's parent is 0.
                while(mCurr->isRightChild())
                    mCurr = mCurr->getParent();
                mCurr = mCurr->getParent();
            }
        }

        void dec() {
            // Already at end?
            if(mCurr == 0)
                return;

            if(mCurr->getLeftChild()) {
                // If current node has a left child, the next lower node is the
                // node with highest key beneath the left child.
                mCurr = getMax(mCurr->getLeftChild());
            } else if(mCurr->isRightChild()) {
                // No left child? Well if current node is a right child then
                // the next lower node is the parent
                mCurr = mCurr->getParent();
            } else {
                // Current node neither is right child nor has a left child.
                // Ie it is either left child or root
                // The next higher node is the parent of the first non-left
                // child (ie either a right child or the root) up in the
                // hierarchy. mRoot's parent is 0.

                while(mCurr->isLeftChild())
                    mCurr = mCurr->getParent();
                mCurr = mCurr->getParent();
            }
        }

        Node* mRoot;
        Node* mCurr;
    }; // Iterator

    //! Const Iterator
    class ConstIterator {
        friend class Iterator;
    public:

        ConstIterator() : mRoot(0), mCurr(0) { }

        // Constructor(Node*)
        ConstIterator(const Node* root) : mRoot(root) {
            reset();
        }

        // Copy constructor
        ConstIterator(const ConstIterator& src) : mRoot(src.mRoot), mCurr(src.mCurr) { }
        ConstIterator(const Iterator& src) : mRoot(src.mRoot), mCurr(src.mCurr) { }

        void reset(bool atLowest = true) {
            mCurr = atLowest ? getMin(mRoot) : getMax(mRoot);
        }

        bool atEnd() const {
            return mCurr == 0;
        }

        const Node* getNode() const {
            return mCurr;
        }

        ConstIterator& operator=(const ConstIterator& src) {
            mRoot = src.mRoot;
            mCurr = src.mCurr;
            return (*this);
        }

        void operator++(s32) {
            inc();
        }

        void operator++() {
            inc();
        }

        void operator--(s32) {
            dec();
        }

        void operator--() {
            dec();
        }

        const Node* operator->() {
            return getNode();
        }

        const Node& operator*() {
            DASSERT(!atEnd()); // access violation
            return *mCurr;
        }

    private:

        const Node* getMin(const Node* n) const {
            while(n && n->getLeftChild()) {
                n = n->getLeftChild();
            }
            return n;
        }

        const Node* getMax(const Node* n) const {
            while(n && n->getRightChild()) {
                n = n->getRightChild();
            }
            return n;
        }

        void inc() {
            // Already at end?
            if(mCurr == 0) {
                return;
            }
            if(mCurr->getRightChild()) {
                // If current node has a right child, the next higher node is the
                // node with lowest key beneath the right child.
                mCurr = getMin(mCurr->getRightChild());
            } else if(mCurr->isLeftChild()) {
                // No right child? Well if current node is a left child then
                // the next higher node is the parent
                mCurr = mCurr->getParent();
            } else {
                // Current node neither is left child nor has a right child.
                // Ie it is either right child or root
                // The next higher node is the parent of the first non-right
                // child (ie either a left child or the root) up in the
                // hierarchy. mRoot's parent is 0.
                while(mCurr->isRightChild())
                    mCurr = mCurr->getParent();
                mCurr = mCurr->getParent();
            }
        }

        void dec() {
            // Already at end?
            if(mCurr == 0) {
                return;
            }
            if(mCurr->getLeftChild()) {
                // If current node has a left child, the next lower node is the
                // node with highest key beneath the left child.
                mCurr = getMax(mCurr->getLeftChild());
            } else if(mCurr->isRightChild()) {
                // No left child? Well if current node is a right child then
                // the next lower node is the parent
                mCurr = mCurr->getParent();
            } else {
                // Current node neither is right child nor has a left child.
                // Ie it is either left child or root
                // The next higher node is the parent of the first non-left
                // child (ie either a right child or the root) up in the
                // hierarchy. mRoot's parent is 0.

                while(mCurr->isLeftChild())
                    mCurr = mCurr->getParent();
                mCurr = mCurr->getParent();
            }
        }

        const Node* mRoot;
        const Node* mCurr;
    }; // ConstIterator


    //! Parent First Iterator.
    /** Traverses the tree from top to bottom. Typical usage is
    when storing the tree structure, because when reading it
    later (and inserting elements) the tree structure will
    be the same. */
    class ParentFirstIterator {
    public:

        ParentFirstIterator() : mRoot(0), mCurr(0) { }

        explicit ParentFirstIterator(Node* root) : mRoot(root), mCurr(0) {
            reset();
        }

        void reset() {
            mCurr = mRoot;
        }

        bool atEnd() const {
            return mCurr == 0;
        }

        Node* getNode() {
            return mCurr;
        }

        ParentFirstIterator& operator=(const ParentFirstIterator& src) {
            mRoot = src.mRoot;
            mCurr = src.mCurr;
            return (*this);
        }

        void operator++(s32) {
            inc();
        }

        void operator++() {
            inc();
        }

        Node* operator->() {
            return getNode();
        }

        Node& operator*() {
            DASSERT(!atEnd()); // access violation
            return *getNode();
        }

    private:

        void inc() {
            // Already at end?
            if(mCurr == 0) {
                return;
            }
            // First we try down to the left
            if(mCurr->getLeftChild()) {
                mCurr = mCurr->getLeftChild();
            } else if(mCurr->getRightChild()) {
                // No left child? The we go down to the right.
                mCurr = mCurr->getRightChild();
            } else {
                // No children? Move up in the hierarcy until
                // we either reach 0 (and are finished) or
                // find a right uncle.
                while(mCurr != 0) {
                    // But if parent is left child and has a right "uncle" the parent
                    // has already been processed but the uncle hasn't. Move to
                    // the uncle.
                    if(mCurr->isLeftChild() && mCurr->getParent()->getRightChild()) {
                        mCurr = mCurr->getParent()->getRightChild();
                        return;
                    }
                    mCurr = mCurr->getParent();
                }
            }
        }

        Node* mRoot;
        Node* mCurr;

    }; // ParentFirstIterator


    //! Parent Last Iterator
    /** Traverse the tree from bottom to top.
    Typical usage is when deleting all elements in the tree
    because you must delete the children before you delete
    their parent. */
    class ParentLastIterator {
    public:
        ParentLastIterator() : mRoot(0), mCurr(0) { }

        explicit ParentLastIterator(Node* root) : mRoot(root), mCurr(0) {
            reset();
        }

        void reset() {
            mCurr = getMin(mRoot);
        }

        bool atEnd() const {
            return mCurr == 0;
        }

        Node* getNode() {
            return mCurr;
        }

        ParentLastIterator& operator=(const ParentLastIterator& src) {
            mRoot = src.mRoot;
            mCurr = src.mCurr;
            return (*this);
        }

        void operator++(s32) {
            inc();
        }

        void operator++() {
            inc();
        }

        Node* operator->() {
            return getNode();
        }

        Node& operator*() {
            DASSERT(!atEnd()); // access violation
            return *getNode();
        }

    private:

        Node* getMin(Node* n) {
            while(n != 0 && (n->getLeftChild() != 0 || n->getRightChild() != 0)) {
                if(n->getLeftChild()) {
                    n = n->getLeftChild();
                } else {
                    n = n->getRightChild();
                }
            }
            return n;
        }

        void inc() {
            // Already at end?
            if(mCurr == 0) {
                return;
            }
            // Note: Starting point is the node as far down to the left as possible.

            // If current node has an uncle to the right, go to the
            // node as far down to the left from the uncle as possible
            // else just go up a level to the parent.
            if(mCurr->isLeftChild() && mCurr->getParent()->getRightChild()) {
                mCurr = getMin(mCurr->getParent()->getRightChild());
            } else {
                mCurr = mCurr->getParent();
            }
        }

        Node* mRoot;
        Node* mCurr;
    }; // ParentLastIterator


    // AccessClass is a temporary class used with the [] operator.
    // It makes it possible to have different behavior in situations like:
    // myTree["Foo"] = 32;
    // If "Foo" already exists update its value else insert a new element.
    // s32 i = myTree["Foo"]
    // If "Foo" exists return its value.
    class AccessClass {
        // Let TMap be the only one who can instantiate this class.
        friend class TMap<TKey, TValue>;

    public:

        // Assignment operator. Handles the myTree["Foo"] = 32; situation
        void operator=(const TValue& value) {
            // Just use the Set method, it handles already exist/not exist situation
            Tree.set(mKey, value);
        }

        // TValue operator
        operator TValue() {
            Node* node = Tree.find(mKey);
            DASSERT(node);
            return node->getValue();
        }

    private:

        AccessClass(TMap& tree, const TKey& key) : Tree(tree), mKey(key) { }

        AccessClass();

        TMap& Tree;
        const TKey& mKey;
    }; // AccessClass


    TMap() : mRoot(nullptr), mSize(0) { }

    ~TMap() {
        clear();
    }

    //------------------------------
    // Public Commands
    //------------------------------

    //! Inserts a new node into the tree
    /** \param keyNew: the index for this value
    \param v: the value to insert
    \return True if successful, false if it fails (already exists) */
    bool insert(const TKey& keyNew, const TValue& v) {
        // First insert node the "usual" way (no fancy balance logic yet)
        Node* newNode = new Node(keyNew, v);
        if(!insert(newNode)) {
            delete newNode;
            return false;
        }

        // Then attend a balancing party
        while(!newNode->isRoot() && (newNode->getParent()->isRed())) {
            if(newNode->getParent()->isLeftChild()) {
                // If newNode is a left child, get its right 'uncle'
                Node* newNodesUncle = newNode->getParent()->getParent()->getRightChild();
                if(newNodesUncle != 0 && newNodesUncle->isRed()) {
                    // case 1 - change the colors
                    newNode->getParent()->setBlack();
                    newNodesUncle->setBlack();
                    newNode->getParent()->getParent()->setRed();
                    // Move newNode up the tree
                    newNode = newNode->getParent()->getParent();
                } else {
                    // newNodesUncle is a black node
                    if(newNode->isRightChild()) {
                        // and newNode is to the right
                        // case 2 - move newNode up and rotate
                        newNode = newNode->getParent();
                        rotateLeft(newNode);
                    }
                    // case 3
                    newNode->getParent()->setBlack();
                    newNode->getParent()->getParent()->setRed();
                    rotateRight(newNode->getParent()->getParent());
                }
            } else {
                // If newNode is a right child, get its left 'uncle'
                Node* newNodesUncle = newNode->getParent()->getParent()->getLeftChild();
                if(newNodesUncle != 0 && newNodesUncle->isRed()) {
                    // case 1 - change the colors
                    newNode->getParent()->setBlack();
                    newNodesUncle->setBlack();
                    newNode->getParent()->getParent()->setRed();
                    // Move newNode up the tree
                    newNode = newNode->getParent()->getParent();
                } else {
                    // newNodesUncle is a black node
                    if(newNode->isLeftChild()) {
                        // and newNode is to the left
                        // case 2 - move newNode up and rotate
                        newNode = newNode->getParent();
                        rotateRight(newNode);
                    }
                    // case 3
                    newNode->getParent()->setBlack();
                    newNode->getParent()->getParent()->setRed();
                    rotateLeft(newNode->getParent()->getParent());
                }

            }
        }
        // Color the root black
        mRoot->setBlack();
        return true;
    }

    //! Replaces the value if the key already exists, otherwise inserts a new element.
    /** \param k The index for this value
    \param v The new value of */
    void set(const TKey& k, const TValue& v) {
        Node* p = find(k);
        if(p) {
            p->setValue(v);
        } else {
            insert(k, v);
        }
    }

    //! Removes a node from the tree and returns it.
    /** The returned node must be deleted by the user
    \param k the key to remove
    \return A pointer to the node, or 0 if not found */
    Node* delink(const TKey& k) {
        Node* p = find(k);
        if(p == 0) {
            return 0;
        }
        // Rotate p down to the left until it has no right child, will get there
        // sooner or later.
        while(p->getRightChild()) {
            // "Pull up my right child and let it knock me down to the left"
            rotateLeft(p);
        }
        // p now has no right child but might have a left child
        Node* left = p->getLeftChild();

        // Let p's parent point to p's child instead of point to p
        if(p->isLeftChild()) {
            p->getParent()->setLeftChild(left);
        } else if(p->isRightChild()) {
            p->getParent()->setRightChild(left);
        } else {
            // p has no parent => p is the root.
            // Let the left child be the new root.
            setRoot(left);
        }

        // p is now gone from the tree in the sense that
        // no one is pointing at it, so return it.

        --mSize;
        return p;
    }

    //! Removes a node from the tree and deletes it.
    /** \return True if the node was found and deleted */
    bool remove(const TKey& k) {
        return remove(find(k));
    }

    //! Removes a node from the tree and deletes it.
    /** \return True if the node was found and deleted */
    bool remove(Node* p) {
        if(p == 0) {
            return false;
        }

        // Rotate p down to the left until it has no right child, will get there
        // sooner or later.
        while(p->getRightChild()) {
            // "Pull up my right child and let it knock me down to the left"
            rotateLeft(p);
        }
        // p now has no right child but might have a left child
        Node* left = p->getLeftChild();

        // Let p's parent point to p's child instead of point to p
        if(p->isLeftChild()) {
            p->getParent()->setLeftChild(left);
        } else if(p->isRightChild()) {
            p->getParent()->setRightChild(left);
        } else {
            // p has no parent => p is the root.
            // Let the left child be the new root.
            setRoot(left);
        }

        // p is now gone from the tree in the sense that
        // no one is pointing at it. Let's get rid of it.
        delete p;

        --mSize;
        return true;
    }

    void clear() {
        ParentLastIterator i(getParentLastIterator());

        while(!i.atEnd()) {
            Node* p = i.getNode();
            i++; // Increment it before it is deleted
                // else iterator will get quite confused.
            delete p;
        }
        mRoot = 0;
        mSize = 0;
    }

    bool empty() const {
        return mRoot == 0;
    }

    //! Search for a node with the specified key.
    //! \param keyToFind: The key to find
    //! \return Returns 0 if node couldn't be found.
    Node* find(const TKey& keyToFind) const {
        Node* pNode = mRoot;

        while(pNode != 0) {
            const TKey& key = pNode->getKey();
            if(keyToFind == key) {
                return pNode;
            } else if(keyToFind < key) {
                pNode = pNode->getLeftChild();
            } else {//keyToFind > key
                pNode = pNode->getRightChild();
            }
        }

        return 0;
    }

    //! Gets the root element.
    //! \return Returns a pointer to the root node, or
    //! 0 if the tree is empty.
    Node* getRoot() const {
        return mRoot;
    }

    //return the number of nodes in the tree.
    usz size() const {
        return mSize;
    }

    //! Swap the content of this TMap container with the content of another TMap
    /** Afterwards this object will contain the content of the other object and the other
    object will contain the content of this object. Iterators will afterwards be valid for
    the swapped object.
    \param other Swap content with this object	*/
    void swap(TMap<TKey, TValue>& other) {
        AppSwap(mRoot, other.mRoot);
        AppSwap(mSize, other.mSize);
    }

    //------------------------------
    // Public Iterators
    //------------------------------

    //! Returns an iterator
    Iterator getIterator() const {
        Iterator it(getRoot());
        return it;
    }

    //! Returns a Constiterator
    ConstIterator getConstIterator() const {
        Iterator it(getRoot());
        return it;
    }

    //! Returns a ParentFirstIterator.
    //! Traverses the tree from top to bottom. Typical usage is
    //! when storing the tree structure, because when reading it
    //! later (and inserting elements) the tree structure will
    //! be the same.
    ParentFirstIterator getParentFirstIterator() const {
        ParentFirstIterator it(getRoot());
        return it;
    }

    //! Returns a ParentLastIterator to traverse the tree from
    //! bottom to top.
    //! Typical usage is when deleting all elements in the tree
    //! because you must delete the children before you delete
    //! their parent.
    ParentLastIterator getParentLastIterator() const {
        ParentLastIterator it(getRoot());
        return it;
    }

    //------------------------------
    // Public Operators
    //------------------------------

    //! operator [] for access to elements
    /** for example myMap["key"] */
    AccessClass operator[](const TKey& k) {
        return AccessClass(*this, k);
    }


private:
    TMap(const TMap&) = delete;
    TMap(TMap&&) = delete;
    TMap& operator=(const TMap&) = delete;
    TMap& operator=(TMap&&) = delete;

    //! Set node as new root.
    /** The node will be set to black, otherwise core dumps may arise
    (patch provided by rogerborg).
    \param newRoot Node which will be the new root
    */
    void setRoot(Node* newRoot) {
        mRoot = newRoot;
        if(mRoot) {
            mRoot->setParent(nullptr);
            mRoot->setBlack();
        }
    }

    //! Insert a node into the tree without using any fancy balancing logic.
    /** \return false if that key already exist in the tree. */
    bool insert(Node* newNode) {
        bool result = true;

        if(nullptr == mRoot) {
            setRoot(newNode);
            mSize = 1;
        } else {
            Node* pNode = mRoot;
            const TKey& keyNew = newNode->getKey();
            while(pNode) {
                const TKey& key = pNode->getKey();
                if(keyNew == key) {
                    result = false;
                    pNode = 0;
                } else if(keyNew < key) {
                    if(pNode->getLeftChild() == 0) {
                        pNode->setLeftChild(newNode);
                        pNode = 0;
                    } else {
                        pNode = pNode->getLeftChild();
                    }
                } else {// keyNew > key
                    if(pNode->getRightChild() == 0) {
                        pNode->setRightChild(newNode);
                        pNode = 0;
                    } else {
                        pNode = pNode->getRightChild();
                    }
                }
            }

            if(result) {
                ++mSize;
            }
        }

        return result;
    }

    //! Rotate left.
    //! Pull up node's right child and let it knock node down to the left
    void rotateLeft(Node* p) {
        Node* right = p->getRightChild();
        p->setRightChild(right->getLeftChild());

        if(p->isLeftChild()) {
            p->getParent()->setLeftChild(right);
        } else if(p->isRightChild()) {
            p->getParent()->setRightChild(right);
        } else {
            setRoot(right);
        }
        right->setLeftChild(p);
    }

    //! Rotate right.
    //! Pull up node's left child and let it knock node down to the right
    void rotateRight(Node* p) {
        Node* left = p->getLeftChild();
        p->setLeftChild(left->getRightChild());

        if(p->isLeftChild()) {
            p->getParent()->setLeftChild(left);
        } else if(p->isRightChild()) {
            p->getParent()->setRightChild(left);
        } else {
            setRoot(left);
        }
        left->setRightChild(p);
    }

    Node* mRoot; // The top node. 0 if empty.
    usz mSize;   // Number of nodes in the tree
};

} // end namespace app

#endif // APP_TMAP_H

