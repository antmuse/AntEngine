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


#ifndef APP_HASHDICT_H
#define APP_HASHDICT_H

/**
 * @brief Hash dict from redis.
 *
 * This file implements in-memory hash tables with insert/del/replace/find/
 * get-random-element operations. Hash tables will auto-resize if needed
 * tables of power of two in size are used, collisions are handled by
 * chaining.
 */

#include "Config.h"


#define DICT_OK 0
#define DICT_ERR 1

namespace app {


struct DictNode {
    void* mKey;
    union {
        s32 mVal32S;
        u32 mVal32U;
        f32 mVal32F;
        void* mVal;
        u64 mVal64U;
        s64 mVal64S;
        f64 mVal64F;
    } mValue;
    DictNode* mNext;
};

struct DictFunctions {
    u64(*mHashFunction)(const void* key);
    void* (*mKeyConstructor)(void* iUserData, const void* key);
    void* (*mValueConstructor)(void* iUserData, const void* obj);
    //return true if equal
    bool(*mKeyCompare)(void* iUserData, const void* key1, const void* key2);
    void(*mKeyDestructor)(void* iUserData, void* key);
    void(*mValueDestructor)(void* iUserData, void* obj);
};

typedef void (AppHashDictScanCallback)(void* iUserData, const DictNode* de);
typedef void (AppHashSlotScanCallback)(void* iUserData, DictNode** bucketref);

class HashDict {
public:
    /**
    * Hash table structure.
    * Every dictionary has two of this as we
    * implement incremental rehashing, for the old to the new table.
    */
    class HashTable {
    public:
        DictNode** mTable;
        u64 mAllocated;
        u64 mSizeMask;
        u64 mUsed;

        HashTable() {
            init();
        }
        void init() {
            mTable = nullptr;
            mAllocated = 0;
            mSizeMask = 0;
            mUsed = 0;
        }
    };

    /* If safe is set to true this is a safe iterator, that means, you can call
    * add, find, and other functions against the dictionary even while
    * iterating. Otherwise it is a non safe iterator, and only dictNext()
    * should be called while iterating. */
    struct SIterator {
        bool mSafe;
        s16 mTableID;
        s64 mIndex;
        HashDict* mDict;
        DictNode* mNode;
        DictNode* mNextNode;
        /* unsafe iterator fingerprint for misuse detection. */
        s64 mFingerprint;
        DictNode* getNext();
    };

    HashDict(const DictFunctions& iType, void* iUserPointer = nullptr);

    ~HashDict();


    void enableResize() {
        mCanResize = true;
    }

    void disableResize() {
        mCanResize = false;
    }

    /* A fingerprint is a 64 bit number that represents the state of the dictionary
    * at a given time, it's just a few HashDict properties xored together.
    * When an unsafe iterator is initialized, we get the HashDict fingerprint, and check
    * the fingerprint again when the iterator is released.
    * If the two fingerprints are different it means that the user of the iterator
    * performed forbidden operations against the dictionary while iterating. */
    s64 getFingerprint()const;

    bool isRehashing() const {
        return (mRehashIndex != -1);
    }

    void setKey(DictNode* entry, const void* _key_) const {
        if(mCallers.mKeyConstructor) {
            entry->mKey = mCallers.mKeyConstructor(mUserPointer, _key_);
        } else {
            entry->mKey = const_cast<void*>(_key_);
        }
    }

    void setValue(DictNode* entry, const void* _val_)const {
        if(mCallers.mValueConstructor) {
            entry->mValue.mVal = mCallers.mValueConstructor(mUserPointer, _val_);
        } else {
            entry->mValue.mVal = const_cast<void*>(_val_);
        }
    }

    bool compareKeys(const void* key1, const void* key2)const {
        return (mCallers.mKeyCompare ? mCallers.mKeyCompare(mUserPointer, key1, key2) : key1 == key2);
    }

    /**
    * @brief Resize the table to the minimal size that contains all the elements,
    * but with the invariant of a USED/BUCKETS ratio near to <= 1 */
    s32 resize();

    //Expand or create the hash table
    s32 reallocate(u64 size);

    u64 getSize() const {
        return (mHashTable[0].mUsed + mHashTable[1].mUsed);
    }

    void* getValue(const void* key);

    /**
    * @brief used to iterate over the elements of a dictionary.
    *
    * Iterating works the following way:
    *
    * 1) Initially you call the function using a cursor (v) value of 0.
    * 2) The function performs one step of the iteration, and returns the
    *    new cursor value you must use in the next call.
    * 3) When the returned cursor is 0, the iteration is complete.
    *
    * The function guarantees all elements present in the
    * dictionary get returned between the start and end of the iteration.
    * However it is possible some elements get returned multiple times.
    *
    * For every element returned, the callback argument 'fn' is
    * called with 'iUserData' as first argument and the dictionary entry
    * 'de' as second argument.
    *
    * HOW IT WORKS.
    *
    * The iteration algorithm was designed by Pieter Noordhuis.
    * The main idea is to increment a cursor starting from the higher order
    * bits. That is, instead of incrementing the cursor normally, the bits
    * of the cursor are reversed, then the cursor is incremented, and finally
    * the bits are reversed again.
    *
    * This strategy is needed because the hash table may be resized between
    * iteration calls.
    *
    * HashDict's hash tables are always power of two in size, and they
    * use chaining, so the position of an element in a given table is given
    * by computing the bitwise AND between Hash(key) and SIZE-1
    * (where SIZE-1 is always the mask that is equivalent to taking the rest
    *  of the division between the Hash of the key and SIZE).
    *
    * For example if the current hash table size is 16, the mask is
    * (in binary) 1111. The position of a key in the hash table will always be
    * the last four bits of the hash output, and so forth.
    *
    * WHAT HAPPENS IF THE TABLE CHANGES IN SIZE?
    *
    * If the hash table grows, elements can go anywhere in one multiple of
    * the old bucket: for example let's say we already iterated with
    * a 4 bit cursor 1100 (the mask is 1111 because hash table size = 16).
    *
    * If the hash table will be resized to 64 elements, then the new mask will
    * be 111111. The new buckets you obtain by substituting in ??1100
    * with either 0 or 1 can be targeted only by keys we already visited
    * when scanning the bucket 1100 in the smaller hash table.
    *
    * By iterating the higher bits first, because of the inverted counter, the
    * cursor does not need to restart if the table size gets bigger. It will
    * continue iterating using cursors without '1100' at the end, and also
    * without any other combination of the final 4 bits already explored.
    *
    * Similarly when the table size shrinks over time, for example going from
    * 16 to 8, if a combination of the lower three bits (the mask for size 8
    * is 111) were already completely explored, it would not be visited again
    * because we are sure we tried, for example, both 0111 and 1111 (all the
    * variations of the higher bit) so we don't need to test it again.
    *
    * WAIT... YOU HAVE *TWO* TABLES DURING REHASHING!
    *
    * Yes, this is true, but we always iterate the smaller table first, then
    * we test all the expansions of the current cursor into the larger
    * table. For example if the current cursor is 101 and we also have a
    * larger table of size 16, we also test (0)101 and (1)101 inside the larger
    * table. This reduces the problem back to having only one table, where
    * the larger one, if it exists, is just an expansion of the smaller one.
    *
    * LIMITATIONS
    *
    * This iterator is completely stateless, and this is a huge advantage,
    * including no additional memory used.
    *
    * The disadvantages resulting from this design are:
    *
    * 1) It is possible we return elements more than once. However this is usually
    *    easy to deal with in the application level.
    * 2) The iterator must return multiple elements per call, as it needs to always
    *    return all the keys chained in a given bucket, and all the expansions, so
    *    we are sure we don't miss keys moving during rehashing.
    * 3) The reverse cursor is somewhat hard to understand at first, but this
    *    comment is supposed to help.
    */
    u64 scanAll(u64 v, AppHashDictScanCallback* fn, AppHashSlotScanCallback* bucketfn);

    /**
    * @brief Performs N steps of incremental rehashing. Returns 1 if there are still
    * keys to move from the old to the new hash table, otherwise 0 is returned.
    *
    * Note that a rehashing step consists in moving a bucket (that may have more
    * than one key as we use chaining) from the old to the new hash table, however
    * since part of the hash table may be composed of empty spaces, it is not
    * guaranteed that this function will rehash even a single bucket, since it
    * will visit at max N*10 empty buckets in total, otherwise the amount of
    * work it does would be unbound and the function may block for a long time. */
    s32 rehashBatch(s32 n);

    u64 getKeyHash(const void* key) const {
        return mCallers.mHashFunction(key);
    }

    /**
    * @brief Rehash for an amount of time between ms milliseconds and ms+1 milliseconds
    * @param ms time in milliseconds.
    */
    s32 rehashWithTimeout(s32 ms);


    /**
    * @brief Add or Overwrite:
    * Add an element, discarding the old value if the key already exists.
    * Return 1 if the key was added from scratch, 0 if there was already an
    * element with such key and addOrReplace() just performed a value update
    * operation. */
    s32 addOrReplace(const void* key, const void* val);

    /**
    * @brief Add an element to hash table
    */
    s32 add(void* key, void* val);

    /**
    * @brief Add or Find:
    * addOrFind() is simply a version of addRaw() that always
    * returns the hash entry of the specified key, even if the key already
    * exists and can't be added (in that case the entry of the already
    * existing key is returned.)
    *
    * See addRaw() for more information. */
    DictNode* addOrFind(const void* key);

    /**
    * @brief Remove an element.
    * @return DICT_OK on success or DICT_ERR if the element was not found.
    */
    s32 remove(const void* key);

    /**
    * @brief Remove an element from the table, but without actually releasing
    * the key, value and dictionary entry. The dictionary entry is returned
    * if the element was found (and unlinked from the table), and the user
    * should later call `releaseNode()` with it in order to release it.
    * Otherwise if the key is not found, nullptr is returned.
    *
    * This function is useful when we want to remove something from the hash
    * table but want to use its value before actually deleting the entry.
    * Without this function the pattern would require two lookups:
    *
    *  entry = find(...);
    *  // Do something with entry
    *  remove(dictionary,entry);
    *
    * Thanks to this function it is possible to avoid this, and use instead:
    *
    * entry = unlink(dictionary,entry);
    * // Do something with entry
    * releaseNode(entry); // <- This does not need to lookup again.
    */
    DictNode* unlink(const void* key);

    DictNode* find(const void* key);

    /**
    * @return a random entry from the hash table.
    * Useful to implement randomized algorithms */
    DictNode* getRandomKey();

    /* This function samples the dictionary to return a few keys from random
    * locations.
    *
    * It does not guarantee to return all the keys specified in 'count', nor
    * it does guarantee to return non-duplicated elements, however it will make
    * some effort to do both things.
    *
    * Returned pointers to hash table entries are stored into 'des' that
    * points to an array of DictNode pointers. The array must have room for
    * at least 'count' elements, that is the argument we pass to the function
    * to tell how many random elements we need.
    *
    * The function returns the number of items stored into 'des', that may
    * be less than 'count' if the hash table has less than 'count' elements
    * inside, or if not enough elements were found in a reasonable amount of
    * steps.
    *
    * Note that this function is not suitable when you need a good distribution
    * of the returned items, but only when you need to "sample" a given number
    * of continuous elements to run some kind of algorithm or to produce
    * statistics. However the function is much faster than getRandomKey()
    * at producing N elements. */
    u64 getRandomKeys(DictNode** des, u64 count);

    u64 getHash(const void* key);

    /**
    * @brief Finds the DictNode reference by using pointer and pre-calculated hash.
    * oldkey is a dead pointer and should not be accessed.
    * the hash value should be provided using getHash().
    * @return The reference to the DictNode if found, or nullptr if not found. */
    DictNode** findNodeRefByPtrAndHash(const void* oldptr, u64 hash);

    /**
    * @brief You need to call this function to really free the entry after a call
    * to unlink(). It's safe to call this function with 'he' = nullptr. */
    void releaseNode(DictNode* he);

    void releaseKey(DictNode* entry)const {
        if(mCallers.mKeyDestructor) {
            mCallers.mKeyDestructor(mUserPointer, entry->mKey);
        }
    }

    void releaseValue(DictNode* entry)const {
        if(mCallers.mValueDestructor) {
            mCallers.mValueDestructor(mUserPointer, entry->mValue.mVal);
        }
    }

    void clear();

    SIterator* createIterator();
    SIterator* createSafeIterator();
    void releaseIterator(SIterator* iter);

    void getStats(s8* buf, usz bufsize);

private:
    bool mCanResize;        //default: true
    s64 mRehashIndex;      //rehashing not in progress if mRehashIndex == -1
    u64 mIterators;         //number of iterators currently running
    void* mUserPointer;
    DictFunctions mCallers;
    HashTable mHashTable[2];

    /**
    * @brief Search and remove an element. This is an helper function for
    * remove() and unlink(), please check the top comment of those functions.
    */
    DictNode* genericRemove(const void* key, s32 nofree);


    /**
    * @brief Low level add or find:
    * This function adds the entry but instead of setting a value returns the
    * DictNode structure to the user, that will make sure to fill the value
    * field as he wishes.
    *
    * This function is also directly exposed to the user API to be called
    * mainly in order to store non-pointers inside the hash value, example:
    *
    * entry = addRaw(HashDict,mykey,nullptr);
    * if (entry != nullptr) dictSetSignedIntegerVal(entry,1000);
    *
    * Return values:
    *
    * If key already exists nullptr is returned, and "*existing" is populated
    * with the existing entry if existing is not nullptr.
    *
    * If key was added, the hash entry is returned to be manipulated by the caller.
    */
    DictNode* addRaw(const void* key, DictNode** existing);

    s32 expandIfNeeded();

    /* This function performs just a step of rehashing, and only if there are
    * no safe iterators bound to our hash table. When we have iterators in the
    * middle of a rehashing we can't mess with the two hash tables otherwise
    * some element can be missed or duplicated.
    *
    * This function is called by common lookup or update operations in the
    * dictionary so that the hash table automatically migrates from H1 to H2
    * while it is actively used. */
    void rehashStep();

    /* Returns the index of a free slot that can be populated with
    * a hash entry for the given 'key'.
    * If the key already exists, -1 is returned
    * and the optional output parameter may be filled.
    *
    * Note that if we are in the process of rehashing the hash table, the
    * index is always returned in the context of the second (new) hash table. */
    s64 getKeyIndex(const void* key, u64 hash, DictNode** existing);

    s32 clearTable(HashTable* ht);

    //for Debugging
    usz getTableStats(s8* buf, usz bufsize, HashTable* ht, s32 tableid);

    HashDict(const HashDict&) = delete;
    HashDict(HashDict&&) = delete;
    HashDict& operator=(const HashDict&) = delete;
    HashDict& operator=(HashDict&&) = delete;
};


}//namespace app

#endif //APP_HASHDICT_H
