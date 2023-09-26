#ifndef APP_RINGBLOCKS_H
#define APP_RINGBLOCKS_H

#include "Config.h"

namespace app {

/**
 * @brief A lockfree ring buffers if there is only one thread reading and one thread writing.
 */
class RingBlocks {
public:
    // each block has 4 bytes head
    struct Block {
        u16 mUsed;
        u16 mOffset;
        s8 mBuf[];
    };

    RingBlocks();

    ~RingBlocks();

    void open(u32 slots, u16 blocksize = 4096);
    void close();

    u32 getBlockSize() const;

    /**@return null if none block to read. */
    Block* getHeadBlock();

    /**@return null if none block to write. */
    Block* getTailBlock();

    void commitHead();
    void commitTail();

    usz write(const void* buf, usz size);
    usz read(void* buf, usz size);

protected:
    static u32 upToPower2(u32 it);
    u32 getHead() const;
    u32 getTail() const;

    u32 mMask;
    u32 mBlockSize;
    u32 mHead;
    u32 mTail;
    s8* mCache;
};

} // namespace app
#endif // APP_RINGBLOCKS_H
