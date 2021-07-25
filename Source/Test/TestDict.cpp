#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "Timer.h"
#include "HashDict.h"
#include "HashFunctions.h"

namespace app {



class CTestKey {
public:
    CTestKey() : mSize(0) {
        mBuf[0] = 0;
    }
    CTestKey(const CTestKey& it) {
        mSize = it.mSize;
        memcpy(mBuf, it.mBuf, 1 + mSize);
    }
    CTestKey& operator=(const s32 it) {
        mSize = snprintf(mBuf, sizeof(mBuf), "%d", it);
        return *this;
    }
    CTestKey& operator=(const CTestKey& it) {
        if(this == &it) {
            return *this;
        }
        mSize = it.mSize;
        memcpy(mBuf, it.mBuf, 1 + mSize);
        return *this;
    }
    bool operator==(const CTestKey& it)const {
        if(mSize != it.mSize) {
            return false;
        }
        return memcmp(mBuf, it.mBuf, mSize) == 0;
    }
    u64 getHashID() const {
        return AppHashSIP(mBuf, mSize);
        //return AppHashMurmur64(mBuf, mSize);
    }
private:
    u32 mSize;
    s8 mBuf[16];
};

using CTestValue = CTestKey;

u64 hashCallback(const void* key) {
    return reinterpret_cast<const CTestKey*>(key)->getHashID();
}

bool compareCallback(void* iUserData, const void* key1, const void* key2) {
    return (*reinterpret_cast<const CTestKey*>(key1)) == (*reinterpret_cast<const CTestKey*>(key2));
}

void freeCallback(void* iUserData, void* val) {
    CTestKey* it = reinterpret_cast<CTestKey*>(val);
    delete it;
}

void* keyValueDump(void* iUserData, const void* kval) {
    CTestKey* ret = new CTestKey(*reinterpret_cast<const CTestKey*>(kval));
    return ret;
}

#define start_benchmark() start = Timer::getTime()
#define end_benchmark(msg) do { \
    elapsed = Timer::getTime()-start; \
    printf(msg ": %ld items in %lld ms\n", count, elapsed); \
} while(0);


void AppTestDict() {
    printf("AppTestDict start\n");
    s8 buf[1024];
    s8 sed[20];
    Timer::getTimeStr(sed, sizeof(sed));
    AppSetHashSeedSIP(sed);
    DictFunctions dictCalls = {
        hashCallback,
        keyValueDump,
        keyValueDump,
        compareCallback,
        freeCallback,
        freeCallback
    };
    s32 j;
    s64 start, elapsed; //times
    HashDict* dict = new HashDict(dictCalls, nullptr);
    const s32 count = 5000000;
    CTestKey kcache;
    CTestValue vcache;
    start_benchmark();
    for(j = 0; j < count; j++) {
        kcache = j;
        s32 retval = dict->add(&kcache, &kcache);
        DASSERT(retval == DICT_OK);
    }
    end_benchmark("Inserting");
    DASSERT((s32)dict->getSize() == count);

    dict->getStats(buf, sizeof(buf));
    printf("1dict status=\n%s\n\n", buf);

    /* Wait for rehashing. */
    while(dict->isRehashing()) {
        dict->rehashWithTimeout(100);
    }

    dict->getStats(buf, sizeof(buf));
    printf("2dict status=\n%s\n\n", buf);

    start_benchmark();
    for(j = 0; j < count; j++) {
        kcache = j;
        DictNode* de = dict->find(&kcache);
        DASSERT(de != nullptr);
    }
    end_benchmark("Linear access of existing elements");

    start_benchmark();
    for(j = 0; j < count; j++) {
        kcache = j;
        DictNode* de = dict->find(&kcache);
        DASSERT(de != nullptr);
    }
    end_benchmark("Linear access of existing elements (2nd round)");

    start_benchmark();
    for(j = 0; j < count; j++) {
        kcache = rand() % count;
        DictNode* de = dict->find(&kcache);
        DASSERT(de != nullptr);
    }
    end_benchmark("Random access of existing elements");

    start_benchmark();
    for(j = 0; j < count; j++) {
        kcache = count + rand() % count;
        DictNode* de = dict->find(&kcache);
        DASSERT(de == nullptr);
    }
    end_benchmark("Accessing missing");

    start_benchmark();
    for(j = 0; j < count; j++) {
        kcache = j;
        s32 retval = dict->remove(&kcache);
        DASSERT(retval == DICT_OK);
        kcache = j + count;
        vcache = j + count;
        retval = dict->add(&kcache, &vcache);
        DASSERT(retval == DICT_OK);
    }
    end_benchmark("Removing and adding");

    dict->getStats(buf, sizeof(buf));
    printf("3dict status=\n%s\n\n", buf);
    delete dict;
}


} //namespace app
