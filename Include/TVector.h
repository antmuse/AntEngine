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

    TVector()
        : mData(nullptr)
        , mAllocated(0)
        , mUsed(0)
        , mStrategy(E_STRATEGY_DOUBLE)
        , mSorted(true) {
    }


    /**
     * @param start_count 预留容量 */
    TVector(usz start_count)
        : mData(nullptr)
        , mAllocated(0)
        , mUsed(0)
        , mStrategy(E_STRATEGY_DOUBLE)
        , mSorted(true) {
        reallocate(start_count);
    }


    TVector(const TVector<T, TAlloc>& other)
        : mData(nullptr), mAllocated(0), mUsed(0) {
        *this = other;
    }

    TVector(TVector<T, TAlloc>&& it)noexcept : mData(it.mData)
        , mAllocated(it.mAllocated)
        , mUsed(it.mUsed)
        , mStrategy(it.mStrategy)
        , mSorted(it.mSorted) {
        it.mUsed = 0;
        it.mAllocated = 0;
        it.mData = nullptr;
    }

    ~TVector() {
        clearAll();
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

        // copy old data
        ssz end = mUsed < new_size ? mUsed : new_size;
        for (ssz i = 0; i < end; ++i) {
            mAllocator.construct(&mData[i], std::move(old_data[i]));
        }

        // destruct old mData
        for (usz j = 0; j < mUsed; ++j) {
            mAllocator.destruct(&old_data[j]);
        }
        mAllocator.deallocate(old_data);

        if (mAllocated < mUsed) {
            mUsed = mAllocated;
        }
    }


    /**
     * @brief 设置内存申请策略
     */
    void setAllocStrategy(EAllocStrategy newStrategy) {
        mStrategy = newStrategy;
    }

    void emplaceBack(T& it) {
        emplace(it, mUsed);
    }

    void emplaceFront(T& it) {
        emplace(it, 0);
    }

    void pushBack(const T& it) {
        insert(it, mUsed);
    }

    void pushFront(const T& it) {
        insert(it, 0);
    }

    /**
    * @note \p element maybe one node of this vector.
    */
    void insert(const T& element, usz index = 0) {
        index = index > mUsed ? mUsed : index;

        if (mUsed + 1 > mAllocated) {
            switch (mStrategy) {
            case E_STRATEGY_DOUBLE:
                mAllocated = mUsed + 1 + (mAllocated < 500 ?
                    (mAllocated < 5 ? 5 : mUsed) : mUsed >> 2);
                break;
            default:
            case E_STRATEGY_SAFE:
                mAllocated = mUsed + 1;
                break;
            }

            T* old_data = mData;
            mData = mAllocator.allocate(mAllocated);

            //element可能是当前vector的一员，先拷贝构造之
            mAllocator.construct(&mData[index], element);

            // copy old data
            usz i = 0;
            for (; i < index; ++i) {
                mAllocator.construct(&mData[i], std::move(old_data[i]));
                mAllocator.destruct(&old_data[i]);
            }
            for (i = index + 1; i <= mUsed; ++i) {
                mAllocator.construct(&mData[i], std::move(old_data[i - 1]));
                mAllocator.destruct(&old_data[i - 1]);
            }
            mAllocator.deallocate(old_data);
        } else {
            if (index < mUsed) {
                mAllocator.construct(&mData[mUsed], std::move(mData[mUsed - 1]));
                // move the leftover
                for (usz i = mUsed - 1; i > index; --i) {
                    mAllocator.destruct(&mData[i]);
                    mAllocator.construct(&mData[i], std::move(mData[i - 1]));
                }
                mAllocator.destruct(&mData[index]);
            }
            // insert the new element to the postion
            mAllocator.construct(&mData[index], element);
        }
        mSorted = false;
        ++mUsed;
    }


    /**
    * @note \p element must not be one node of this vector.
    */
    void emplace(T& element, usz index = 0) {
        index = index > mUsed ? mUsed : index;

        if (mUsed + 1 > mAllocated) {
            switch (mStrategy) {
            case E_STRATEGY_DOUBLE:
                mAllocated = mUsed + 1 + (mAllocated < 500 ?
                    (mAllocated < 5 ? 5 : mUsed) : mUsed >> 2);
                break;
            default:
            case E_STRATEGY_SAFE:
                mAllocated = mUsed + 1;
                break;
            }

            T* old_data = mData;
            mData = mAllocator.allocate(mAllocated);

            //element不能是当前vector的一员，移动构造之
            mAllocator.construct(&mData[index], std::move(element));

            // copy old data
            usz i = 0;
            for (; i < index; ++i) {
                mAllocator.construct(&mData[i], std::move(old_data[i]));
                mAllocator.destruct(&old_data[i]);
            }
            for (i = index + 1; i <= mUsed; ++i) {
                mAllocator.construct(&mData[i], std::move(old_data[i - 1]));
                mAllocator.destruct(&old_data[i - 1]);
            }
            mAllocator.deallocate(old_data);
        } else {
            if (index < mUsed) {
                mAllocator.construct(&mData[mUsed], std::move(mData[mUsed - 1]));
                // move the leftover
                for (usz i = mUsed - 1; i > index; --i) {
                    mAllocator.destruct(&mData[i]);
                    mAllocator.construct(&mData[i], std::move(mData[i - 1]));
                }
                mAllocator.destruct(&mData[index]);
            }
            // insert the new element to the postion
            mAllocator.construct(&mData[index], std::move(element));
        }
        mSorted = false;
        ++mUsed;
    }

    /**
    * @brief 析构所有元素
    */
    void clear() {
        for (usz i = 0; i < mUsed; ++i) {
            mAllocator.destruct(&mData[i]);
        }
        mUsed = 0;
        mSorted = true;
    }

    /**
    * @brief 析构所有元素并删除数组空间
    */
    void clearAll() {
        if (mData) {
            clear();
            mAllocator.deallocate(mData);
            mData = nullptr;
            mAllocated = 0;
        }
    }

    /**
    * @brief 设置已用元素数量
    * @param usedNow 已用元素数量*/
    void resize(usz usedNow) {
        mSorted = mSorted != 0 && usedNow <= mUsed;
        if (mAllocated < usedNow) {
            reallocate(usedNow);
        }
        //build
        if (usedNow < mUsed) {
            for (usz i = usedNow; i < mUsed; ++i) {
                mAllocator.destruct(&mData[i]);
            }
        } else {
            for (usz i = mUsed; i < usedNow; ++i) {
                mAllocator.construct(&mData[i]);
            }
        }
        mUsed = usedNow;
    }

    TVector<T, TAlloc>& operator=(const TVector<T, TAlloc>& other) {
        if (this == &other) {
            return *this;
        }
        resize(0);
        if (mAllocated < other.mUsed) {
            mAllocator.deallocate(mData);
            mAllocated = other.mUsed;
            mData = mAllocator.allocate(mAllocated);
        }
        mStrategy = other.mStrategy;
        mUsed = other.mUsed;
        mSorted = other.mSorted;

        for (usz i = 0; i < other.mUsed; ++i) {
            mAllocator.construct(&mData[i], other.mData[i]);
        }
        return *this;
    }


    TVector<T, TAlloc>& operator=(TVector<T, TAlloc>&& other)noexcept {
        if (&other != this) {
            AppSwap(mData, other.mData);
            AppSwap(mAllocated, other.mAllocated);
            AppSwap(mUsed, other.mUsed);
            AppSwap(mAllocator, other.mAllocator);

            // can't use AppSwap with bitfields
            EAllocStrategy estra = mStrategy;
            mStrategy = other.mStrategy;
            other.mStrategy = estra;

            bool tmp = mSorted;
            mSorted = other.mSorted;
            other.mSorted = tmp;
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
        if (index < mUsed) {
            for (++index; index < mUsed; ++index) {
                mAllocator.destruct(&mData[index - 1]);
                mAllocator.construct(&mData[index - 1], std::move(mData[index]));
            }
            mAllocator.destruct(&mData[mUsed - 1]);
            --mUsed;
        }
    }

    /**
    * @brief 快速移除&用末位填补
    * @param index 移除位置
    */
    void quickErase(usz index) {
        if (index < mUsed) {
            mAllocator.destruct(&mData[index]);
            if (index + 1 < mUsed) {
                mAllocator.construct(&mData[index], std::move(mData[mUsed - 1]));
                mAllocator.destruct(&mData[mUsed - 1]);
            }
            --mUsed;
        }
    }

    /**
     * @brief 删除多个元素
     * @param index 删除开始位置
     * @param count 删除元素数量
     */
    void erase(usz index, usz count) {
        if (index >= mUsed || count < 1) {
            return;
        }
        usz last = index + count;
        if (last > mUsed) {
            last = mUsed;
        }
        for (usz i = index; i < last; ++i) {
            mAllocator.destruct(&mData[i]);
        }
        for (; last < mUsed; ++last) {
            mAllocator.construct(&mData[index++], std::move(mData[last]));
            mAllocator.destruct(&mData[last]);
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

        // can't use AppSwap with bitfields
        EAllocStrategy estra = mStrategy;
        mStrategy = other.mStrategy;
        other.mStrategy = estra;

        bool tmp = mSorted;
        mSorted = other.mSorted;
        other.mSorted = tmp;
    }


private:
    T* mData;
    usz mAllocated;
    usz mUsed;
    TAlloc mAllocator;
    EAllocStrategy mStrategy : 4;
    bool mSorted : 1;
};


}

#endif //APP_VECTOR_H