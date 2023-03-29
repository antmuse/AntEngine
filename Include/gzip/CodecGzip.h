#ifndef APP_CODECGZIP_H
#define APP_CODECGZIP_H

#include "Config.h"

#if defined(DUSE_ZLIB)

#include <cstdlib>

#ifndef ZLIB_CONST
#define ZLIB_CONST
#endif
#include <limits>
#include <stdexcept>
#include <string>
#include "zlib.h"

namespace app {

class CodecGzip {
public:
    CodecGzip(usz max_bytes = 1000000000) : mMaxBytes(max_bytes) {
    }

    ~CodecGzip() {
    }

    /**
     * These live in core.h because it doesnt need to use deps.
     * Otherwise, they would need to live in impl files if these methods used
     * zlib structures or functions like inflate/deflate)
     */
    inline static bool isCompressed(const s8* data, usz size) {
        return size > 2
               && (( // zlib
                       static_cast<u8>(data[0]) == 0x78
                       && (static_cast<u8>(data[1]) == 0x9C || static_cast<u8>(data[1]) == 0x01
                           || static_cast<u8>(data[1]) == 0xDA || static_cast<u8>(data[1]) == 0x5E))
                   || ( // core
                       static_cast<u8>(data[0]) == 0x1F && static_cast<u8>(data[1]) == 0x8B));
    }


    usz getInputSize() const {
        return mStream.total_in;
    }

    usz getOutputSize() const {
        return mStream.total_out;
    }

    /**
     * @return ratio = 100.f * output / input;
     */
    f32 getRatio() const {
        f32 inmax = mStream.total_in > 0 ? mStream.total_in : 1.f;
        return 100.f * mStream.total_out / inmax;
    }

protected:
    usz mMaxBytes;
    z_stream mStream;

    CodecGzip() = delete;
    CodecGzip(const CodecGzip&) = delete;
    CodecGzip(const CodecGzip&&) = delete;
    CodecGzip& operator=(const CodecGzip&) = delete;
    CodecGzip& operator=(const CodecGzip&&) = delete;
};


} // namespace app

#endif // DUSE_ZLIB
#endif // APP_CODECGZIP_H