#include "Futex.h"
#include "System.h"

#if defined(DOS_WINDOWS)
#include <stdlib.h>
#include <stdio.h>
#include <windows.h>
#include <synchapi.h>

namespace app {

//@note Futex在windows中不能跨进程使用
Futex::Futex() : mValue(0) {
}

Futex::~Futex() {
}

void Futex::lock() {
    s32 val = 0;
    while (!mValue.compare_exchange_strong(val, 1)) {
        val = 1;
        BOOL ret = WaitOnAddress(&mValue, &val, sizeof(mValue), INFINITE);
        if (FALSE == ret) {
            printf("Futex: lock err=%d, errno=%d\n", ret, System::getError());
        }
        val = 0;
    }
}


void Futex::unlock() {
    tryUnlock();
}


bool Futex::tryLock() {
    s32 val = 0;
    return mValue.compare_exchange_strong(val, 1); //__sync_bool_compare_and_swap(&mValue, 0, 1);
}

bool Futex::tryUnlock() {
    s32 val = 1;
    if (mValue.compare_exchange_strong(val, 0)) {
        /*这里有一个情况是，将futex值改为0后，还没进行下一步唤醒，此时有新线程过来,
        则新线程会获得futex的所有权*/
        WakeByAddressSingle(&mValue); // WakeByAddressAll(&mValue);
        return true;
    }
    return false;
}


} // namespace app


#endif
