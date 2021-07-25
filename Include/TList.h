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
        SKListNode(const T& e) : Next(0), Prev(0), Element(e) { }

        SKListNode* Next;
        SKListNode* Prev;
        T Element;
    };

public:
    class ConstIterator;

    //! List iterator.
    class Iterator {
    public:
        Iterator() : Current(0) { }

        Iterator& operator ++() { Current = Current->Next; return *this; }
        Iterator& operator --() { Current = Current->Prev; return *this; }
        Iterator  operator ++(s32) { Iterator tmp = *this; Current = Current->Next; return tmp; }
        Iterator  operator --(s32) { Iterator tmp = *this; Current = Current->Prev; return tmp; }

        Iterator& operator +=(s32 num) {
            if(num > 0) {
                while(num-- && this->Current != 0) ++(*this);
            } else {
                while(num++ && this->Current != 0) --(*this);
            }
            return *this;
        }

        Iterator  operator + (s32 num) const { Iterator tmp = *this; return tmp += num; }
        Iterator& operator -=(s32 num) { return (*this) += (-num); }
        Iterator  operator - (s32 num) const { return (*this) + (-num); }

        bool operator ==(const Iterator& other) const { return Current == other.Current; }
        bool operator !=(const Iterator& other) const { return Current != other.Current; }
        bool operator ==(const ConstIterator& other) const { return Current == other.Current; }
        bool operator !=(const ConstIterator& other) const { return Current != other.Current; }

        T& operator*() { return Current->Element; }
        T* operator->() { return &Current->Element; }

    private:
        explicit Iterator(SKListNode* begin) : Current(begin) { }

        SKListNode* Current;

        friend class TList<T>;
        friend class ConstIterator;
    };

    //! List iterator for const access.
    class ConstIterator {
    public:

        ConstIterator() : Current(0) { }
        ConstIterator(const Iterator& iter) : Current(iter.Current) { }

        ConstIterator& operator ++() { Current = Current->Next; return *this; }
        ConstIterator& operator --() { Current = Current->Prev; return *this; }
        ConstIterator  operator ++(s32) { ConstIterator tmp = *this; Current = Current->Next; return tmp; }
        ConstIterator  operator --(s32) { ConstIterator tmp = *this; Current = Current->Prev; return tmp; }

        ConstIterator& operator +=(s32 num) {
            if(num > 0) {
                while(num-- && this->Current != 0) ++(*this);
            } else {
                while(num++ && this->Current != 0) --(*this);
            }
            return *this;
        }

        ConstIterator  operator + (s32 num) const { ConstIterator tmp = *this; return tmp += num; }
        ConstIterator& operator -=(s32 num) { return (*this) += (-num); }
        ConstIterator  operator - (s32 num) const { return (*this) + (-num); }

        bool operator ==(const ConstIterator& other) const { return Current == other.Current; }
        bool operator !=(const ConstIterator& other) const { return Current != other.Current; }
        bool operator ==(const Iterator& other) const { return Current == other.Current; }
        bool operator !=(const Iterator& other) const { return Current != other.Current; }

        const T& operator * () { return Current->Element; }
        const T* operator ->() { return &Current->Element; }

        ConstIterator& operator =(const Iterator& iterator) { Current = iterator.Current; return *this; }

    private:
        explicit ConstIterator(SKListNode* begin) : Current(begin) { }

        SKListNode* Current;

        friend class Iterator;
        friend class TList<T>;
    };

    //! Default constructor for empty TList.
    TList()
        : First(0), Last(0), Size(0) {
    }


    //! Copy constructor.
    TList(const TList<T>& other) : First(0), Last(0), Size(0) {
        *this = other;
    }


    //! Destructor
    ~TList() {
        clear();
    }


    //! Assignment operator
    void operator=(const TList<T>& other) {
        if(&other == this) {
            return;
        }

        clear();

        SKListNode* node = other.First;
        while(node) {
            pushBack(node->Element);
            node = node->Next;
        }
    }


    //! Returns amount of elements in TList.
    /** \return Amount of elements in the TList. */
    u32 size() const {
        return Size;
    }
    u32 getSize() const {
        return Size;
    }


    //! Clears the TList, deletes all elements in the TList.
    /** All existing iterators of this TList will be invalid. */
    void clear() {
        while(First) {
            SKListNode* next = First->Next;
            allocator.destruct(First);
            allocator.deallocate(First);
            First = next;
        }

        //First = 0; handled by loop
        Last = 0;
        Size = 0;
    }


    //! Checks for empty TList.
    /** \return True if the TList is empty and false if not. */
    bool empty() const {
        return (First == 0);
    }


    //! Adds an element at the end of the TList.
    /** \param element Element to add to the TList. */
    void pushBack(const T& element) {
        SKListNode* node = allocator.allocate(1);
        allocator.construct(node, element);

        ++Size;

        if(First == 0)
            First = node;

        node->Prev = Last;

        if(Last != 0)
            Last->Next = node;

        Last = node;
    }


    //! Adds an element at the begin of the TList.
    /** \param element: Element to add to the TList. */
    void push_front(const T& element) {
        SKListNode* node = allocator.allocate(1);
        allocator.construct(node, element);

        ++Size;

        if(First == 0) {
            Last = node;
            First = node;
        } else {
            node->Next = First;
            First->Prev = node;
            First = node;
        }
    }


    //! Gets first node.
    /** \return A TList iterator pointing to the beginning of the TList. */
    Iterator begin() {
        return Iterator(First);
    }


    //! Gets first node.
    /** \return A const TList iterator pointing to the beginning of the TList. */
    ConstIterator begin() const {
        return ConstIterator(First);
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
        return Iterator(Last);
    }


    //! Gets last element.
    /** \return Const TList iterator pointing to the last element of the TList. */
    ConstIterator getLast() const {
        return ConstIterator(Last);
    }


    //! Inserts an element after an element.
    /** \param it Iterator pointing to element after which the new element
    should be inserted.
    \param element The new element to be inserted into the TList.
    */
    void insert_after(const Iterator& it, const T& element) {
        SKListNode* node = allocator.allocate(1);
        allocator.construct(node, element);

        node->Next = it.Current->Next;

        if(it.Current->Next)
            it.Current->Next->Prev = node;

        node->Prev = it.Current;
        it.Current->Next = node;
        ++Size;

        if(it.Current == Last)
            Last = node;
    }


    //! Inserts an element before an element.
    /** \param it Iterator pointing to element before which the new element
    should be inserted.
    \param element The new element to be inserted into the TList.
    */
    void insert_before(const Iterator& it, const T& element) {
        SKListNode* node = allocator.allocate(1);
        allocator.construct(node, element);

        node->Prev = it.Current->Prev;

        if(it.Current->Prev)
            it.Current->Prev->Next = node;

        node->Next = it.Current;
        it.Current->Prev = node;
        ++Size;

        if(it.Current == First)
            First = node;
    }


    //! Erases an element.
    /** \param it Iterator pointing to the element which shall be erased.
    \return Iterator pointing to next element. */
    Iterator erase(Iterator& it) {
        // suggest changing this to a const Iterator& and
        // working around line: it.Current = 0 (possibly with a mutable, or just let it be garbage?)

        Iterator returnIterator(it);
        ++returnIterator;

        if(it.Current == First) {
            First = it.Current->Next;
        } else {
            it.Current->Prev->Next = it.Current->Next;
        }

        if(it.Current == Last) {
            Last = it.Current->Prev;
        } else {
            it.Current->Next->Prev = it.Current->Prev;
        }

        allocator.destruct(it.Current);
        allocator.deallocate(it.Current);
        it.Current = 0;
        --Size;

        return returnIterator;
    }

    //! Swap the content of this TList container with the content of another TList
    /** Afterwards this object will contain the content of the other object and the other
    object will contain the content of this object. Iterators will afterwards be valid for
    the swapped object.
    \param other Swap content with this object	*/
    void swap(TList<T>& other) {
        AppSwap(First, other.First);
        AppSwap(Last, other.Last);
        AppSwap(Size, other.Size);
        AppSwap(allocator, other.allocator);	// memory is still released by the same allocator used for allocation
    }


private:

    SKListNode* First;
    SKListNode* Last;
    u32 Size;
    TAllocator<SKListNode> allocator;
};


}// end namespace app

#endif //APP_TLIST_H

