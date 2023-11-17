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


#include "Net/EventPoller.h"
#include "System.h"
#include "Net/Socket.h"
#include "Logger.h"

#if defined(DOS_WINDOWS)
#include <Windows.h>
#elif defined(DOS_LINUX) || defined(DOS_ANDROID)
#include <sys/epoll.h>
#include <unistd.h>
#include <errno.h>
#endif


#if defined(DOS_WINDOWS)
namespace app {

EventPoller::EventPoller() :mHandle(INVALID_HANDLE_VALUE) {
    DASSERT(sizeof(SEvent::mKey) == sizeof(ULONG_PTR));
    DASSERT(sizeof(OVERLAPPED_ENTRY) == sizeof(SEvent));
    DASSERT(DOFFSET(SEvent, mBytes) == DOFFSET(OVERLAPPED_ENTRY, dwNumberOfBytesTransferred));
    DASSERT(DOFFSET(SEvent, mInternal) == DOFFSET(OVERLAPPED_ENTRY, Internal));
    DASSERT(DOFFSET(SEvent, mKey) == DOFFSET(OVERLAPPED_ENTRY, lpCompletionKey));
    DASSERT(DOFFSET(SEvent, mPointer) == DOFFSET(OVERLAPPED_ENTRY, lpOverlapped));
}


EventPoller::~EventPoller() {
    close();
}


bool EventPoller::open() {
    mHandle = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);
    return INVALID_HANDLE_VALUE != mHandle;
}

void EventPoller::close() {
    if(INVALID_HANDLE_VALUE != mHandle) {
        ::CloseHandle(mHandle);
        mHandle = INVALID_HANDLE_VALUE;
    }
}


s32 EventPoller::getEvent(SEvent& iEvent, u32 iTime) {
    if(TRUE == ::GetQueuedCompletionStatus(
        mHandle,
        (DWORD*)(&iEvent.mBytes),
        (PULONG_PTR)(&iEvent.mKey),
        (LPOVERLAPPED*)(&iEvent.mPointer),
        iTime
        )) {
        return 1;
    }

    return -1;
}


s32 EventPoller::getEvents(SEvent* iEvent, s32 iSize, u32 iTime) {
    u32 retSize = 0;
    if(TRUE == ::GetQueuedCompletionStatusEx(
        mHandle,
        (OVERLAPPED_ENTRY*)iEvent,
        iSize,
        (DWORD*)& retSize,
        iTime,
        FALSE)) {
        return retSize;
    }

    return -1;
}

bool EventPoller::add(const net::Socket& iSock, void* iKey) {
    return add((void*)iSock.getValue(), iKey);
}


bool EventPoller::add(void* fd, void* key) {
    return (0 != ::CreateIoCompletionPort(fd, mHandle, (ULONG_PTR)key, 0));
}

bool EventPoller::cancelIO(void* handle) {
    if(FALSE == ::CancelIo(handle)) {
        return (ERROR_NOT_FOUND == System::getError() || ERROR_OPERATION_ABORTED == System::getError());
    }
    return true;
}

bool EventPoller::hasOverlappedIoCompleted(void* overlp) {
    return (overlp && HasOverlappedIoCompleted(reinterpret_cast<LPOVERLAPPED>(overlp)));
}

bool EventPoller::cancelIO(void* handle, void* overlp) {
    if(FALSE == ::CancelIoEx(handle, reinterpret_cast<LPOVERLAPPED>(overlp))) {
        return (ERROR_NOT_FOUND == System::getError() || ERROR_OPERATION_ABORTED == System::getError());
    }
    return true;
}

bool EventPoller::cancelIO(const net::Socket& sock) {
    return cancelIO(reinterpret_cast<void*>(sock.getValue()));
}

bool EventPoller::cancelIO(const net::Socket& sock, void* overlp) {
    return cancelIO(reinterpret_cast<void*>(sock.getValue()), overlp);
}

bool EventPoller::getResult(void* fd, void* userPointer, u32* bytes, u32 wait) {
    return TRUE == ::GetOverlappedResult(fd,
        reinterpret_cast<LPOVERLAPPED>(userPointer),
        reinterpret_cast<LPDWORD>(bytes),
        wait);
}


bool EventPoller::postEvent(SEvent& iEvent) {
    return (TRUE == ::PostQueuedCompletionStatus(
        mHandle,
        iEvent.mBytes,
        (ULONG_PTR)iEvent.mKey,
        LPOVERLAPPED(iEvent.mPointer)
        ));
}


} //namespace app


#elif defined(DOS_LINUX) || defined(DOS_ANDROID)


namespace app {

EventPoller::EventPoller() :mEpollFD(-1) {
    DASSERT(sizeof(SEvent) == sizeof(epoll_event));
    DASSERT(DOFFSET(SEvent, mEvent) == DOFFSET(epoll_event, events));
    DASSERT(DOFFSET(SEvent, mData) == DOFFSET(epoll_event, data));
}


EventPoller::~EventPoller() {
    close();
}

bool EventPoller::open() {
    if (sizeof(SEvent) != sizeof(epoll_event)) {
        Logger::log(ELL_ERROR, "EventPoller::open>>failed sizeof %lu-%lu", sizeof(SEvent), sizeof(epoll_event));
        return false;
    }
    if (DOFFSET(SEvent, mEvent) != DOFFSET(epoll_event, events)) {
        Logger::log(
            ELL_ERROR, "EventPoller::open>>failed evt %lu-%lu", DOFFSET(SEvent, mEvent), DOFFSET(epoll_event, events));
        return false;
    }
    if (DOFFSET(SEvent, mData) != DOFFSET(epoll_event, data)) {
        Logger::log(
            ELL_ERROR, "EventPoller::open>>failed dat %lu-%lu", DOFFSET(SEvent, mData), DOFFSET(epoll_event, data));
        return false;
    }

    // mEpollFD = ::epoll_create(0x7ffffff);
    mEpollFD = ::epoll_create1(EPOLL_CLOEXEC); // O_CLOEXEC
    if (-1 == mEpollFD) {
        return false;
    }
    return mIOUR.open(mEpollFD, 256, IORING_SETUP_SQPOLL);
}

void EventPoller::close() {
    if (-1 != mEpollFD) {
        ::close(mEpollFD);
        mIOUR.close();
        mEpollFD = -1;
    }
}

s32 EventPoller::getEvent(SEvent& iEvent, u32 iTime) {
    return ::epoll_wait(mEpollFD, (epoll_event*)(&iEvent), 1, iTime);
}


s32 EventPoller::getEvents(SEvent* iEvent, s32 iSize, u32 iTime) {
    return ::epoll_wait(mEpollFD, (epoll_event*)iEvent, iSize, iTime);
}

bool EventPoller::add(const net::Socket& iSock, SEvent& iEvent) {
    return add(iSock.getValue(), iEvent);
}

bool EventPoller::add(s32 fd, SEvent& iEvent) {
    /*epoll_event ev;
    ev.events = 0;
    ev.data.ptr = iEvent.mData.mPointer;*/
    return 0 == ::epoll_ctl(mEpollFD, EPOLL_CTL_ADD, fd, (epoll_event*)(&iEvent));
}

bool EventPoller::remove(const net::Socket& iSock) {
    return remove(iSock.getValue());
}


bool EventPoller::remove(s32 fd) {
    //epoll_event iEvent;
    //iEvent.events = 0xffffffff;
    //iEvent.data.ptr = 0;
    return 0 == ::epoll_ctl(mEpollFD, EPOLL_CTL_DEL, fd, 0);
}


bool EventPoller::modify(const net::Socket& iSock, SEvent& iEvent) {
    return modify(iSock.getValue(), iEvent);
}

bool EventPoller::modify(s32 fd, SEvent& iEvent) {
    return 0 == ::epoll_ctl(mEpollFD, EPOLL_CTL_MOD, fd, (epoll_event*)(&iEvent));
}


bool EventPoller::postEvent(SEvent& iEvent) {
    return false;
    //return sizeof(iEvent.mEvent) == mSocketPair.getSocketB().sendAll(&iEvent.mEvent, sizeof(iEvent.mEvent));
}


} //namespace app

#endif
