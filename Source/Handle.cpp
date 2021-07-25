#include "Handle.h"
#if defined(DOS_ANDROID) || defined(DOS_LINUX)
#include "Linux/Request.h"

namespace app {

Request* Handle::popReadReq() {
    Request* head = nullptr;
    if (mReadQueue) {
        head = mReadQueue->mNext;
        if (head == mReadQueue) {
            mReadQueue = nullptr;
        } else {
            mReadQueue->mNext = head->mNext;
        }
        head->mNext = nullptr;
    }
    return head;
}

Request* Handle::popWriteReq() {
    Request* head = nullptr;
    if (mWriteQueue) {
        head = mWriteQueue->mNext;
        if (head == mWriteQueue) {
            mWriteQueue = nullptr;
        } else {
            mWriteQueue->mNext = head->mNext;
        }
        head->mNext = nullptr;
    }
    return head;
}

void Handle::addReadPendingTail(Request* it) {
    if (mReadQueue) {
        it->mNext = mReadQueue->mNext;
        mReadQueue->mNext = it;
    } else {
        it->mNext = it;
    }
    mReadQueue = it;
}

void Handle::addReadPendingHead(Request* it) {
    if (mReadQueue) {
        it->mNext = mReadQueue->mNext;
        mReadQueue->mNext = it;
    } else {
        it->mNext = it;
        mReadQueue = it;
    }
}

void Handle::addWritePendingTail(Request* it) {
    if (mWriteQueue) {
        it->mNext = mWriteQueue->mNext;
        mWriteQueue->mNext = it;
    } else {
        it->mNext = it;
    }
    mWriteQueue = it;
}


void Handle::addWritePendingHead(Request* it) {
    if (mWriteQueue) {
        it->mNext = mWriteQueue->mNext;
        mWriteQueue->mNext = it;
    } else {
        it->mNext = it;
        mWriteQueue = it;
    }
}

} //namespace app


#endif
