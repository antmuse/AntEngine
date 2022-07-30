#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "HashDict.h"
#include "Timer.h"
#include "HashFunctions.h"

namespace app {

/* Using enableResize() / disableResize() we make possible to
* enable/disable resizing of the hash table as needed. This is very important
* for Redis, as we use copy-on-write and don't want to move too much memory
* around when there is a child performing saving operations.
*
* Note that even when mCanResize is set to 0, not all resizes are
* prevented: a hash table is still allowed to grow if the ratio between
* the number of elements and the buckets > G_DICT_FORCE_RESIZE_RATIO. */
static const u32 G_DICT_FORCE_RESIZE_RATIO = 5;

//This is the initial size of every hash table
static const u32 G_DICT_MIN_SIZE = 8;

u32 random() {
    return rand();
}

/* Our hash table capability is a power of two */
u64 AppGetNextPower(u64 size) {
    u64 i = G_DICT_MIN_SIZE;
    if(size >= 0x7FFFFFFFFFFFFFFFULL) {
        return 0x7FFFFFFFFFFFFFFFULL + 1ULL;
    }
    while(1) {
        if(i >= size) {
            return i;
        }
        i *= 2;
    }
}

/**
* @brief Function to reverse bits.
* Algorithm from:
* http://graphics.stanford.edu/~seander/bithacks.html#ReverseParallel */
u64 AppReverseBits(u64 v) {
    u64 s = 8 * sizeof(v); // bit size; must be power of 2
    u64 mask = ~0LL;
    while((s >>= 1) > 0) {
        mask ^= (mask << s);
        v = ((v >> s) & mask) | ((v << s) & ~mask);
    }
    return v;
}


HashDict::HashDict(const DictFunctions& iType, void* iUserData) :
    mCallers(iType),
    mUserPointer(iUserData),
    mRehashIndex(-1),
    mIterators(0),
    mCanResize(true) {
}

HashDict::~HashDict() {
    clear();
}


s32 HashDict::resize() {
    u64 minimal;

    if(!mCanResize || isRehashing()) {
        return DICT_ERR;
    }
    minimal = mHashTable[0].mUsed;
    if(minimal < G_DICT_MIN_SIZE) {
        minimal = G_DICT_MIN_SIZE;
    }
    return reallocate(minimal);
}

s32 HashDict::reallocate(u64 size) {
    /* the size is invalid if it is smaller than the number of
     * elements already inside the hash table */
    if(isRehashing() || mHashTable[0].mUsed > size) {
        return DICT_ERR;
    }
    HashTable n; /* the new hash table */
    u64 realsize = AppGetNextPower(size);

    /* Rehashing to the same table size is not useful. */
    if(realsize == mHashTable[0].mAllocated) {
        return DICT_ERR;
    }
    /* Allocate the new hash table and initialize all pointers to nullptr */
    n.mAllocated = realsize;
    n.mSizeMask = realsize - 1;
    n.mTable = reinterpret_cast<DictNode * *>(calloc(1, realsize * sizeof(DictNode*)));
    n.mUsed = 0;

    /* Is this the first initialization? If so it's not really a rehashing
     * we just set the first hash table so that it can accept keys. */
    if(mHashTable[0].mTable == nullptr) {
        mHashTable[0] = n;
        return DICT_OK;
    }

    /* Prepare a second hash table for incremental rehashing */
    mHashTable[1] = n;
    mRehashIndex = 0;
    return DICT_OK;
}


s32 HashDict::rehashBatch(s32 n) {
    s32 empty_visits = n * 10; /* Max number of empty buckets to visit. */
    if(!isRehashing()) {
        return 0;
    }
    while(n-- && mHashTable[0].mUsed != 0) {
        DictNode* de, * nextde;

        /* Note that mRehashIndex can't overflow as we are sure there are more
         * elements because mHashTable[0].mUsed != 0 */
        DASSERT(mHashTable[0].mAllocated > (u64)mRehashIndex);
        while(mHashTable[0].mTable[mRehashIndex] == nullptr) {
            mRehashIndex++;
            if(--empty_visits == 0) {
                return 1;
            }
        }
        de = mHashTable[0].mTable[mRehashIndex];
        /* Move all the keys in this bucket from the old to the new hash HT */
        while(de) {
            u64 h;

            nextde = de->mNext;
            /* Get the index in the new hash table */
            h = getKeyHash(de->mKey) & mHashTable[1].mSizeMask;
            de->mNext = mHashTable[1].mTable[h];
            mHashTable[1].mTable[h] = de;
            mHashTable[0].mUsed--;
            mHashTable[1].mUsed++;
            de = nextde;
        }
        mHashTable[0].mTable[mRehashIndex] = nullptr;
        mRehashIndex++;
    }

    /* Check if we already rehashed the whole table... */
    if(mHashTable[0].mUsed == 0) {
        free(mHashTable[0].mTable);
        mHashTable[0] = mHashTable[1];
        mHashTable[1].init();
        mRehashIndex = -1;
        return 0;
    }

    /* More to rehash... */
    return 1;
}


s32 HashDict::rehashWithTimeout(s32 ms) {
    s64 start = Timer::getRelativeTime();
    s32 rehashes = 0;

    while(rehashBatch(100)) {
        rehashes += 100;
        if(Timer::getRelativeTime() - start > ms) {
            break;
        }
    }
    return rehashes;
}


void HashDict::rehashStep() {
    if(mIterators == 0) {
        rehashBatch(1);
    }
}

s32 HashDict::add(void* key, void* val) {
    DictNode* entry = addRaw(key, nullptr);

    if(!entry) { return DICT_ERR; }
    setValue(entry, val);
    return DICT_OK;
}

DictNode* HashDict::addRaw(const void* key, DictNode** existing) {
    s64 index;
    if(isRehashing()) {
        rehashStep();
    }
    /* Get the index of the new element, or -1 if
     * the element already exists. */
    if((index = getKeyIndex(key, getKeyHash(key), existing)) == -1) {
        return nullptr;
    }
    /* Allocate the memory and store the new entry.
     * Insert the element in top, with the assumption that in a database
     * system it is more likely that recently added entries are accessed
     * more frequently. */
    HashTable* pht = isRehashing() ? &mHashTable[1] : &mHashTable[0];
    DictNode* entry = reinterpret_cast<DictNode*>(malloc(sizeof(*entry)));
    entry->mNext = pht->mTable[index];
    pht->mTable[index] = entry;
    pht->mUsed++;

    /* Set the hash entry fields. */
    setKey(entry, key);
    return entry;
}

s32 HashDict::addOrReplace(const void* key, const void* val) {
    DictNode* entry, * existing, auxentry;

    /* Try to add the element. If the key does not exists add will succeed. */
    entry = addRaw(key, &existing);
    if(entry) {
        setValue(entry, val);
        return 1;
    }

    /* Set the new value and free the old one. Note that it is important
     * to do that in this order, as the value may just be exactly the same
     * as the previous one. In this context, think to reference counting,
     * you want to increment (set), and then decrement (free), and not the
     * reverse. */
    auxentry = *existing;
    setValue(existing, val);
    releaseValue(&auxentry);
    return 0;
}


DictNode* HashDict::addOrFind(const void* key) {
    DictNode* entry, * existing;
    entry = addRaw(key, &existing);
    return entry ? entry : existing;
}

DictNode* HashDict::genericRemove(const void* key, s32 nofree) {
    if(mHashTable[0].mUsed == 0 && mHashTable[1].mUsed == 0) {
        return nullptr;
    }
    if(isRehashing()) {
        rehashStep();
    }
    DictNode* he, * prevHe;
    u64 idx;
    u64 h = getKeyHash(key);
    for(s32 table = 0; table <= 1; table++) {
        idx = h & mHashTable[table].mSizeMask;
        he = mHashTable[table].mTable[idx];
        prevHe = nullptr;
        while(he) {
            if(key == he->mKey || compareKeys(key, he->mKey)) {
                //Unlink the element from the list
                if(prevHe) {
                    prevHe->mNext = he->mNext;
                } else {
                    mHashTable[table].mTable[idx] = he->mNext;
                }
                if(!nofree) {
                    releaseKey(he);
                    releaseValue(he);
                    free(he);
                }
                mHashTable[table].mUsed--;
                return he;
            }
            prevHe = he;
            he = he->mNext;
        }
        if(!isRehashing()) {
            break;
        }
    }
    return nullptr; /* not found */
}


s32 HashDict::remove(const void* key) {
    return genericRemove(key, 0) ? DICT_OK : DICT_ERR;
}

DictNode* HashDict::unlink(const void* key) {
    return genericRemove(key, 1);
}


void HashDict::releaseNode(DictNode* he) {
    if(he == nullptr) {
        return;
    }
    releaseKey(he);
    releaseValue(he);
    free(he);
}

/* Destroy an entire dictionary */
s32 HashDict::clearTable(HashTable* ht) {
    for(u64 i = 0; i < ht->mAllocated && ht->mUsed > 0; i++) {
        DictNode* he, * nextHe;
        if((he = ht->mTable[i]) == nullptr) {
            continue;
        }
        while(he) {
            nextHe = he->mNext;
            releaseKey(he);
            releaseValue(he);
            free(he);
            ht->mUsed--;
            he = nextHe;
        }
    }
    /* Free the table and the allocated cache structure */
    free(ht->mTable);
    /* Re-initialize the table */
    ht->init();
    return DICT_OK; /* never fails */
}


DictNode* HashDict::find(const void* key) {
    DictNode* he;
    u64 h, idx, table;

    if(mHashTable[0].mUsed + mHashTable[1].mUsed == 0) {
        return nullptr;
    }
    if(isRehashing()) {
        rehashStep();
    }
    h = getKeyHash(key);
    for(table = 0; table <= 1; table++) {
        idx = h & mHashTable[table].mSizeMask;
        he = mHashTable[table].mTable[idx];
        while(he) {
            if(key == he->mKey || compareKeys(key, he->mKey)) {
                return he;
            }
            he = he->mNext;
        }
        if(!isRehashing()) {
            return nullptr;
        }
    }
    return nullptr;
}

void* HashDict::getValue(const void* key) {
    DictNode* he = find(key);
    return he ? ((he)->mValue.mVal) : nullptr;
}


s64 HashDict::getFingerprint() const {
    s64 integers[6], hash = 0;
    s32 j;

    integers[0] = reinterpret_cast<s64>(mHashTable[0].mTable);
    integers[1] = mHashTable[0].mAllocated;
    integers[2] = mHashTable[0].mUsed;
    integers[3] = reinterpret_cast<s64>(mHashTable[1].mTable);
    integers[4] = mHashTable[1].mAllocated;
    integers[5] = mHashTable[1].mUsed;

    /* We hash N integers by summing every successive integer with the integer
     * hashing of the previous sum. Basically:
     *
     * Result = hash(hash(hash(int1)+int2)+int3) ...
     *
     * This way the same set of integers in a different order will (likely) hash
     * to a different number. */
    for(j = 0; j < 6; j++) {
        hash += integers[j];
        /* For the hashing step we use Tomas Wang's 64 bit integer hash. */
        hash = (~hash) + (hash << 21); // hash = (hash << 21) - hash - 1;
        hash = hash ^ (hash >> 24);
        hash = (hash + (hash << 3)) + (hash << 8); // hash * 265
        hash = hash ^ (hash >> 14);
        hash = (hash + (hash << 2)) + (hash << 4); // hash * 21
        hash = hash ^ (hash >> 28);
        hash = hash + (hash << 31);
    }
    return hash;
}

HashDict::SIterator* HashDict::createIterator() {
    SIterator* iter = reinterpret_cast<SIterator*>(malloc(sizeof(*iter)));
    iter->mDict = this;
    iter->mTableID = 0;
    iter->mIndex = -1;
    iter->mSafe = false;
    iter->mNode = nullptr;
    iter->mNextNode = nullptr;
    return iter;
}

HashDict::SIterator* HashDict::createSafeIterator() {
    HashDict::SIterator* ret = createIterator();
    ret->mSafe = true;
    return ret;
}

DictNode* HashDict::SIterator::getNext() {
    while(1) {
        if(mNode == nullptr) {
            HashTable* ht = &mDict->mHashTable[mTableID];
            if(mIndex == -1 && mTableID == 0) {
                if(mSafe) {
                    mDict->mIterators++;
                } else {
                    mFingerprint = mDict->getFingerprint();
                }
            }
            mIndex++;
            if(mIndex >= (s64)ht->mAllocated) {
                if(mDict->isRehashing() && mTableID == 0) {
                    mTableID++;
                    mIndex = 0;
                    ht = &mDict->mHashTable[1];
                } else {
                    break;
                }
            }
            mNode = ht->mTable[mIndex];
        } else {
            mNode = mNextNode;
        }
        if(mNode) {
            /* We need to save the 'next' here, the iterator user
             * may delete the entry we are returning. */
            mNextNode = mNode->mNext;
            return mNode;
        }
    }
    return nullptr;
}

void HashDict::releaseIterator(HashDict::SIterator* iter) {
    if(!(iter->mIndex == -1 && iter->mTableID == 0)) {
        if(iter->mSafe) {
            iter->mDict->mIterators--;
        } else {
            DASSERT(iter->mFingerprint == iter->mDict->getFingerprint());
        }
    }
    free(iter);
}


DictNode* HashDict::getRandomKey() {
    DictNode* he, * orighe;
    u64 h;
    s32 listlen, listele;

    if(getSize() == 0) {
        return nullptr;
    }
    if(isRehashing()) {
        rehashStep();
    }
    if(isRehashing()) {
        do {
            /* We are sure there are no elements in indexes from 0 to mRehashIndex-1 */
            h = mRehashIndex + (random() % (mHashTable[0].mAllocated + mHashTable[1].mAllocated - mRehashIndex));
            he = (h >= mHashTable[0].mAllocated) ? mHashTable[1].mTable[h - mHashTable[0].mAllocated] : mHashTable[0].mTable[h];
        } while(he == nullptr);
    } else {
        do {
            h = random() & mHashTable[0].mSizeMask;
            he = mHashTable[0].mTable[h];
        } while(he == nullptr);
    }

    /* Now we found a non empty bucket, but it is a linked
     * list and we need to get a random element from the list.
     * The only sane way to do so is counting the elements and
     * select a random index. */
    listlen = 0;
    orighe = he;
    while(he) {
        he = he->mNext;
        listlen++;
    }
    listele = random() % listlen;
    he = orighe;
    while(listele--) {
        he = he->mNext;
    }
    return he;
}


u64 HashDict::getRandomKeys(DictNode** des, u64 count) {
    u64 j; /* internal hash table id, 0 or 1. */
    u64 tables; /* 1 or 2 tables? */
    u64 stored = 0, maxsizemask;
    u64 maxsteps;

    if(getSize() < count) {
        count = getSize();
    }
    maxsteps = count * 10;

    /* Try to do a rehashing work proportional to 'count'. */
    for(j = 0; j < count; j++) {
        if(isRehashing()) {
            rehashStep();
        } else {
            break;
        }
    }

    tables = isRehashing() ? 2 : 1;
    maxsizemask = mHashTable[0].mSizeMask;
    if(tables > 1 && maxsizemask < mHashTable[1].mSizeMask) {
        maxsizemask = mHashTable[1].mSizeMask;
    }
    /* Pick a random point inside the larger table. */
    u64 i = random() & maxsizemask;
    u64 emptylen = 0; /* Continuous empty entries so far. */
    while(stored < count && maxsteps--) {
        for(j = 0; j < tables; j++) {
            /* Invariant of the HashDict.c rehashing: up to the indexes already
             * visited in mHashTable[0] during the rehashing, there are no populated
             * buckets, so we can skip mHashTable[0] for indexes between 0 and idx-1. */
            if(tables == 2 && j == 0 && i < (u64)mRehashIndex) {
                /* Moreover, if we are currently out of range in the second
                 * table, there will be no elements in both tables up to
                 * the current rehashing index, so we jump if possible.
                 * (this happens when going from big to small table). */
                if(i >= mHashTable[1].mAllocated)
                    i = mRehashIndex;
                else
                    continue;
            }
            if(i >= mHashTable[j].mAllocated) {
                continue;
            }
            DictNode* he = mHashTable[j].mTable[i];

            /* Count contiguous empty buckets, and jump to other
             * locations if they reach 'count' (with a minimum of 5). */
            if(he == nullptr) {
                emptylen++;
                if(emptylen >= 5 && emptylen > count) {
                    i = random() & maxsizemask;
                    emptylen = 0;
                }
            } else {
                emptylen = 0;
                while(he) {
                    /* Collect all the elements of the buckets found non
                     * empty while iterating. */
                    *des = he;
                    des++;
                    he = he->mNext;
                    stored++;
                    if(stored == count) {
                        return stored;
                    }
                }
            }
        }
        i = (i + 1) & maxsizemask;
    }
    return stored;
}


u64 HashDict::scanAll(u64 v, AppHashDictScanCallback* fn, AppHashSlotScanCallback* bucketfn) {
    HashTable* t0, * t1;
    const DictNode* de, * next;
    u64 m0, m1;

    if(getSize() == 0) {
        return 0;
    }
    if(!isRehashing()) {
        t0 = &(mHashTable[0]);
        m0 = t0->mSizeMask;

        /* Emit entries at cursor */
        if(bucketfn) {
            bucketfn(mUserPointer, &t0->mTable[v & m0]);
        }
        de = t0->mTable[v & m0];
        while(de) {
            next = de->mNext;
            fn(mUserPointer, de);
            de = next;
        }

        /* Set unmasked bits so incrementing the reversed cursor
         * operates on the masked bits */
        v |= ~m0;

        /* Increment the reverse cursor */
        v = AppReverseBits(v);
        v++;
        v = AppReverseBits(v);

    } else {
        t0 = &mHashTable[0];
        t1 = &mHashTable[1];

        /* Make sure t0 is the smaller and t1 is the bigger table */
        if(t0->mAllocated > t1->mAllocated) {
            t0 = &mHashTable[1];
            t1 = &mHashTable[0];
        }

        m0 = t0->mSizeMask;
        m1 = t1->mSizeMask;

        /* Emit entries at cursor */
        if(bucketfn) {
            bucketfn(mUserPointer, &t0->mTable[v & m0]);
        }
        de = t0->mTable[v & m0];
        while(de) {
            next = de->mNext;
            fn(mUserPointer, de);
            de = next;
        }

        /* Iterate over indices in larger table that are the expansion
         * of the index pointed to by the cursor in the smaller table */
        do {
            /* Emit entries at cursor */
            if(bucketfn) bucketfn(mUserPointer, &t1->mTable[v & m1]);
            de = t1->mTable[v & m1];
            while(de) {
                next = de->mNext;
                fn(mUserPointer, de);
                de = next;
            }

            /* Increment the reverse cursor not covered by the smaller mask.*/
            v |= ~m1;
            v = AppReverseBits(v);
            v++;
            v = AppReverseBits(v);

            /* Continue while bits covered by mask difference is non-zero */
        } while(v & (m0 ^ m1));
    }

    return v;
}


s32 HashDict::expandIfNeeded() {
    /* Incremental rehashing already in progress. Return. */
    if(isRehashing()) {
        return DICT_OK;
    }
    /* If the hash table is empty expand it to the initial size. */
    if(mHashTable[0].mAllocated == 0) {
        return reallocate(G_DICT_MIN_SIZE);
    }
    /* If we reached the 1:1 ratio, and we are allowed to resize the hash
     * table or we should avoid it but the ratio between
     * elements/buckets is over the "safe" threshold, we resize doubling
     * the number of buckets. */
    if(mHashTable[0].mUsed >= mHashTable[0].mAllocated &&
        (mCanResize || mHashTable[0].mUsed / mHashTable[0].mAllocated > G_DICT_FORCE_RESIZE_RATIO)) {
        return reallocate(mHashTable[0].mUsed * 2);
    }
    return DICT_OK;
}



s64 HashDict::getKeyIndex(const void* key, u64 hash, DictNode** existing) {
    u64 idx, table;
    DictNode* he;
    if(existing) {
        *existing = nullptr;
    }
    if(expandIfNeeded() == DICT_ERR) {
        return -1;
    }
    for(table = 0; table <= 1; table++) {
        idx = hash & mHashTable[table].mSizeMask;
        /* Search if this slot does not already contain the given key */
        he = mHashTable[table].mTable[idx];
        while(he) {
            if(key == he->mKey || compareKeys(key, he->mKey)) {
                if(existing) {
                    *existing = he;
                }
                return -1;
            }
            he = he->mNext;
        }
        if(!isRehashing()) {
            break;
        }
    }
    return idx;
}

void HashDict::clear() {
    clearTable(&mHashTable[0]);
    clearTable(&mHashTable[1]);
    mRehashIndex = -1;
    mIterators = 0;
}


u64 HashDict::getHash(const void* key) {
    return getKeyHash(key);
}


DictNode** HashDict::findNodeRefByPtrAndHash(const void* oldptr, u64 hash) {
    DictNode* he, ** heref;
    u64 idx, table;

    if(mHashTable[0].mUsed + mHashTable[1].mUsed == 0) {
        return nullptr;
    }
    for(table = 0; table <= 1; table++) {
        idx = hash & mHashTable[table].mSizeMask;
        heref = &mHashTable[table].mTable[idx];
        he = *heref;
        while(he) {
            if(oldptr == he->mKey) {
                return heref;
            }
            heref = &he->mNext;
            he = *heref;
        }
        if(!isRehashing()) {
            return nullptr;
        }
    }
    return nullptr;
}

/* ------------------------------- Debugging ---------------------------------*/
#define DICT_STATS_VECTLEN 50
usz HashDict::getTableStats(s8 * buf, usz bufsize, HashDict::HashTable * ht, s32 tableid) {
    u64 i, slots = 0, chainlen, maxchainlen = 0;
    u64 totchainlen = 0;
    u32 clvector[DICT_STATS_VECTLEN];
    usz l = 0;

    if(ht->mUsed == 0) {
        return snprintf(buf, bufsize, "No stats available for empty dictionaries\n");
    }

    /* Compute stats. */
    for(i = 0; i < DICT_STATS_VECTLEN; i++) clvector[i] = 0;
    for(i = 0; i < ht->mAllocated; i++) {
        DictNode* he;

        if(ht->mTable[i] == nullptr) {
            clvector[0]++;
            continue;
        }
        slots++;
        /* For each hash entry on this slot... */
        chainlen = 0;
        he = ht->mTable[i];
        while(he) {
            chainlen++;
            he = he->mNext;
        }
        clvector[(chainlen < DICT_STATS_VECTLEN) ? chainlen : (DICT_STATS_VECTLEN - 1)]++;
        if(chainlen > maxchainlen) maxchainlen = chainlen;
        totchainlen += chainlen;
    }

    /* Generate human readable stats. */
    l += snprintf(buf + l, bufsize - l,
        "Hash table[%d] stats:\n"
        " Allocated slots: %llu\n"
        " Elements: %llu\n"
        " Different slots: %llu\n"
        " Slots used ratio: %.02f\n"
        " Max chain length: %llu\n"
        " avg chain length (counted): %.02f\n"
        " avg chain length (computed): %.02f\n"
        " Chain length distribution:\n",
        tableid, ht->mAllocated, ht->mUsed, slots,
        (f32)slots / ht->mAllocated, maxchainlen,
        (f32)totchainlen / slots, (f32)ht->mUsed / slots);

    for(i = 0; i < DICT_STATS_VECTLEN - 1; i++) {
        if(clvector[i] == 0) { continue; }
        if(l >= bufsize) { break; }
        l += snprintf(buf + l, bufsize - l,
            "   %s%llu: %ld (%.02f%%)\n",
            (i == DICT_STATS_VECTLEN - 1) ? ">= " : "",
            i, clvector[i], ((f32)clvector[i] / ht->mAllocated) * 100);
    }

    /* Unlike snprintf(), teturn the number of characters actually written. */
    if(bufsize) buf[bufsize - 1] = '\0';
    return strlen(buf);
}

void HashDict::getStats(s8* buf, usz bufsize) {
    usz l;
    s8* orig_buf = buf;
    usz orig_bufsize = bufsize;

    l = getTableStats(buf, bufsize, &mHashTable[0], 0);
    buf += l;
    bufsize -= l;
    if(isRehashing() && bufsize > 0) {
        getTableStats(buf, bufsize, &mHashTable[1], 1);
    }
    /* Make sure there is a nullptr term at the end. */
    if(orig_bufsize) orig_buf[orig_bufsize - 1] = '\0';
}

}//namespace app
