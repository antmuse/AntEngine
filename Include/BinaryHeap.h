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


#ifndef APP_BINARYHEAP_H
#define	APP_BINARYHEAP_H


#include "Node.h"


namespace app {


/**
* @brief 二叉堆
* 此堆不管理任何内存
*/
class BinaryHeap {
public:
    BinaryHeap() = delete;
    BinaryHeap(const BinaryHeap&) = delete;
    BinaryHeap(const BinaryHeap&&) = delete;
    BinaryHeap& operator=(const BinaryHeap&) = delete;
    BinaryHeap& operator=(const BinaryHeap&&) = delete;

    /**
    * @param fn 结点大小比较函数，其返回值决定了此堆为大堆还是小堆
    */
    BinaryHeap(FuncNode3Less fn) :
        mTree(nullptr),
        mSize(0),
        mCall(fn) {
        DASSERT(fn);
    }

    ~BinaryHeap() {
        DASSERT(0 == mSize);
        DASSERT(nullptr == mTree);
    }

    u64 getSize()const {
        return mSize;
    }

    Node3* getTop() const {
        return mTree;
    }

    /**
    * @brief remove top node from tree.
    * @return top node, should be deleted.
    */
    Node3* removeTop() {
        Node3* ret = mTree;
        remove(mTree);
        return ret;
    }

    void insert(Node3* newnode) {
        if(!newnode) {
            return;
        }
        newnode->mLeft = nullptr;
        newnode->mRight = nullptr;
        newnode->mParent = nullptr;

        /* Calculate the path from the root to the insertion point.  This is a min
        * heap so we always insert at the left-most free node of the bottom row.
        */
        u64 path = 0;
        u64 n;
        u64 k;
        for(k = 0, n = 1 + mSize; n >= 2; ++k, n >>= 1) {
            path = (path << 1) | (n & 1);
        }

        /* Now traverse the heap using the path we calculated in the previous step. */
        Node3** parent;
        Node3** child;
        parent = child = (Node3 * *)(&mTree);
        while(k > 0) {
            parent = child;
            if(path & 1) {
                child = &(*child)->mRight;
            } else {
                child = &(*child)->mLeft;
            }
            path >>= 1;
            --k;
        }

        /* Insert the new node. */
        newnode->mParent = *parent;
        *child = newnode;
        ++mSize;

        /*
        * Walk up the tree and check at each node if the heap property holds.
        * It's a min heap so parent < child must be true.
        */
        while(newnode->mParent != nullptr && mCall(newnode, newnode->mParent)) {
            swapNode(newnode->mParent, newnode);
        }
    }


    void remove(Node3* node) {
        if(0 == mSize || !node) {
            DASSERT(0);
            return;
        }

        /* Calculate the path from the min (the root) to the max,
        the left-most node of the bottom row. */
        u64 path = 0;
        u64 k;
        u64 n;
        for(k = 0, n = mSize; n >= 2; ++k, n >>= 1) {
            path = (path << 1) | (n & 1);
        }

        /* Now traverse the heap using the path we calculated in the previous step. */
        Node3** max = (Node3 * *)(&mTree);
        while(k > 0) {
            if(path & 1) {
                max = &(*max)->mRight;
            } else {
                max = &(*max)->mLeft;
            }
            path >>= 1;
            --k;
        }

        --mSize;

        //unlink the max node
        Node3* child = *max;
        *max = nullptr;

        if(child == node) {
            /* We're removing either the max or the last node in the tree. */
            if(child == mTree) {
                mTree = nullptr;
            }
            return;
        }

        /* Replace the to be deleted node with the max node. */
        child->mLeft = node->mLeft;
        child->mRight = node->mRight;
        child->mParent = node->mParent;

        if(child->mLeft != nullptr) {
            child->mLeft->mParent = child;
        }

        if(child->mRight != nullptr) {
            child->mRight->mParent = child;
        }

        if(node->mParent == nullptr) {
            mTree = child;
        } else if(node->mParent->mLeft == node) {
            node->mParent->mLeft = child;
        } else {
            node->mParent->mRight = child;
        }

        /* Walk down the subtree and check at each node if the heap property holds.
        * It's a min heap so parent < child must be true.  If the parent is bigger,
        * swap it with the smallest child.  */
        Node3* smallest;
        for(;;) {
            smallest = child;
            if (child->mLeft != nullptr && mCall(child->mLeft, smallest)) {
                smallest = child->mLeft;
            }
            if (child->mRight != nullptr && mCall(child->mRight, smallest)) {
                smallest = child->mRight;
            }
            if(smallest == child) {
                break;
            }
            swapNode(child, smallest);
        }

        /* Walk up the subtree and check that each parent is less than the node
        * this is required, because `max` node is not guaranteed to be the
        * actual maximum in tree */
        while(child->mParent != nullptr && mCall(child, child->mParent)) {
            swapNode(child->mParent, child);
        }
    }


private:
    Node3* mTree; //the top node
    u64 mSize;
    FuncNode3Less mCall;

    void swapNode(Node3* parent, Node3* child) {
        Node3* sibling;
        Node3 tmp = *parent;
        *parent = *child;
        *child = tmp;

        parent->mParent = child;
        if(child->mLeft == child) {
            child->mLeft = parent;
            sibling = child->mRight;
        } else {
            child->mRight = parent;
            sibling = child->mLeft;
        }
        if(sibling != nullptr) {
            sibling->mParent = child;
        }
        if(parent->mLeft != nullptr) {
            parent->mLeft->mParent = parent;
        }
        if(parent->mRight != nullptr) {
            parent->mRight->mParent = parent;
        }
        if(child->mParent == nullptr) {
            mTree = child;
        } else if(child->mParent->mLeft == parent) {
            child->mParent->mLeft = child;
        } else {
            child->mParent->mRight = child;
        }
    }
};


}//namespace app


#endif //APP_BINARYHEAP_H
