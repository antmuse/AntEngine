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

#include "Handle.h"
#include "Loop.h"

namespace app {

s32 Handle::launchClose() {
    DASSERT(mLoop);
    return mLoop->closeHandle(this);
}

#if defined(DOS_ANDROID) || defined(DOS_LINUX)
#include "Linux/Request.h"


RequestFD* Handle::popReadReq() {
    return AppPopRingQueueHead_1(mReadQueue);
}

RequestFD* Handle::popWriteReq() {
    return AppPopRingQueueHead_1(mWriteQueue);
}

void Handle::addReadPendingTail(RequestFD* it) {
    AppPushRingQueueTail_1(mReadQueue, it);
}

void Handle::addReadPendingHead(RequestFD* it) {
    AppPushRingQueueHead_1(mReadQueue, it);
}

void Handle::addWritePendingTail(RequestFD* it) {
    AppPushRingQueueTail_1(mWriteQueue, it);
}

void Handle::addWritePendingHead(RequestFD* it) {
    AppPushRingQueueHead_1(mWriteQueue, it);
}

#endif

} //namespace app
