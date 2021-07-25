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


#include "TimerWheel.h"


#ifndef APP_TIMER_MANAGER_LIMIT
#define APP_TIMER_MANAGER_LIMIT	300000		// 300 seconds
#endif


namespace app {
//----------------------------------------------------------------
TimerWheel::STimeNode::STimeNode() :
    mCycleStep(0),
    mMaxRepeat(1),
    mTimeoutStep(0),
    mCallback(0),
    mCallbackData(0) {
    //mLinker.init();
}


TimerWheel::STimeNode::~STimeNode() {
    mLinker.delink();
    mCallback = 0;
    mCallbackData = 0;
    mTimeoutStep = 0;
}


//---------------------------------------------------------------------
TimerWheel::TimerWheel(s64 millisec, s32 interval) :
    mCurrentStep(0),
    mCurrent(millisec),
    mInterval((interval > 0) ? interval : 1) {
    init();
}


TimerWheel::~TimerWheel() {
    clear();
}


void TimerWheel::init() {
    initSlot(mSlot_0, APP_TIME_ROOT_SLOT_SIZE);
    initSlot(mSlot_1, APP_TIME_SLOT_SIZE);
    initSlot(mSlot_2, APP_TIME_SLOT_SIZE);
    initSlot(mSlot_3, APP_TIME_SLOT_SIZE);
    initSlot(mSlot_4, APP_TIME_SLOT_SIZE);
}


void TimerWheel::clear() {
    mSpinlock.lock();
    clearSlot(mSlot_0, APP_TIME_ROOT_SLOT_SIZE);
    clearSlot(mSlot_1, APP_TIME_SLOT_SIZE);
    clearSlot(mSlot_2, APP_TIME_SLOT_SIZE);
    clearSlot(mSlot_3, APP_TIME_SLOT_SIZE);
    clearSlot(mSlot_4, APP_TIME_SLOT_SIZE);
    mSpinlock.unlock();
}


void TimerWheel::initSlot(Node2 all[], u32 size) {
    for(u32 i = 0; i < size; i++) {
        all[i].clear();
    }
}


void TimerWheel::clearSlot(Node2 all[], u32 size) {
    STimeNode* node;
    AppTimeoutCallback fn;
    Node2 queued;
    for(u32 j = 0; j < size; j++) {
        if(all[j].mNext) {
            all[j].splitAndJoin(queued);

            mSpinlock.unlock(); //unlock

            while(queued.mNext) {
                node = DGET_HOLDER(queued.mNext, STimeNode, mLinker);
                if(node->mLinker.mNext) {
                    node->mLinker.delink();
                }
                fn = node->mCallback;
                if(fn) {
                    fn(node->mCallbackData);
                }
            }//while

            mSpinlock.lock(); //lock
        }//if
    }//for
}


void TimerWheel::add(STimeNode& node, u32 period, s32 repeat) {
    mSpinlock.lock();

    if(node.mLinker.mNext) {
        node.mLinker.delink();
    }
    u32 steps = (period + mInterval - 1) / mInterval;
    if(steps >= 0x70000000U) {//21 days max if mInterval=1 millisecond
        steps = 0x70000000U;
    }
    node.mCycleStep = static_cast<s32>(steps);
    node.mTimeoutStep = mCurrentStep + node.mCycleStep;
    node.mMaxRepeat = repeat;
    innerAdd(node);

    mSpinlock.unlock();
}


void TimerWheel::add(STimeNode& node) {
    mSpinlock.lock();

    if(node.mLinker.mNext) {
        node.mLinker.delink();
    }
    innerAdd(node);

    mSpinlock.unlock();
}


bool TimerWheel::remove(STimeNode& node) {
    mSpinlock.lock();

    if(node.mLinker.mNext) {
        node.mLinker.delink();
        mSpinlock.unlock(); //unlock
        return true;
    }

    mSpinlock.unlock(); //unlock
    return false;
}


void TimerWheel::innerAdd(STimeNode& node) {
    s32 expires = node.mTimeoutStep;
    u32 idx = expires - mCurrentStep;
    if(idx < ESLOT0_MAX) {
        mSlot_0[expires & APP_TIME_ROOT_SLOT_MASK].pushBack(node.mLinker);
    } else if(idx < ESLOT1_MAX) {
        mSlot_1[getIndex(expires, 0)].pushBack(node.mLinker);
    } else if(idx < ESLOT2_MAX) {
        mSlot_2[getIndex(expires, 1)].pushBack(node.mLinker);
    } else if(idx < ESLOT3_MAX) {
        mSlot_3[getIndex(expires, 2)].pushBack(node.mLinker);
    } else if(static_cast<s32>(idx) < 0) {
        mSlot_0[mCurrentStep & APP_TIME_ROOT_SLOT_MASK].pushBack(node.mLinker);
    } else {
        mSlot_4[getIndex(expires, 3)].pushBack(node.mLinker);
    }
}


void TimerWheel::innerCascade(Node2& head) {
    Node2 queued;
    //queued.init();
    head.splitAndJoin(queued);
    STimeNode* node;
    while(queued.mNext) {
        node = DGET_HOLDER(queued.mNext, STimeNode, mLinker);
        node->mLinker.delink();
        innerAdd(*node);
    }
}


void TimerWheel::innerUpdate() {
    s32 index = mCurrentStep & APP_TIME_ROOT_SLOT_MASK;
    if(index == 0) {
        s32 i = getIndex(mCurrentStep, 0);
        innerCascade(mSlot_1[i]);
        if(i == 0) {
            i = getIndex(mCurrentStep, 1);
            innerCascade(mSlot_2[i]);
            if(i == 0) {
                i = getIndex(mCurrentStep, 2);
                innerCascade(mSlot_3[i]);
                if(i == 0) {
                    i = getIndex(mCurrentStep, 3);
                    innerCascade(mSlot_4[i]);
                }
            }
        }
    }//if

    mCurrentStep++;

    if(mSlot_0[index].mNext) {
        Node2 queued;
        //queued.init();
        mSlot_0[index].splitAndJoin(queued);
        STimeNode* node;
        AppTimeoutCallback fn;
        s32 repeat;
        mSpinlock.unlock(); //unlock

        while(queued.mNext) {
            node = DGET_HOLDER(queued.mNext, STimeNode, mLinker);
            node->mLinker.delink();
            fn = node->mCallback;
            repeat = node->mMaxRepeat - (node->mMaxRepeat > 0 ? 1 : 0);
            if(fn) {
                fn(node->mCallbackData); //@note: node maybe deleted by this call.
            }
            if(0 != repeat && node->mCycleStep > 0) {
                node->mMaxRepeat = repeat;
                node->mTimeoutStep = mCurrentStep + node->mCycleStep;
                innerAdd(*node);
            }
        }

        mSpinlock.lock(); //lock
    }//if
}


// run timer events
void TimerWheel::update(s64 millisec) {
    s64 limit = mInterval * 64LL;
    s64 diff = millisec - mCurrent;
    if(diff > APP_TIMER_MANAGER_LIMIT + limit) {
        mCurrent = millisec;
    } else if(diff < -APP_TIMER_MANAGER_LIMIT - limit) {
        mCurrent = millisec;
    }
    while(isTimeAfter64(millisec, mCurrent)) {
        mSpinlock.lock();
        innerUpdate();
        mCurrent += mInterval;
        mSpinlock.unlock();
    }
}


}//namespace app
