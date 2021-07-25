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


#ifndef APP_VECTOR_H
#define	APP_VECTOR_H


#include "Config.h"
#include "FuncSort.h"
#include "TAllocator.h"

namespace app {


/**
 * @brief Self reallocating template TVector (like stl vector) with additional features.
 * Some features are: Heap sorting, binary search methods, easier debugging.
 */
template <class T, typename TAlloc = TAllocator<T> >
class TVector {
public:

    TVector() : mData(nullptr), mAllocated(0), mUsed(0),
        mStrategy(E_STRATEGY_DOUBLE), mNeedDelete(true), mSorted(true) {
    }


    /**
     * @param start_count 预留容量 */
    TVector(usz start_count) : mData(nullptr), mAllocated(0), mUsed(0),
        mStrategy(E_STRATEGY_DOUBLE), mNeedDelete(true), mSorted(true) {
        reallocate(start_count);
    }


    TVector(const TVector<T, TAlloc>& other) : mData(nullptr), mAllocated(0), mUsed(0) {
        *this = other;
    }

    TVector(TVector<T, TAlloc>&& it)noexcept : mData(it.mData), mAllocated(it.mAllocated),
        mUsed(it.mUsed), mStrategy(it.mStrategy),
        mNeedDelete(it.mNeedDelete), mSorted(it.mSorted) {
        it.mNeedDelete = false;
        it.mUsed = 0;
        it.mAllocated = 0;
        it.mData = nullptr;
    }

    /**
     * @brief Free allocated memory if setFreeWhenDestroyed was not set to
              false by the user before. */
    ~TVector() {
        clear();
    }


    void reallocate(usz new_size, bool canShrink = true) {
        if (mAllocated == new_size) {
            return;
        }
        if (!canShrink && (new_size < mAllocated)) {
            return;
        }

        T* old_data = mData;
        mData = mAllocator.allocate(new_size); //new T[new_size];
        mAllocated = new_size;

        // copy old mData
        ssz end = mUsed < new_size ? mUsed : new_size;
        for (ssz i = 0; i < end; ++i) {
            // mData[i] = old_data[i];
            mAllocator.construct(&mData[i], old_data[i]);
        }

        // destruct old mData
        if (mNeedDelete) {
            for (usz j = 0; j < mUsed; ++j) {
                mAllocator.destruct(&old_data[j]);
            }
            mAllocator.deallocate(old_data);
        }
        if (mAllocated < mUsed) {
            mUsed = mAllocated;
        }
        mNeedDelete = true;
    }


    /**
     * @brief 设置内存申请策略
     */
    void setAllocStrategy(EAllocStrategy newStrategy) {
        mStrategy = newStrategy;
    }


    void pushBack(const T& element) {
        insert(element, mUsed);
    }


    void pushFront(const T& element) {
        insert(element);
    }


    void insert(const T& element, usz index = 0) {
        DASSERT(index <= mUsed);

        if (mUsed + 1 > mAllocated) {
            // this doesn't work if the element is in the same
            // TVector. So we'll copy the element first to be sure
            // we'll get no mData corruption
            const T e(element);

            // increase mData block
            usz newAlloc;
            switch (mStrategy) {
            case E_STRATEGY_DOUBLE:
                newAlloc = mUsed + 1 + (mAllocated < 500 ?
                    (mAllocated < 5 ? 5 : mUsed) : mUsed >> 2);
                break;
            default:
            case E_STRATEGY_SAFE:
                newAlloc = mUsed + 1;
                break;
            }
            reallocate(newAlloc);

            // move TVector content and construct new element
            // first move end one up
            for (usz i = mUsed; i > index; --i) {
                if (i < mUsed) {
                    mAllocator.destruct(&mData[i]);
                }
                mAllocator.construct(&mData[i], mData[i - 1]); // mData[i] = mData[i-1];
            }
            // then add new element
            if (mUsed > index) {
                mAllocator.destruct(&mData[index]);
            }
            mAllocator.construct(&mData[index], e); // mData[index] = e;
        } else {
            // element inserted not at end
            if (mUsed > index) {
                // create one new element at the end
                mAllocator.construct(&mData[mUsed], mData[mUsed - 1]);

                // move the rest of the TVector content
                for (usz i = mUsed - 1; i > index; --i) {
                    mData[i] = mData[i - 1];
                }
                // insert the new element
                mData[index] = element;
            } else {
                // insert the new element to the end
                mAllocator.construct(&mData[index], element);
            }
        }
        // set to false as we don't know if we have the comparison operators
        mSorted = false;
        ++mUsed;
    }


    /**
    * @brief 如果mNeedDelete，则析构所有元素并清理所有空间
    */
    void clear() {
        if (mNeedDelete) {
            for (usz i = 0; i < mUsed; ++i) {
                mAllocator.destruct(&mData[i]);
            }
            mAllocator.deallocate(mData); // delete[] mData;
        }
        mData = nullptr;
        mUsed = 0;
        mAllocated = 0;
        mSorted = true;
    }

    /**
    * @param newPointer 新空间，注：不会被析构并删除
    * @param maxsz 新空间大小
    * @param size 新空间中元素数量
    */
    void setPointer(T* newPointer, usz maxsz, usz size, bool sorted = false) {
        clear();
        mData = newPointer;
        mAllocated = maxsz;
        mUsed = size;
        mSorted = sorted;
        mNeedDelete = false;
    }


    /**
    * @param it 是否需要析构并删除空间*/
    void setFreeWhenDestroyed(bool it) {
        mNeedDelete = it;
    }


    /**
    * @brief 设置已用元素数量
    * @param build 是否对变动元素调用构造函数或析构函数
    * @param usedNow 已用元素数量*/
    void resize(usz usedNow, bool build = true) {
        mSorted = mSorted != 0 && usedNow <= mUsed;
        if (mAllocated < usedNow) {
            reallocate(usedNow);
        }
        if (build) {
            if (usedNow < mUsed) {
                for (usz i = usedNow; i < mUsed; ++i) {
                    mAllocator.destruct(&mData[i]);
                }
            } else {
                for (usz i = mUsed; i < usedNow; ++i) {
                    mAllocator.construct(&mData[i]);
                }
            }
        }
        mUsed = usedNow;
    }

    /**
    * @brief 设置已用元素数量,因不对变动元素调用构造函数或析构函数,
    *        所以复杂元素不可使用此函数,以避免可能的内存泄露。
    * @param usedNow 已用元素数量
    */
    void setUsed(usz usedNow) {
        resize(usedNow, false);
    }

    TVector<T, TAlloc>& operator=(const TVector<T, TAlloc>& other) {
        if (this == &other) {
            return *this;
        }
        mStrategy = other.mStrategy;
        if (mData) {
            clear();
        }
        //if (mAllocated < other.mAllocated)
        if (other.mAllocated == 0) {
            mData = 0;
        } else {
            mData = mAllocator.allocate(other.mAllocated); // new T[other.mAllocated];
        }
        mUsed = other.mUsed;
        mNeedDelete = true;
        mSorted = other.mSorted;
        mAllocated = other.mAllocated;

        for (usz i = 0; i < other.mUsed; ++i) {
            mAllocator.construct(&mData[i], other.mData[i]); // mData[i] = other.mData[i];
        }
        return *this;
    }


    TVector<T, TAlloc>& operator=(TVector<T, TAlloc>&& other)noexcept {
        if (&other != this) {
            AppSwap(mData, other.mData);
            AppSwap(mAllocated, other.mAllocated);
            AppSwap(mUsed, other.mUsed);
            AppSwap(mAllocator, other.mAllocator);

            EAllocStrategy helper_strategy(mStrategy);	// can't use AppSwap with bitfields
            mStrategy = other.mStrategy;
            other.mStrategy = helper_strategy;

            bool helper_free_when_destroyed(mNeedDelete);
            mNeedDelete = other.mNeedDelete;
            other.mNeedDelete = helper_free_when_destroyed;

            bool helper_is_sorted(mSorted);
            mSorted = other.mSorted;
            other.mSorted = helper_is_sorted;
        }
        return *this;
    }


    bool operator==(const TVector<T, TAlloc>& other) const {
        if (mUsed != other.mUsed) {
            return false;
        }
        for (usz i = 0; i < mUsed; ++i) {
            if (mData[i] != other.mData[i]) {
                return false;
            }
        }
        return true;
    }


    bool operator!=(const TVector<T, TAlloc>& other) const {
        return !(*this == other);
    }


    T& operator[](usz index) {
        DASSERT(index < mUsed);
        return mData[index];
    }


    const T& operator[](usz index) const {
        DASSERT(index < mUsed);
        return mData[index];
    }


    T& getLast() {
        DASSERT(mUsed > 0);
        return mData[mUsed - 1];
    }

    const T& getLast() const {
        DASSERT(mUsed > 0);
        return mData[mUsed - 1];
    }

    T* getPointer() {
        return mData;
    }

    const T* getPointer() const {
        return mData;
    }

    usz size() const {
        return mUsed;
    }

    usz capacity() const {
        return mAllocated;
    }

    bool empty() const {
        return 0 == mUsed;
    }


    /**
     * @param dir true则从小到大排序，否则反之
     */
    void heapSort(bool dir = true) {
        if (!mSorted) {
            AppHeapSort(mData, mUsed, dir ? AppCompareLess<T> : AppCompareMore<T>);
        }
        mSorted = true;
    }

    /**
     * @param dir true则从小到大排序，否则反之
     */
    void quickSort(bool dir = true) {
        if (!mSorted) {
            AppQuickSort(mData, mUsed, dir ? AppCompareLess<T> : AppCompareMore<T>);
        }
        mSorted = true;
    }

    /**
     * @brief 二分查找，无序会先引发排序
     * @param element 元素
     * @return 成功返回元素位置，否则返回-1
     */
    ssz binarySearch(const T& element) {
        heapSort();
        return binarySearch(element, 0, mUsed - 1);
    }


    /**
     * @brief 二分查找，无序时退化为线性查找
     * @param element 元素
     * @return 成功返回元素位置，否则返回-1
     */
    ssz binarySearch(const T& element) const {
        if (mSorted) {
            return binarySearch(element, 0, (ssz)mUsed - 1);
        } else {
            return linearSearch(element);
        }
    }


    /**
     * @brief 二分查找
     * @param element 元素
     * @param left 最小查找位置
     * @param right 最大查找位置
     * @return 成功返回元素位置，否则返回-1
     */
    ssz binarySearch(const T& element, ssz left, ssz right) const {
        if (0 == mUsed) {
            return -1;
        }
        ssz m;
        do {
            m = (left + right) >> 1;
            if (element < mData[m]) {
                right = m - 1;
            } else {
                left = m + 1;
            }
        } while ((element < mData[m] || mData[m] < element) && left <= right);
        // this last line equals to:
        // " while((element != TVector[m]) && left<=right);"
        // but we only want to use the '<' operator.
        // the same in next line, it is "(element == TVector[m])"
        if (!(element < mData[m]) && !(mData[m] < element)) {
            return m;
        }
        return -1;
    }


    /**
     * @brief 正序查找
     * @param element 查找元素
     * @return 成功返回元素位置，否则返回-1
     */
    ssz linearSearch(const T& element) const {
        for (usz i = 0; i < mUsed; ++i) {
            if (element == mData[i]) {
                return (ssz)i;
            }
        }
        return -1;
    }


    /**
     * @brief 倒序查找
     * @param element 查找元素
     * @return 成功返回元素位置，否则返回-1
     */
    ssz linearReverseSearch(const T& element) const {
        for (ssz i = (ssz)mUsed - 1; i >= 0; --i) {
            if (mData[i] == element) {
                return i;
            }
        }
        return -1;
    }


    /**
     * @brief 删除元素
     * @param index 删除位置
     */
    void erase(usz index) {
        DASSERT(index < mUsed); // access violation
        for (usz i = index + 1; i < mUsed; ++i) {
            mAllocator.destruct(&mData[i - 1]);
            mAllocator.construct(&mData[i - 1], mData[i]); // mData[i-1] = mData[i];
        }
        mAllocator.destruct(&mData[mUsed - 1]);
        --mUsed;
    }

    /**
    * @brief 快速移除&用末位填补
    * @param index 移除位置
    */
    void quickErase(usz index) {
        DASSERT(index < mUsed);
        if (index + 1 < mUsed) {
            mAllocator.destruct(&mData[index]);
            mAllocator.construct(&mData[index], mData[mUsed - 1]);//mData[index] = mData[mUsed-1];
        }
        mAllocator.destruct(&mData[mUsed - 1]);
        --mUsed;
    }

    /**
     * @brief 删除多个元素
     * @param index 删除开始位置
     * @param count 删除元素数量
     */
    void erase(usz index, ssz count) {
        if (index >= mUsed || count < 1) {
            return;
        }
        if (index + count > mUsed) {
            count = mUsed - index;
        }
        usz i;
        for (i = index; i < index + count; ++i) {
            mAllocator.destruct(&mData[i]);
        }
        for (i = index + count; i < mUsed; ++i) {
            if (i - count >= index + count) {	// not already destructed before loop
                mAllocator.destruct(&mData[i - count]);
            }
            mAllocator.construct(&mData[i - count], mData[i]); // mData[i-count] = mData[i];
            if (i >= mUsed - count) {	// those which are not overwritten
                mAllocator.destruct(&mData[i]);
            }
        }
        mUsed -= count;
    }


    void setSorted(bool val) {
        mSorted = val;
    }


    void swap(TVector<T, TAlloc>& other) {
        AppSwap(mData, other.mData);
        AppSwap(mAllocated, other.mAllocated);
        AppSwap(mUsed, other.mUsed);
        AppSwap(mAllocator, other.mAllocator);

        EAllocStrategy helper_strategy(mStrategy);	// can't use AppSwap with bitfields
        mStrategy = other.mStrategy;
        other.mStrategy = helper_strategy;

        bool helper_free_when_destroyed(mNeedDelete);
        mNeedDelete = other.mNeedDelete;
        other.mNeedDelete = helper_free_when_destroyed;

        bool helper_is_sorted(mSorted);
        mSorted = other.mSorted;
        other.mSorted = helper_is_sorted;
    }


private:
    T* mData;
    usz mAllocated;
    usz mUsed;
    TAlloc mAllocator;
    EAllocStrategy mStrategy : 4;
    bool mNeedDelete : 1;
    bool mSorted : 1;
};


}

#endif //APP_VECTOR_H