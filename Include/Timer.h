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


#ifndef APP_TIMER_H
#define APP_TIMER_H

#include "Config.h"

namespace app {

/// Interface for date and time
class Timer {
public:
    enum EWeekday {
        EWD_SUNDAY = 0,
        EWD_MONDAY,
        EWD_TUESDAY,
        EWD_WEDNESDAY,
        EWD_THURSDAY,
        EWD_FRIDAY,
        EWD_SATURDAY
    };

    struct SDate {
        // Hour of the day, from 0 to 23
        u32 mHour;
        // Minute of the hour, from 0 to 59
        u32 mMinute;
        // Second of the minute, due to extra seconds from 0 to 61
        u32 mSecond;
        // Year of the gregorian calender
        u32 mYear;
        // Month of the year, from 1 to 12
        u32 mMonth;
        // Day of the month, from 1 to 31
        u32 mDay;
        // Weekday for the current day
        EWeekday mWeekday;
        // Day of the year, from 1 to 366
        u32 mYearday;
        // Whether daylight saving is on
        bool mIsDST;
    };

    static SDate getDate(s64 time);

    static SDate getDate();

    static void getDate(SDate& date);

    static void getDate(s64 time, SDate& date);

    static bool isLeapYear(u32 iYear);

    /**
    * @return time in seconds
    */
    static s64 cutDays(s64 timestamp);

    /**
    * @return time zone in seconds
    */
    static s64 getTimeZone();

    /**
    * @return time in seconds
    */
    static s64 getTimestamp();

    /**
    * @return time in milliseconds
    */
    static s64 getTime();

    /**
    * @return time in microseconds
    */
    static s64 getRealTime();

    /**
    * @return time in milliseconds
    */
    static s64 getRelativeTime();

    static u32 getMonthMaxDay(u32 iYear, u32 iMonth);

    /**
    *@brief Get current time as string.
    *@param iTime timestamp in seconds.
    *@param format format str, eg:
    *   "%a, %d %b %Y %H:%M:%S GMT", //Sat, 11 Mar 2017 21:49:51 GMT
    *   "%Y-%m-%d %H:%M:%S"          //2017-07-09 21:49:51

    *@param cache The cache to write into.
    *@param cacheSize The max cache size, should not be less than 20.
    *@return len of output str.
    */
    static u64 getTimeStr(s64 iTime, s8* cache, usz cacheSize, const s8* format = "%Y-%m-%d %H:%M:%S");
    static u64 getTimeStr(s64 iTime, wchar_t* cache, usz cacheSize, const wchar_t* format = L"%Y-%m-%d %H:%M:%S");

    static u64 getTimeStr(s8* cache, usz cacheSize, const s8* format = "%Y-%m-%d %H:%M:%S");
    static u64 getTimeStr(wchar_t* cache, usz cacheSize, const wchar_t* format = L"%Y-%m-%d %H:%M:%S");
};

} // end namespace app

#endif //APP_TIMER_H
