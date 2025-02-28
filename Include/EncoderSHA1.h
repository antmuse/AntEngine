#ifndef APP_ENCODERSHA1_H
#define APP_ENCODERSHA1_H

#include "TString.h"

namespace app {

#define SHA_BLOCKSIZE 64
#define SHA_DIGESTSIZE 20

class EncoderSHA1 {
public:
    EncoderSHA1();
    ~EncoderSHA1();
    void clear();
    void add(const void* data, usz size);
  
    //@return the result in hex str, 40 bytes;
    String finish();

    //@param buf at least 20 bytes buf;
    void finish(u8 buf[20]);

private:
    void transform(const u8* dp);

    u32 mDigest[5];          /* message digest */
    u32 mCountLow;           /* 64-bit bit count part1 */
    u32 mCountHi;            /* 64-bit bit count part2 */
    u8 mData[SHA_BLOCKSIZE]; /* SHA data buffer */
    u32 mLocal;              /* unprocessed amount in mData */
};

} // namespace app
#endif //  APP_ENCODERSHA1_H
