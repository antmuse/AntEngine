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


#ifndef APP_TLIST_H
#define APP_TLIST_H

#include "TAllocator.h"

namespace app {


//! Doubly linked TList template.
template <class T>
class TList {
private:

    //! List element node with pointer to previous and next element in the TList.
    struct SKListNode {
        SKListNode(const T& e) : mNext(0), mPrev(0), mElement(e) {
        }
        SKListNode(T&& e)
            : mNext(0)
            , mPrev(0)
            , mElement(std::move(e)) {
        }
        SKListNode* mNext;
        SKListNode* mPrev;
        T mElement;
    };

public:
    class ConstIterator;

    //! List iterator.
    class Iterator {
    public:
        Iterator() : mCurrent(0) { }

        Iterator& operator ++() { mCurrent = mCurrent->mNext; return *this; }
        Iterator& operator --() { mCurrent = mCurrent->mPrev; return *this; }
        Iterator  operator ++(s32) { Iterator tmp = *this; mCurrent = mCurrent->mNext; return tmp; }
        Iterator  operator --(s32) { Iterator tmp = *this; mCurrent = mCurrent->mPrev; return tmp; }

        Iterator& operator +=(s32 num) {
            if (num > 0) {
                while (num-- && this->mCurrent != 0) ++(*this);
            } else {
                while (num++ && this->mCurrent != 0) --(*this);
            }
            return *this;
        }

        Iterator  operator + (s32 num) const { Iterator tmp = *this; return tmp += num; }
        Iterator& operator -=(s32 num) { return (*this) += (-num); }
        Iterator  operator - (s32 num) const { return (*this) + (-num); }

        bool operator ==(const Iterator& other) const { return mCurrent == other.mCurrent; }
        bool operator !=(const Iterator& other) const { return mCurrent != other.mCurrent; }
        bool operator ==(const ConstIterator& other) const { return mCurrent == other.mCurrent; }
        bool operator !=(const ConstIterator& other) const { return mCurrent != other.mCurrent; }

        T& operator*() { return mCurrent->mElement; }
        T* operator->() { return &mCurrent->mElement; }

    private:
        explicit Iterator(SKListNode* begin) : mCurrent(begin) { }

        SKListNode* mCurrent;

        friend class TList<T>;
        friend class ConstIterator;
    };

    //! List iterator for const access.
    class ConstIterator {
    public:
        ConstIterator() : mCurrent(0) { }
        ConstIterator(const Iterator& iter) : mCurrent(iter.mCurrent) { }

        ConstIterator& operator ++() { mCurrent = mCurrent->mNext; return *this; }
        ConstIterator& operator --() { mCurrent = mCurrent->mPrev; return *this; }
        ConstIterator  operator ++(s32) { ConstIterator tmp = *this; mCurrent = mCurrent->mNext; return tmp; }
        ConstIterator  operator --(s32) { ConstIterator tmp = *this; mCurrent = mCurrent->mPrev; return tmp; }

        ConstIterator& operator +=(s32 num) {
            if (num > 0) {
                while (num-- && this->mCurrent != 0) ++(*this);
            } else {
                while (num++ && this->mCurrent != 0) --(*this);
            }
            return *this;
        }

        ConstIterator  operator + (s32 num) const { ConstIterator tmp = *this; return tmp += num; }
        ConstIterator& operator -=(s32 num) { return (*this) += (-num); }
        ConstIterator  operator - (s32 num) const { return (*this) + (-num); }

        bool operator ==(const ConstIterator& other) const { return mCurrent == other.mCurrent; }
        bool operator !=(const ConstIterator& other) const { return mCurrent != other.mCurrent; }
        bool operator ==(const Iterator& other) const { return mCurrent == other.mCurrent; }
        bool operator !=(const Iterator& other) const { return mCurrent != other.mCurrent; }

        const T& operator * () { return mCurrent->mElement; }
        const T* operator ->() { return &mCurrent->mElement; }

        ConstIterator& operator =(const Iterator& iterator) { mCurrent = iterator.mCurrent; return *this; }

    private:
        explicit ConstIterator(SKListNode* begin) : mCurrent(begin) { }

        SKListNode* mCurrent;

        friend class Iterator;
        friend class TList<T>;
    };

    //! Default constructor for empty TList.
    TList() : mFirst(0), mLast(0), mSize(0) {
    }


    //! Copy constructor.
    TList(const TList<T>& other) : mFirst(0), mLast(0), mSize(0) {
        *this = other;
    }

    TList(TList<T>&& it) : mFirst(it.mFirst), mLast(it.mLast), mSize(it.mSize) {
        it.mFirst = nullptr;
        it.mLast = nullptr;
        it.mSize = 0;
    }

    //! Destructor
    ~TList() {
        clear();
    }


    //! Assignment operator
    TList<T>& operator=(const TList<T>& other) {
        if (&other == this) {
            return *this;
        }

        clear();

        SKListNode* node = other.mFirst;
        while (node) {
            pushBack(node->mElement);
            node = node->mNext;
        }
        return *this;
    }

    TList<T>& operator=(TList<T>&& it) {
        if (&it == this) {
            return *this;
        }
        swap(it);
        return *this;
    }

    //! Returns amount of elements in TList.
    /** \return Amount of elements in the TList. */
    usz size() const {
        return mSize;
    }

    //! Clears the TList, deletes all elements in the TList.
    /** All existing iterators of this TList will be invalid. */
    void clear() {
        while (mFirst) {
            SKListNode* next = mFirst->mNext;
            mAllocator.destruct(mFirst);
            mAllocator.deallocate(mFirst);
            mFirst = next;
        }

        //mFirst = 0; handled by loop
        mLast = 0;
        mSize = 0;
    }


    //! Checks for empty TList.
    /** \return True if the TList is empty and false if not. */
    bool empty() const {
        return (mFirst == 0);
    }


    //! Adds an element at the end of the TList.
    /** \param element mElement to add to the TList. */
    void pushBack(const T& element) {
        SKListNode* node = mAllocator.allocate(1);
        mAllocator.construct(node, element);

        ++mSize;
        if (mFirst == 0) {
            mFirst = node;
        }
        node->mPrev = mLast;
        if (mLast != 0) {
            mLast->mNext = node;
        }
        mLast = node;
    }

    void emplaceBack(T& element) {
        SKListNode* node = mAllocator.allocate(1);
        mAllocator.construct(node, std::move(element));

        ++mSize;
        if (mFirst == 0) {
            mFirst = node;
        }
        node->mPrev = mLast;
        if (mLast != 0) {
            mLast->mNext = node;
        }
        mLast = node;
    }

    //! Adds an element at the begin of the TList.
    /** \param element: mElement to add to the TList. */
    void pushFront(const T& element) {
        SKListNode* node = mAllocator.allocate(1);
        mAllocator.construct(node, element);

        ++mSize;
        if (mFirst == 0) {
            mLast = node;
            mFirst = node;
        } else {
            node->mNext = mFirst;
            mFirst->mPrev = node;
            mFirst = node;
        }
    }

    void emplaceFront(T& element) {
        SKListNode* node = mAllocator.allocate(1);
        mAllocator.construct(node, std::move(element));

        ++mSize;
        if (mFirst == 0) {
            mLast = node;
            mFirst = node;
        } else {
            node->mNext = mFirst;
            mFirst->mPrev = node;
            mFirst = node;
        }
    }

    //! Gets first node.
    /** \return A TList iterator pointing to the beginning of the TList. */
    Iterator begin() {
        return Iterator(mFirst);
    }


    //! Gets first node.
    /** \return A const TList iterator pointing to the beginning of the TList. */
    ConstIterator begin() const {
        return ConstIterator(mFirst);
    }


    //! Gets end node.
    /** \return List iterator pointing to null. */
    Iterator end() {
        return Iterator(0);
    }


    //! Gets end node.
    /** \return Const TList iterator pointing to null. */
    ConstIterator end() const {
        return ConstIterator(0);
    }


    //! Gets last element.
    /** \return List iterator pointing to the last element of the TList. */
    Iterator getLast() {
        return Iterator(mLast);
    }


    //! Gets last element.
    /** \return Const TList iterator pointing to the last element of the TList. */
    ConstIterator getLast() const {
        return ConstIterator(mLast);
    }


    //! Inserts an element after an element.
    /** \param it Iterator pointing to element after which the new element
    should be inserted.
    \param element The new element to be inserted into the TList.
    */
    void insertAfter(const Iterator& it, const T& element) {
        SKListNode* node = mAllocator.allocate(1);
        mAllocator.construct(node, element);

        node->mNext = it.mCurrent->mNext;

        if (it.mCurrent->mNext) {
            it.mCurrent->mNext->mPrev = node;
        }
        node->mPrev = it.mCurrent;
        it.mCurrent->mNext = node;
        ++mSize;
        if (it.mCurrent == mLast) {
            mLast = node;
        }
    }
    void emplaceAfter(const Iterator& it, T& element) {
        SKListNode* node = mAllocator.allocate(1);
        mAllocator.construct(node, std::move(element));

        node->mNext = it.mCurrent->mNext;

        if (it.mCurrent->mNext) {
            it.mCurrent->mNext->mPrev = node;
        }
        node->mPrev = it.mCurrent;
        it.mCurrent->mNext = node;
        ++mSize;
        if (it.mCurrent == mLast) {
            mLast = node;
        }
    }

    //! Inserts an element before an element.
    /** \param it Iterator pointing to element before which the new element
    should be inserted.
    \param element The new element to be inserted into the TList.
    */
    void insertBefore(const Iterator& it, const T& element) {
        SKListNode* node = mAllocator.allocate(1);
        mAllocator.construct(node, element);

        node->mPrev = it.mCurrent->mPrev;
        if (it.mCurrent->mPrev) {
            it.mCurrent->mPrev->mNext = node;
        }
        node->mNext = it.mCurrent;
        it.mCurrent->mPrev = node;
        ++mSize;
        if (it.mCurrent == mFirst) {
            mFirst = node;
        }
    }
    void emplaceBefore(const Iterator& it, T& element) {
        SKListNode* node = mAllocator.allocate(1);
        mAllocator.construct(node, std::move(element));

        node->mPrev = it.mCurrent->mPrev;
        if (it.mCurrent->mPrev) {
            it.mCurrent->mPrev->mNext = node;
        }
        node->mNext = it.mCurrent;
        it.mCurrent->mPrev = node;
        ++mSize;
        if (it.mCurrent == mFirst) {
            mFirst = node;
        }
    }

    //! Erases an element.
    /** \param it Iterator pointing to the element which shall be erased.
    \return Iterator pointing to next element. */
    Iterator erase(Iterator& it) {
        // suggest changing this to a const Iterator& and
        // working around line: it.mCurrent = 0 (possibly with a mutable, or just let it be garbage?)

        Iterator returnIterator(it);
        ++returnIterator;

        if (it.mCurrent == mFirst) {
            mFirst = it.mCurrent->mNext;
        } else {
            it.mCurrent->mPrev->mNext = it.mCurrent->mNext;
        }

        if (it.mCurrent == mLast) {
            mLast = it.mCurrent->mPrev;
        } else {
            it.mCurrent->mNext->mPrev = it.mCurrent->mPrev;
        }

        mAllocator.destruct(it.mCurrent);
        mAllocator.deallocate(it.mCurrent);
        it.mCurrent = 0;
        --mSize;

        return returnIterator;
    }

    //! Swap the content of this TList container with the content of another TList
    /** Afterwards this object will contain the content of the other object and the other
    object will contain the content of this object. Iterators will afterwards be valid for
    the swapped object.
    \param other Swap content with this object	*/
    void swap(TList<T>& other) {
        AppSwap(mFirst, other.mFirst);
        AppSwap(mLast, other.mLast);
        AppSwap(mSize, other.mSize);
        AppSwap(mAllocator, other.mAllocator);
    }


private:

    SKListNode* mFirst;
    SKListNode* mLast;
    usz mSize;
    TAllocator<SKListNode> mAllocator;
};


}// end namespace app

#endif //APP_TLIST_H

