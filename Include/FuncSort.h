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


#ifndef APP_FUNCSORT_H
#define APP_FUNCSORT_H

#include "Config.h"

namespace app {


template <class T>
inline bool AppCompareLess(const T& left, const T& right) {
    return (left < right);
}

template <class T>
inline bool AppCompareMore(const T& left, const T& right) {
    return (left > right);
}


template <class T>
inline void AppSelectSort(T* iArray, usz iSize, bool (*cmper)(const T&, const T&) = AppCompareLess<T>) {
    if (nullptr == iArray || iSize < 2) {
        return;
    }
    T temp;
    usz minpos;
    const usz max = iSize - 1;
    for (usz i = 0; i < max; i++) {
        minpos = i;
        for (usz j = i + 1; j < iSize; j++) {
            if (cmper(iArray[j], iArray[minpos])) {
                minpos = j;
            }
        }
        if (minpos != i) {
            temp = iArray[i];
            iArray[i] = iArray[minpos];
            iArray[minpos] = temp;
        }
    }
}


// Sinks an element into the heap.
template <class T>
inline void AppHeapSink(T* array, ssz element, ssz max, bool (*cmper)(const T&, const T&) = AppCompareLess<T>) {
    T tmp;
    while ((element << 1) < max) { // there is a left child
        ssz j = (element << 1);
        if (j + 1 < max && cmper(array[j], array[j + 1])) {
            j = j + 1; // take right child
        }
        if (cmper(array[element], array[j])) {
            tmp = array[j]; // swap elements
            array[j] = array[element];
            array[element] = tmp;
            element = j;
        } else {
            return;
        }
    }
}


/**
 * @brief 堆排序
 * 没有额外的内存浪费，算法在最坏情况下执行O(n*log n)
 */
template <class T>
inline void AppHeapSort(T* iArray, ssz iSize, bool (*cmper)(const T&, const T&) = AppCompareLess<T>) {
    if (iSize < 2 || nullptr == iArray) {
        return;
    }
    // for AppHeapSink we pretent this is not c++, where
    // arrays start with index 0. So we decrease the array pointer,
    // the maximum always +2 and the element always +1
    T* const virtualArray = iArray - 1;
    const ssz virtualSize = iSize + 2;
    ssz i;

    // build heap
    for (i = ((iSize - 1) / 2); i >= 0; --i) {
        AppHeapSink(virtualArray, i + 1, virtualSize - 1, cmper);
    }

    // sort array, leave out the last element (0)
    T tmp;
    for (i = iSize - 1; i > 0; --i) {
        tmp = iArray[0];
        iArray[0] = iArray[i];
        iArray[i] = tmp;
        AppHeapSink(virtualArray, 1, i + 1, cmper);
    }
}


/**
 * @brief 递归的快速排序，元素不多时(<=256)退化为选择排序
 */
template <class T>
void AppQuickSort(T* iData, usz iSize, bool (*cmper)(const T&, const T&) = AppCompareLess<T>) {
    if (iSize < 2 || nullptr == iData) {
        return;
    }
    if (iSize <= 256) {
        AppSelectSort(iData, iSize, cmper);
        return;
    }
    usz pos = iSize >> 1;
    usz start = 0;
    usz end = iSize - 1;
    T dig = iData[pos];

    // printf("step>> pos = %d, dig = %d\n",  pos, dig);

    while (start < end) {
        for (; start < pos; ++start) {
            if (cmper(dig, iData[start])) {
                // printf("move [%d] to [%d]\n", start, pos);
                iData[pos] = iData[start];
                pos = start;
                break;
            }
        }
        for (; end > pos; --end) {
            if (cmper(iData[end], dig)) {
                // printf("move [%d] to [%d]\n", end, pos);
                iData[pos] = iData[end];
                pos = end;
                break;
            }
        }
    } // while

    iData[pos] = dig;

    AppQuickSort(iData, pos++, cmper);
    if (pos < iSize) {
        AppQuickSort(iData + pos, iSize - pos, cmper);
    }
}



} // end namespace app

#endif // APP_FUNCSORT_H
