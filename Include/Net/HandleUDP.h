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


#ifndef APP_HANDLEUDP_H
#define	APP_HANDLEUDP_H

#include "Handle.h"
#include "Net/HandleTCP.h"


namespace app {
class RequestUDP;
class RequestAccept;

namespace net {

class HandleUDP : public HandleTCP {
public:
    HandleUDP() {
        mType = EHT_UDP;
    }

    ~HandleUDP() { }

    s32 write(RequestUDP* it);

    s32 writeTo(RequestUDP* it);

    s32 read(RequestUDP* it);

    s32 readFrom(RequestUDP* it);

    s32 close();

    /**
    * @brief open UDP handle, must provide the remote address if connect flag on.
    * @param it read buffer
    * @param remote remote ip:port
    * @param local local ip:port, default is null
    * @param flag [1=reuse ip:port, 2=connect it]
    * @return 0 if success, else failed.
    */
    s32 open(RequestUDP* it, const s8* remote, const s8* local = nullptr, s32 flag = 0);
    
    /**
    * @brief open UDP handle, must provide the remote address if connect flag on.
    * @param it read buffer
    * @param remote remote address
    * @param local local address, default is null
    * @param flag [1=reuse ip:port, 2=connect it]
    * @return 0 if success, else failed.
    */
    s32 open(RequestUDP* it, const NetAddress* remote, const NetAddress* local = nullptr, s32 flag = 0);

private:
    s32 mFlags;  //1=reused ip:port, 2=connected

    s32 read(RequestUDP* it, s32 flag);
    s32 write(RequestUDP* it, s32 flag);
};


} //namespace net
} //namespace app

#endif //APP_HANDLEUDP_H
