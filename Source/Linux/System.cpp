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


#include "System.h"
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
// #include <sys/sysctl.h>  needless
#include <sys/sysinfo.h>
#include <sys/statfs.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include "Logger.h"
#include "Engine.h"

namespace app {

struct EngSignal {
    s32 mSig;
    const s8* mName;
};
EngSignal gSignals[] = {
    {SIGALRM, "SIGALRM"},
    {SIGINT, "SIGINT.ctl-c"},
    {SIGTERM, "SIGTERM.kill-15"},
    {SIGIO, "SIGIO"},
    {SIGCHLD, "SIGCHLD"},
    {SIGSYS, "SIGSYS, SIG_IGN"},
    {SIGPIPE, "SIGPIPE, SIG_IGN"},
    {50, "ENG.kill-50"},   //should in range [SIGRTMIN-SIGRTMAX]
    {51, "ENG.kill-51"},
    {0, NULL}
};

static u32 AppGetDiskSectorSize(const tchar* disk = DSTR("/")) {
    struct statfs64 sfs;
    if (0 == statfs64(disk, &sfs)) { //fstatfs(fd,s);
        return sfs.f_bsize;
    }
    return 0;
}

static u32 AppGetCoreCount() {
    return get_nprocs();
}

static u32 AppGetPageSize() {
    return getpagesize();
}


s32 System::gSignal = 0;

System::System() {
}

System::~System() {
}


static void System_getStatus() {
    s32 status;
    const s8* process;
    pid_t pid;
    s32 err;

    for (;;) {
        pid = waitpid(-1, &status, WNOHANG);
        if (pid == 0) {
            return;
        }

        if (pid == -1) {
            err = errno;
            if (err == EINTR) {
                continue;
            }
            if(ECHILD!=err){
                Logger::log(ELL_INFO, "waitpid() failed, ecode=%d", err);
            }
            return;
        }


        process = "unknown process";

        TVector<Process>& procs = Engine::getInstance().getChilds();
        usz i = 0;
        for (; i < procs.size(); i++) {
            if (procs[i].mID == pid) {
                procs[i].mStatus = EPS_RESPAWN;
                procs[i].mAlive = false;
                procs[i].mSocket.close();
                process = "mychild";
                break;
            }
        }

        if (WTERMSIG(status)) {
#ifdef WCOREDUMP
            Logger::log(ELL_INFO, "%s %d exited on signal %d%s",
                process, pid, WTERMSIG(status), WCOREDUMP(status) ? " (core dumped)" : "");
#else
            Logger::log(ELL_INFO, "%s %d exited on signal %d", process, pid, WTERMSIG(status));
#endif
        } else {
            Logger::log(ELL_INFO, "%s %d exited with code %d", process, pid, WEXITSTATUS(status));
        }

        if (WEXITSTATUS(status) == 2) {
            if (i < procs.size()) {
                procs[i].mStatus = EPS_EXITED;
            }
            Logger::log(
                ELL_INFO, "%s %d exited with fatal code %d and cannot be respawned", process, pid, WEXITSTATUS(status));
        } else {
            Engine::getInstance().postCommand(ECT_RESPAWN);
        }
    }
}


static void AppOnSignal(s32 val, siginfo_t* info, void* pit) {
    if (info) {
        Logger::log(ELL_INFO, "System::onSignal>>recv signal = %d,from=%d,user=%d",
            val, info->si_pid, info->si_uid);
    } else {
        Logger::log(ELL_INFO, "System::onSignal>>recv signal = %d", val);
    }

    /*EngSignal* sig;
    for (sig = gSignals; sig->mName; sig++) {
        if (val == sig->mSig) {
            break;
        }
    }*/

    switch (val) {
    case SIGINT: //ctrl+c
    case SIGTERM: //kill-15
        Engine::getInstance().postCommand(ECT_EXIT);
        break;

    case SIGCHLD: //child exit
        System_getStatus();
        break;

    case 50:
        Engine::getInstance().postCommand(ECT_ACTIVE);
        break;

    case 51:
        printf("cmd=51-----------------\n");
        break;

    default:
        break;
    }
}



s32 System::getPID() {
    return getpid();
}


s32 System::daemon() {
    s32 pid = fork();
    if (pid < 0) {
        Logger::logError("System::daemon>>fork() failed,ecode=%d", getError());
        return EE_ERROR;
    }
    if (pid > 0) {
        Logger::logInfo("System::daemon>>fork() pid=%d", pid);
        Logger::flush();
        exit(0);
        //return EE_ERROR;
    }

    if (setsid() == -1) {
        Logger::logError("System::daemon>>setsid() failed,ecode=%d", getError());
        return EE_ERROR;
    }

    umask(0);

    s32 fd = open("/dev/null", O_RDWR);
    if (fd == -1) {
        Logger::logError("System::daemon>>open(\"/dev/null\") failed,ecode=%d", getError());
        return EE_ERROR;
    }

    if (dup2(fd, STDIN_FILENO) == -1) {
        Logger::logError("System::daemon>>dup2(STDIN) failed,ecode=%d", getError());
        return EE_ERROR;
    }

    if (dup2(fd, STDOUT_FILENO) == -1) {
        Logger::logError("System::daemon>>dup2(STDOUT) failed,ecode=%d", getError());
        return EE_ERROR;
    }

#if 0
    if (dup2(fd, STDERR_FILENO) == -1) {
        Logger::logError("System::daemon>>dup2(STDERR) failed,ecode=%d", getError());
        return EE_ERROR;
    }
#endif

    if (fd > STDERR_FILENO) {
        if (close(fd) == -1) {
            Logger::logError("System::daemon>>close(%d) failed,ecode=%d", fd, getError());
            return EE_ERROR;
        }
    }

    return EE_OK;
}


s32 System::loadNetLib() {
    EngSignal* sig;
    struct sigaction sa;
    for (sig = gSignals; sig->mName; sig++) {
        memset(&sa, 0, sizeof(struct sigaction));
        sa.sa_sigaction = AppOnSignal;
        sa.sa_flags = SA_SIGINFO; //SIG_IGN;
        sigemptyset(&sa.sa_mask);

        if (sigaction(sig->mSig, &sa, NULL) == -1) {
            Logger::logError("sigaction(%d,%s) failed", sig->mSig, sig->mName);
            return EE_ERROR;
        }
    }
    return 0;
}


s32 System::unloadNetLib() {
    return 0;
}

s32 System::getNetError() {
    return errno;
}

s32 System::getError() {
    return errno;
}

void System::setError(s32 it) {
    errno = it;
}


s32 System::getAppError(s32 err) {
    switch (err) {
    case 0:
        DASSERT(0);
        return EE_ERROR;
    case EINTR:
        return EE_INTR;
    case EAGAIN:    //case EWOULDBLOCK:
        return EE_RETRY;
    case EINPROGRESS:
        return EE_POSTED;
    case EMFILE:
        return EE_TOO_MANY_FD;
    case EALREADY:
    default:
        return err;
    }
}

s32 System::getAppError() {
    return getAppError(getError());
}

u32 System::getDiskSectorSize() {
    static u32 ret = AppGetDiskSectorSize();
    return ret;
}

u32 System::getCoreCount() {
    static u32 ret = AppGetCoreCount();
    return ret;
}


u32 System::getPageSize() {
    static u32 ret = AppGetPageSize();
    return ret;
}

s32 System::createPath(const String& it) {
    if (0 == it.getLen()) {
        return EE_ERROR;
    }
    String path(it.getLen());
    const s8* curr = it.c_str();
    const s8* end = curr + it.getLen();
    path += *curr++;
    for (; curr < end; ++curr) {
        if (isPathDiv(*curr)) {
            if (0 != access(path.c_str(), F_OK)) {
                if (0 != mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)) {
                    return System::getAppError();
                }
            }
        }
        path += *curr;
    }
    return EE_OK;
}



s32 System::removeFile(const String& it) {
    return remove(it.c_str());
}


s32 System::isExist(const String& it) {
    //return (0 == access(it.c_str(), F_OK)) ? 0 : -1;
    struct stat statbuf;
    if (0 == stat(it.c_str(), &statbuf)) {
        if (0 != (S_IFDIR & statbuf.st_mode)) {
            return 1;
        }
        return (0 != (S_IFREG & statbuf.st_mode)) ? 0 : -1;
    }
    return -1;
}


void System::getPathNodes(const String& pth, usz pos, TVector<FileInfo>& out) {
    tchar fname[260];
    usz len = AppMin<usz>(sizeof(fname), pth.getLen());
    if (0 == len || len + 3 > 260) {
        return;
    }
    memcpy(fname, pth.c_str(), len);
    if (!isPathDiv(fname[len - 1])) {
        fname[len++] = DSTR('\\');
    }
    fname[len] = 0;

    DIR* dir;
    struct dirent* nd;
    if ((dir = opendir(fname)) == nullptr) {
        //("Open dir error...");
        return;
    }

    FileInfo itm;
    const s8* prefix = pth.c_str() + pos;
    if (pos < pth.getLen()) {
        pos = pth.getLen() - pos;
    } else {
        prefix = pth.c_str();
        pos = 0;
    }
    memcpy(itm.mFileName, prefix, pos);
    usz addlen = sizeof(itm.mFileName) - pos;
    while ((nd = readdir(dir)) != nullptr) {
        if ('.' == nd->d_name[0]) {
            if (0 == nd->d_name[1] || ('.' == nd->d_name[1] || 0 == nd->d_name[2])) {
                continue;
            }
        }
        len = strlen(nd->d_name);
        if (len + 2 > addlen) { //too long
            continue;
        }
        memcpy(itm.mFileName + pos, nd->d_name, len + 1);
        len += pos;

        itm.mSize = 0;
        itm.mLastSaveTime = 0;

        if (nd->d_type == 8) {    //file
            itm.mFlag = 0;
            out.pushBack(itm);
        } else if (nd->d_type == 4) {    //dir
            itm.mFlag = 1;
            itm.mFileName[len++] = '/';
            itm.mFileName[len] = '\0';
            out.pushBack(itm);
        } else if (nd->d_type == 10) {    //link file

        }
    }
    closedir(dir);
}

void System::waitProcess(void* handle) {
    
}

s32 System::createProcess(usz socket, void*& handle) {
    handle = nullptr;

    s32 pid = fork();
    if (pid < 0) {
        Logger::logError("System::createProcess>>fork() failed,ecode=%d", getError());
        return pid;
    }
    if (pid > 0) {
        Logger::logInfo("System::createProcess>>fork() pid=%d", pid);
        Logger::flush();
        handle = (void*)pid;
        //exit(0);
        return pid;
    }

    if (setsid() == -1) {
        Logger::logError("System::createProcess>>setsid() failed,ecode=%d", getError());
    }
    return pid;
}

String System::getWorkingPath() {
    usz pathSize = 128;
    String wkpath(pathSize);

#if defined(DWCHAR_SYS)
    TString<tchar> tmpPath(pathSize);
    while (pathSize < (1 << 16) && NULL == wgetcwd((tchar*)tmpPath.c_str(), pathSize)) {
        pathSize <<= 1;
        tmpPath.reserve(pathSize);
    }
    wkpath = tmpPath.c_str();
    wkpath.reserve(4 * DSLEN(tmpPath.c_str()));
    usz len = AppWcharToUTF8((tchar*)tmpPath.c_str(), (s8*)wkpath.c_str(), wkpath.getAllocated());
    wkpath.setLen(len);
#else
    while (pathSize < (1 << 16) && NULL == getcwd((s8*)wkpath.c_str(), pathSize)) {
        pathSize <<= 1;
        wkpath.reserve(pathSize);
    }
    wkpath.setLen(DSLEN(wkpath.c_str()));
#endif

    // wkpath.replace('\\', '/');
    if ('/' != wkpath.lastChar()) {
        wkpath += '/';
    }
    return wkpath;
}

} //namespace app

