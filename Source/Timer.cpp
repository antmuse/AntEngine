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


#include "Timer.h"
#include <stdio.h>
#include <time.h>
#include <wchar.h>
#if defined(DOS_WINDOWS)
#include <windows.h>
#elif defined(DOS_LINUX) || defined(DOS_ANDROID)
#include <sys/time.h>
#endif

namespace app {

s64 AppGetTimeZone() {
    long ret = 0;

#if defined(DOS_WINDOWS)
    time_t tms = 0;
    tm gm;
    localtime_s(&gm, &tms);
    _get_timezone(&ret);
#elif defined(DOS_LINUX) || defined(DOS_ANDROID)
    struct timeval tv;
    struct timezone tz;
    gettimeofday(&tv, &tz);
    ret = tz.tz_dsttime * 60;
#endif
    return ret;
}


void AppDateConvert(const struct tm& timeinfo, Timer::SDate& date) {
    date.mHour = (u32)timeinfo.tm_hour;
    date.mMinute = (u32)timeinfo.tm_min;
    date.mSecond = (u32)timeinfo.tm_sec;
    date.mDay = (u32)timeinfo.tm_mday;
    date.mMonth = (u32)timeinfo.tm_mon + 1;
    date.mYear = (u32)timeinfo.tm_year + 1900;
    date.mWeekday = (Timer::EWeekday)timeinfo.tm_wday;
    date.mYearday = (u32)timeinfo.tm_yday + 1;
    date.mIsDST = timeinfo.tm_isdst != 0;
}


void Timer::getDate(Timer::SDate& date) {
    time_t rawtime = time(nullptr);
    struct tm timeinfo;
#if defined(DOS_WINDOWS)
    localtime_s(&timeinfo, &rawtime);
#elif defined(DOS_LINUX) || defined(DOS_ANDROID)
    localtime_r(&rawtime, &timeinfo);
#endif
    AppDateConvert(timeinfo, date);
}


void Timer::getDate(s64 rawtime, Timer::SDate& date) {
    struct tm timeinfo;
#if defined(DOS_WINDOWS)
    localtime_s(&timeinfo, &rawtime);
    //gmtime_s(&timeinfo, &rawtime);
#elif defined(DOS_LINUX) || defined(DOS_ANDROID)
    localtime_r((time_t*)& rawtime, &timeinfo);
#endif
    AppDateConvert(timeinfo, date);
}


Timer::SDate Timer::getDate() {
    Timer::SDate date = {0};
    getDate(date);
    return date;
}


Timer::SDate Timer::getDate(s64 time) {
    Timer::SDate date = {0};
    getDate(time, date);
    return date;
}


s64 Timer::getRelativeTime() {
#if defined(DOS_WINDOWS)
#if defined(DOS_64BIT)
    return ::GetTickCount64();
#else
    return ::GetTickCount();
#endif
#elif defined(DOS_LINUX) || defined(DOS_ANDROID)
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (ts.tv_sec * 1000LL + ts.tv_nsec / 1000000LL);
#endif
}

s64 Timer::getTimestamp() {
    return time(nullptr);
}

s64 Timer::getTimeZone() {
    static s64 ret = AppGetTimeZone();
    return ret;
}

s64 Timer::cutDays(s64 timestamp) {
    const s64 daysec = 24 * 60 * 60;
    return (timestamp - getTimeZone()) / daysec * daysec + getTimeZone();
}


#if defined(DOS_WINDOWS)
static BOOL HighPerformanceTimerSupport = FALSE;
static BOOL MultiCore = FALSE;
LARGE_INTEGER AppGetSysFrequency() {
    LARGE_INTEGER ret;
    HighPerformanceTimerSupport = QueryPerformanceFrequency(&ret);
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    MultiCore = (sysinfo.dwNumberOfProcessors > 1);
    return ret;
}
s64 AppGetHardwareTime() {
    static LARGE_INTEGER HighPerformanceFreq = AppGetSysFrequency();
    if(HighPerformanceTimerSupport) {
        // Avoid potential timing inaccuracies across multiple cores by
        // temporarily setting the affinity of this process to one core.
        DWORD_PTR affinityMask = 0;
        if(MultiCore) {
            affinityMask = SetThreadAffinityMask(GetCurrentThread(), 1);
        }
        LARGE_INTEGER nTime;
        BOOL queriedOK = QueryPerformanceCounter(&nTime);
        if(MultiCore) {// Restore the true affinity.
            SetThreadAffinityMask(GetCurrentThread(), affinityMask);
        }
        if(queriedOK) {
            return ((nTime.QuadPart) * 1000LL / HighPerformanceFreq.QuadPart);
        }
    }
    return time(nullptr);
}
#endif


s64 Timer::getTime() {
#if defined(DOS_WINDOWS)
    struct tm tms;
    SYSTEMTIME wtm;
    GetLocalTime(&wtm);
    tms.tm_year = wtm.wYear - 1900;
    tms.tm_mon = wtm.wMonth - 1;
    tms.tm_mday = wtm.wDay;
    tms.tm_hour = wtm.wHour;
    tms.tm_min = wtm.wMinute;
    tms.tm_sec = wtm.wSecond;
    tms.tm_isdst = -1;
    return mktime(&tms) * 1000LL + wtm.wMilliseconds;
#elif defined(DOS_LINUX) || defined(DOS_ANDROID)
    timeval tv;
    gettimeofday(&tv, 0);
    return ((tv.tv_sec * 1000LL) + tv.tv_usec / 1000LL);
#endif
}

s64 Timer::getRealTime() {
#if defined(DOS_WINDOWS)
    static const u64 FILETIME_to_timval_skew = 116444736000000000ULL;
    FILETIME tfile;
    //这个函数获取到的是从1601年1月1日到目前经过的纳秒
    ::GetSystemTimeAsFileTime(&tfile);

    ULARGE_INTEGER _100ns;
    _100ns.LowPart = tfile.dwLowDateTime;
    _100ns.HighPart = tfile.dwHighDateTime;
    _100ns.QuadPart -= FILETIME_to_timval_skew;

    //timeval timenow;
    //// Convert 100ns units to seconds;
    //timenow.tv_sec = ( long) (_100ns.QuadPart / (10000 * 1000));
    //// Convert remainder to microseconds;
    //timenow.tv_usec = ( long) ((_100ns.QuadPart % (10000 * 1000)) / 10);
    //return (timenow.tv_sec * 1000000LL + timenow.tv_usec);

    return (_100ns.QuadPart / 10);

#elif defined(DOS_LINUX) || defined(DOS_ANDROID)
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (ts.tv_sec * 1000000LL + ts.tv_nsec / 1000LL);
#endif
}

u64 Timer::getTimeStr(s64 iTime, s8* cache, usz max, const s8* format) {
    struct tm timeinfo;
#if defined(DOS_WINDOWS)
    localtime_s(&timeinfo, &iTime);
    //gmtime_s(&timeinfo, &rawtime);
#elif defined(DOS_LINUX) || defined(DOS_ANDROID)
    localtime_r((time_t*)& iTime, &timeinfo);
#endif
    return strftime(cache, max, format, &timeinfo);
}

u64 Timer::getTimeStr(s8* cache, usz max, const s8* format) {
    s64 iTime = ::time(nullptr);
    struct tm timeinfo;
#if defined(DOS_WINDOWS)
    localtime_s(&timeinfo, &iTime);
    //gmtime_s(&timeinfo, &rawtime);
#elif defined(DOS_LINUX) || defined(DOS_ANDROID)
    localtime_r((time_t*)& iTime, &timeinfo);
#endif
    return strftime(cache, max, format, &timeinfo);
}


u64 Timer::getTimeStr(s64 iTime, wchar_t* cache, usz max, const wchar_t* format) {
    struct tm timeinfo;
#if defined(DOS_WINDOWS)
    localtime_s(&timeinfo, &iTime);
    //gmtime_s(&timeinfo, &rawtime);
#elif defined(DOS_LINUX) || defined(DOS_ANDROID)
    localtime_r((time_t*)& iTime, &timeinfo);
#endif
    return wcsftime(cache, max, format, &timeinfo);
}

u64 Timer::getTimeStr(wchar_t* cache, usz max, const wchar_t* format) {
    s64 iTime = ::time(nullptr);
    struct tm timeinfo;
#if defined(DOS_WINDOWS)
    localtime_s(&timeinfo, &iTime);
    //gmtime_s(&timeinfo, &rawtime);
#elif defined(DOS_LINUX) || defined(DOS_ANDROID)
    localtime_r((time_t*)& iTime, &timeinfo);
#endif
    return wcsftime(cache, max, format, &timeinfo);
}


bool Timer::isLeapYear(u32 iYear) {
    return ((0 == iYear % 4 && 0 != iYear % 100) || 0 == iYear % 400);
}


u32 Timer::getMonthMaxDay(u32 iYear, u32 iMonth) {
    switch(iMonth) {
    case 2:
        return Timer::isLeapYear(iYear) ? 29 : 28;
    case 4:
    case 6:
    case 9:
    case 11:
        return 30;
    default:
        break;
    }
    return 31;
}


} // end namespace app
