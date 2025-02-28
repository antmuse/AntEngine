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
#define APP_NODE_H

#include "Config.h"


namespace app {

/*
 * @brief 将结点加入循环单链表尾部
 * @param que, 队列尾结点
 * @param it, 入队结点
 */
template <class T>
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
template <class T>
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
template <class T>
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



/**
 * @return true if a<b
 */
class Node2;
typedef bool (*FuncNode2Less)(const Node2* a, const Node2* b);

class Node2 {
public:
    Node2() : mNext(this), mPrevious(this) {
    }

    ~Node2() {
    }

    Node2* getPrevious() const {
        return mPrevious;
    }

    Node2* getNext() const {
        return mNext;
    }

    bool empty() const {
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

    /**
     * @brief merge an other cycle queue at this tail(front)
     * @param head head of other cycle queue.
     */
    Node2& operator+=(Node2& head) {
        // if(&head != this)
        Node2* my_tail = mPrevious;
        Node2* new_tail = head.mPrevious;
        head.mPrevious = my_tail;
        my_tail->mNext = &head;
        mPrevious = new_tail;
        new_tail->mNext = this;
        return *this;
    }

    /**
     * @return the Head of queue
     */
    Node2* sort(FuncNode2Less cmp) {
        return sort(this, this, cmp);
    }

    /**
     * @brief sort parts of queue, the original queue should fixed by user.
     * @param head the head of queue.
     * @param tail the node of tail's next
     * @return head of a cycle queue had been sorted.
     */
    static Node2* sort(Node2* head, const Node2* const tail, FuncNode2Less cmp) {
        if (!head || head->mNext == tail) {
            return head;
        }
        const int maxpos = (sizeof(size_t) << 3) - 1;
        int idx;
        Node2* buf[maxpos + 1] = {nullptr};
        Node2* curr;
        do {
            curr = head;
            head = head->mNext;
            curr->clear(); //@note: do not use delink() here, because we must not changed tail here.
            idx = 0;
            if (nullptr == buf[idx]) {
                buf[idx] = curr;
            } else {
                buf[idx] = mergeSorted(buf[idx], curr, cmp);
                do {
                    if (nullptr == buf[idx + 1]) {
                        buf[idx + 1] = buf[idx];
                        buf[idx] = nullptr;
                        break;
                    }
                    buf[idx + 1] = mergeSorted(buf[idx], buf[idx + 1], cmp);
                    buf[idx++] = nullptr;
                } while (idx < maxpos);
            }
        } while (head != tail);

        int pos;
        for (idx = 0; idx < maxpos;) {
            if (buf[idx]) {
                for (pos = idx + 1; pos <= maxpos; ++pos) {
                    if (buf[pos]) {
                        buf[pos] = mergeSorted(buf[idx], buf[pos], cmp);
                        idx = pos;
                        pos = 0;
                        break;
                    }
                }
                if (0 != pos) {
                    break;
                }
            } else {
                ++idx;
            }
        }
        return buf[idx];
    }


    /**
     * @brief merge 2 sorted cycle queue.
     * @return the head of merged cycle queue, soted also.
     */
    static Node2* mergeSorted(Node2* va, Node2* vb, FuncNode2Less cmp) {
        if (nullptr == va) {
            return vb;
        }
        if (nullptr == vb) {
            return va;
        }
        Node2 head;
        Node2* curr;
        while (va && vb) {
            if (cmp(va, vb)) {
                curr = va;
                va = va->empty() ? nullptr : va->getNext();
            } else {
                curr = vb;
                vb = vb->empty() ? nullptr : vb->getNext();
            }
            curr->delink();
            head.pushFront(*curr);
        }
        curr = va ? va : vb;
        if (curr) {
            head += (*curr);
        }
        curr = head.mNext;
        head.delink();
        return curr;
    }


    Node2* mNext;
    Node2* mPrevious;
};



class Node3 {
public:
    Node3* mParent;
    Node3* mLeft;
    Node3* mRight;

    Node3() : mParent(nullptr), mLeft(nullptr), mRight(nullptr) {
    }

    Node3(const Node3& val) : mParent(val.mParent), mLeft(val.mLeft), mRight(val.mRight) {
    }

    Node3(const Node3&& val) noexcept : mParent(val.mParent), mLeft(val.mLeft), mRight(val.mRight) {
    }

    ~Node3() {
    }

    Node3& operator=(const Node3& val) {
        mParent = val.mParent;
        mLeft = val.mLeft;
        mRight = val.mRight;
        return *this;
    }

    Node3& operator=(const Node3&& val) noexcept {
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



} // namespace app


#endif // APP_NODE_H
