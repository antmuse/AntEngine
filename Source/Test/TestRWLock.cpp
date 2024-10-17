#include <stdio.h>
#include <thread>
#include <iostream>
#include <mutex>
#if defined(DUSE_CPP17)
#include <shared_mutex>
#endif
#include "Engine.h"
#include "Loop.h"
#include "ReadWriteLock.h"

namespace app {

// g++ main.cpp -lpthread -std=c++17

std::atomic<s64> i(0);
std::atomic<s64> s(0);
s32 count = 1 * 1000 * 1000;

#if defined(DUSE_CPP17)
// c++17
typedef std::shared_lock<std::shared_mutex> read_lock;
typedef std::unique_lock<std::shared_mutex> write_lock;
std::shared_mutex sm;
#endif

ReadWriteLock rwlk;

void fun1() {
    for (int c = 0; c < count; c++) {
        rwlk.lockRead();
        rwlk.unlockRead();
        // write_lock w(sm);
        i++;
    }
}

void fun2() {
    for (int c = 0; c < count; c++) {
        rwlk.lockWrite();
        rwlk.unlockWrite();
        // read_lock r(sm);
        ++s;
    }
}


s32 AppTestReadWriteLock(s32 argc, s8** argv) {
    printf("AppTestReadWriteLock>>start\n");

    auto beforeTime = std::chrono::steady_clock::now();
    std::thread t1(fun1);
    std::thread t2(fun1);
    std::thread t3(fun1);
    std::thread t4(fun1);
    std::thread t5(fun2);
    t1.join();
    t2.join();
    t3.join();
    t4.join();
    t5.join();
    std::cout << " read count: " << i << std::endl;
    std::cout << " write count: " << s << std::endl;
    auto afterTime = std::chrono::steady_clock::now();
    std::cout << "total consumed time:" << std::endl;
    double duration_millsecond = std::chrono::duration<double, std::milli>(afterTime - beforeTime).count();
    std::cout << duration_millsecond << "ms" << std::endl;
    printf("AppTestReadWriteLock>>stop\n");
    return 0;
}

} // namespace app
