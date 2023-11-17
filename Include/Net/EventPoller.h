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


#ifndef APP_EVENTPOLLER_H
#define APP_EVENTPOLLER_H

#include "Socket.h"

#if defined(DOS_WINDOWS)
namespace app {

class EventPoller {
public:
#if defined(DOS_WINDOWS)
    ///SEvent layout must same as OVERLAPPED_ENTRY's layout
    struct SEvent {
#if defined(DOS_64BIT)
        u64 mKey;           ///< PULONG_PTR
        void* mPointer;     ///< LPOVERLAPPED
        u64 mInternal;
#else
        u32 mKey;           ///< PULONG_PTR
        void* mPointer;     ///< LPOVERLAPPED
        u32 mInternal;
#endif
        u32 mBytes;         ///< DWORD, Bytes Transferred
    };
#endif //DOS_WINDOWS

    EventPoller();

    ~EventPoller();

    bool open();

    void close();

    /**
    * @return 1 if success or timeout, else -1
    */
    s32 getEvent(SEvent& iEvent, u32 time);

    /**
    * @return count of launched events if success, else -1
    */
    s32 getEvents(SEvent* iEvent, s32 iSize, u32 iTime);

    bool add(void* fd, void* key);

    bool add(const net::Socket& iSock, void* iKey);

    //bool remove(void* fd, void* overlp);

    static bool getResult(void* fd, void* overlp, u32* bytes, u32 wait);

    /**
    *@brief cancel IO of this thread.
    *@param handle The IO handle.
    */
    static bool cancelIO(void* handle);

    static bool cancelIO(const net::Socket& sock);

    /**
    *@brief cancel IO of all thread.
    *@param handle The IO handle.
    *@param overlp 0 to cancel all IO of handle, else just cancel the IO against to \p overlp.
    */
    static bool cancelIO(void* handle, void* overlp);

    static bool cancelIO(const net::Socket& sock, void* overlp);

    static bool hasOverlappedIoCompleted(void* overlp);

    bool postEvent(SEvent& iEvent);

protected:
    void* mHandle;
};


} //namespace app

#elif defined(DOS_LINUX) || defined(DOS_ANDROID)

#include "Linux/IOURing.h"

namespace app {

class EventPoller {
public:
    ///SEvent layout must same as epoll_event's layout
#if defined(DOS_LINUX)
#pragma pack(4)
#endif
    struct SEvent {
        union UData {
            u32 mData32;
            u64 mData64;
            void* mPointer;
        };
        u32 mEvent;
        UData mData;
        void setMessage(u32 msg) {
            mEvent = msg;
        }
    };
#if defined(DOS_LINUX)
#pragma pack()
#endif

    EventPoller();

    ~EventPoller();

    bool open();

    void close();

    /**
    * @return count of launched events if success, or 0 if timeout, else -1
    */
    s32 getEvent(SEvent& iEvent, u32 iTime);

    /**
    * @return count of launched events if success, or 0 if timeout, else -1
    */
    s32 getEvents(SEvent* iEvent, s32 iSize, u32 iTime);

    //EPOLL_CTL_ADD
    bool add(s32 fd, SEvent& iEvent);
    bool add(const net::Socket& iSock, SEvent& iEvent);

    //EPOLL_CTL_MOD
    bool modify(s32 fd, SEvent& iEvent);
    bool modify(const net::Socket& iSock, SEvent& iEvent);

    //EPOLL_CTL_DEL
    bool remove(s32 fd);
    bool remove(const net::Socket& iSock);

    bool postEvent(SEvent& iEvent);

    IOURing& getIOURing() {
        return mIOUR;
    }

protected:
    s32 mEpollFD;
    IOURing mIOUR;
};

} //namespace app

#endif



#endif //APP_EVENTPOLLER_H
