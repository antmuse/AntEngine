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


#ifndef APP_NODE_H
#define	APP_NODE_H

#include "Config.h"


namespace app {

/*
* @brief 将结点加入循环单链表尾部
* @param que, 队列尾结点
* @param it, 入队结点
*/
template<class T>
DFINLINE void AppPushRingQueueTail_1(T*& que, T* it) {
    if (que) {
        it->mNext = que->mNext;
        que->mNext = it;
    } else {
        it->mNext = it;
    }
    que = it;
}


/*
* @brief 将结点加入循环单链表头部
* @param que, 队列尾结点
* @param it, 入队结点
*/
template<class T>
DFINLINE void AppPushRingQueueHead_1(T*& que, T* it) {
    if (que) {
        it->mNext = que->mNext;
        que->mNext = it;
    } else {
        it->mNext = it;
        que = it;
    }
}


/*
* @brief 将循环单链表头部出队
* @param que, 队列尾结点
* @return 出队结点或null
*/
template<class T>
DFINLINE T* AppPopRingQueueHead_1(T*& que) {
    if (que) {
        T* head = (T*)(que->mNext);
        if (head == que) {
            que = nullptr;
        } else {
            que->mNext = head->mNext;
        }
        return head;
    }
    return nullptr;
}



class Node2 {
public:
    Node2() :mNext(this), mPrevious(this) {
    }

    ~Node2() {
    }

    bool empty()const {
        return this == mNext;
    }

    void clear() {
        mNext = this;
        mPrevious = this;
    }

    void delink() {
        if (mNext) {
            mNext->mPrevious = mPrevious;
        }
        if (mPrevious) {
            mPrevious->mNext = mNext;
        }
        clear();
    }

    void pushBack(Node2& it) {
        it.mPrevious = this;
        it.mNext = mNext;
        mNext->mPrevious = &it;
        mNext = &it;
    }

    void pushFront(Node2& it) {
        it.mPrevious = mPrevious;
        it.mNext = this;
        mPrevious->mNext = &it;
        mPrevious = &it;
    }

    /**
    *@brief Split all nodes from this queue and join to the new queue.
    *@param newHead The head node of new queue.
    *@note This is the head of current queue, and head node will not join to new queue.
    */
    void splitAndJoin(Node2& newHead) {
        if (mNext) {
            Node2* first = mNext;
            Node2* last = mPrevious;
            Node2* at = newHead.mNext;
            first->mPrevious = &newHead;
            newHead.mNext = first;
            last->mNext = at;
            at->mPrevious = last;
        }
        clear();
    }

    Node2* getPrevious()const {
        return mPrevious;
    }

    Node2* getNext()const {
        return mNext;
    }

    Node2* mNext;
    Node2* mPrevious;
};



class Node3 {
public:
    Node3* mParent;
    Node3* mLeft;
    Node3* mRight;

    Node3() :mParent(nullptr), mLeft(nullptr), mRight(nullptr) {
    }

    Node3(const Node3& val) :mParent(val.mParent), mLeft(val.mLeft), mRight(val.mRight) {
    }

    Node3(const Node3&& val)noexcept :mParent(val.mParent), mLeft(val.mLeft), mRight(val.mRight) {
    }

    ~Node3() {
    }

    Node3& operator=(const Node3& val) {
        mParent = val.mParent;
        mLeft = val.mLeft;
        mRight = val.mRight;
        return *this;
    }

    Node3& operator=(const Node3&& val)noexcept {
        mParent = val.mParent;
        mLeft = val.mLeft;
        mRight = val.mRight;
        return *this;
    }

    void clear() {
        mLeft = nullptr;
        mRight = nullptr;
        mParent = nullptr;
    }
};



/**
* @return true if a<b
*/
typedef bool (*FuncNode3Less)(const Node3* a, const Node3* b);



} //namespace app


#endif //APP_NODE_H
