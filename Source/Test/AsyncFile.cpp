#include "AsyncFile.h"
#include "System.h"
#include "Engine.h"

namespace app {

AsyncFile::AsyncFile() :
    mLoop(&Engine::getInstance().getLoop())
    , mPack(1024)
    , mAsyncSize(0) {
    mHandleFile.setClose(EHT_FILE, AsyncFile::funcOnClose, this);
    //35秒内没处理完成时，在AsyncFile::onTimeout关闭mHandleFile
    mHandleFile.setTime(AsyncFile::funcOnTime, 35 * 1000, 20 * 1000, -1);
}


AsyncFile::~AsyncFile() {
}


s32 AsyncFile::open(const String& addr, s32 flag) {
    mAsyncSize = 0;
    if (EE_OK != mHandleFile.open(addr, flag)) {
        return EE_NO_OPEN;
    }
    //mHandleFile.setFileSize(2048);
    RequestFD* it = (RequestFD*)RequestFD::newRequest(4 * System::getDiskSectorSize());
    it->mUser = this;

    if (6 & flag) {
        it->mUsed = snprintf(it->mData, it->mAllocated, "from AsyncFile::write");
        it->mCall = AsyncFile::funcOnWrite;
        if (0 != mHandleFile.write(it)) {
            RequestFD::delRequest(it);
            return EE_ERROR;
        }
        return EE_OK;
    }

    it->mCall = AsyncFile::funcOnRead;
    if (0 != mHandleFile.read(it)) {
        RequestFD::delRequest(it);
        return EE_ERROR;
    }
    return EE_OK;
}


s32 AsyncFile::onTimeout(HandleTime& it) {
    DASSERT(mHandleFile.getGrabCount() > 0);
    return EE_ERROR; //close it
}


void AsyncFile::onClose(Handle* it) {
    DASSERT(&mHandleFile == it && "AsyncFile::onClose handle?");
    delete this;
}


void AsyncFile::onWrite(RequestFD* it) {
    if (0 == it->mError) {
        //test read then
        it->mCall = AsyncFile::funcOnRead;
        it->mUsed = 0;
        if (0 == mHandleFile.read(it)) {
            return;
        }
    } else {
        Logger::log(ELL_ERROR, "AsyncFile::onWrite>>size=%u, ecode=%d", it->mUsed, it->mError);
    }
    RequestFD::delRequest(it);
}


void AsyncFile::onRead(RequestFD* it) {
    if (it->mUsed > 0) {
        mAsyncSize += it->mUsed;
        usz got = mPack.write(it->mData, it->mUsed);
        if (it->mUsed < it->mAllocated) {//if (mAsyncSize >= mHandleFile.getFileSize()) {
            //EOF
            RequestFD::delRequest(it);
            return;
        }
        it->mUsed = 0;
        if (EE_OK != mHandleFile.read(it, mPack.size())) {
            RequestFD::delRequest(it);
            return;
        }
        //printf("AsyncFile::onRead>>%.*s\n", (s32)dat.mLen, dat.mData);
    } else {
        //EOF
        //使用FILE_FLAG_NO_BUFFERING时,读取时所设偏移量正好在文件尾且文件大小为磁盘扇区整数倍时，ecode = 10022
        //Logger::log(ELL_ERROR, "AsyncFile::onRead>>read EOF, ecode=%d", it->mError);
        RequestFD::delRequest(it);
    }
}


} //namespace app
