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


#ifndef APP_TALLOCATOR_H
#define APP_TALLOCATOR_H

#include "Config.h"
#include <new>
#include <utility>  //for std::move

namespace app {

#ifdef DMEM_DEBUG
#undef DMEM_DEBUG
#define DMEM_DEBUG new
#endif

template<typename T>
class TAllocator {
public:
    virtual ~TAllocator() { }

    //allocate memory for an array of objects
    T* allocate(usz cnt) {
        return (T*)innerNew(cnt * sizeof(T));
    }

    //deallocate memory for an array of objects
    void deallocate(T* ptr) {
        innerDelete(ptr);
    }

    //construct an element
    void construct(T* ptr) {
        new ((void*)ptr) T();
    }

    //construct an element
    void construct(T* ptr, const T& e) {
        new ((void*)ptr) T(e);
    }

    //construct an element
    void construct(T* ptr, T&& e) {
        new ((void*)ptr) T(std::move(e));
    }

    //destruct an element
    void destruct(T* ptr) {
        ptr->~T();
    }

protected:
    virtual void* innerNew(usz cnt) {
        return operator new(cnt);
    }

    virtual void innerDelete(void* ptr) {
        operator delete(ptr);
    }
};



template<typename T>
class TAllocatorFast {
public:

    //allocate memory for an array of objects
    T* allocate(usz cnt) {
        return (T*)operator new(cnt * sizeof(T));
    }

    // Deallocate memory for an array of objects
    void deallocate(T* ptr) {
        operator delete(ptr);
    }

    //construct an element
    void construct(T* ptr) {
        new ((void*)ptr) T();
    }

    // Construct an element
    void construct(T* ptr, const T& e) {
        new ((void*)ptr) T(e);
    }

    void construct(T* ptr, T&& e) {
        new ((void*)ptr) T(std::move(e));
    }

    // Destruct an element
    void destruct(T* ptr) {
        ptr->~T();
    }
};



#ifdef DMEM_DEBUG
#undef DMEM_DEBUG
#define DMEM_DEBUG new( _CLIENT_BLOCK, __FILE__, __LINE__)
#endif

//! defines an allocation strategy
enum EAllocStrategy {
    E_STRATEGY_SAFE = 0,
    E_STRATEGY_DOUBLE = 1,
    E_STRATEGY_SQRT = 2
};


} // end namespace app

#endif //APP_TALLOCATOR_H

