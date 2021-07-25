#include "gzip/CodecGzip.h"

namespace umc {
namespace core {

class EncoderGzip : public CodecGzip {
public:
    // by default refuse operation if uncompressed data is > 1GB
    EncoderGzip(int level = Z_DEFAULT_COMPRESSION, std::size_t max_bytes = 1000000000) :
        mLevel(level),
        CodecGzip(max_bytes) {
        clear();
    }

    void clear() {
        mStream.zalloc = nullptr;
        mStream.zfree = nullptr;
        mStream.opaque = nullptr;
        mStream.avail_in = 0;
        mStream.next_in = nullptr;
        mStream.avail_out = 0;
        mStream.next_out = nullptr;


        // The windowBits parameter is the base two logarithm of the window size (the size of the history buffer).
        // It should be in the range 8..15 for this version of the library.
        // Larger values of this parameter result in better compression at the expense of memory usage.
        // This range of values also changes the decoding type:
        //  -8 to -15 for raw deflate
        //  8 to 15 for zlib
        // (8 to 15) + 16 for core
        // (8 to 15) + 32 to automatically detect core/zlib header (decompression/inflate only)
        constexpr int window_bits = 15 + 16; // core with windowbits of 15

        constexpr int mem_level = 8;
        // The memory requirements for deflate are (in bytes):
        // (1 << (window_bits+2)) +  (1 << (mem_level+9))
        // with a default value of 8 for mem_level and our window_bits of 15
        // this is 128Kb

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
        if (deflateInit2(&mStream, mLevel, Z_DEFLATED, window_bits, mem_level, Z_DEFAULT_STRATEGY) != Z_OK) {
            throw std::runtime_error("deflate init failed");
        }
#pragma GCC diagnostic pop
    }


    template <typename Tout>
    void compress(Tout& output, const char* data, size_t size) {
#ifdef DEBUG
        // Verify if size input will fit into unsigned int, type used for zlib's avail_in
        if (size > std::numeric_limits<unsigned int>::max()) {
            throw std::runtime_error("size arg is too large to fit into unsigned int type");
        }
#endif
        if (size > mMaxBytes) {
            throw std::runtime_error("size may use more memory than intended when decompressing");
        }

        mStream.next_in = reinterpret_cast<z_const Bytef*>(data);
        mStream.avail_in = static_cast<unsigned int>(size);
        mStream.avail_out = 0;
        //mStream.next_out = reinterpret_cast<Bytef*>((char*)output.data() + usedsz);

        std::size_t usedsz = output.size();
        size_t increase = size >> 1;
        increase = increase < 128 ? 128 : increase;

        do {
            if (0 == mStream.avail_out) {
                output.resize(usedsz + increase);
                mStream.avail_out = static_cast<unsigned int>(increase);
                mStream.next_out = reinterpret_cast<Bytef*>((char*)output.data() + usedsz);
            }
            // From http://www.zlib.net/zlib_how.html
            // "deflate() has a return value that can indicate errors, yet we do not check it here.
            // Why not? Well, it turns out that deflate() can do no wrong here."
            // Basically only possible error is from deflateInit not working properly
            deflate(&mStream, Z_NO_FLUSH);
            usedsz += (increase - mStream.avail_out);
        } while (mStream.avail_in > 0);

        output.resize(usedsz);
    }

    template <typename Tout>
    void finish(Tout& output) {
        size_t usedsz = output.size();
        size_t increase = 128;
        mStream.avail_out = 0;
        //mStream.next_out = reinterpret_cast<Bytef*>((char*)output.data() + usedsz);
        do {
            if (0 == mStream.avail_out) {
                output.resize(usedsz + increase);
                mStream.avail_out = static_cast<unsigned int>(increase);
                mStream.next_out = reinterpret_cast<Bytef*>((char*)output.data() + usedsz);
            }
            deflate(&mStream, Z_FINISH);
            usedsz += (increase - mStream.avail_out);
        } while (0 == mStream.avail_out);
        deflateEnd(&mStream);
        output.resize(usedsz);
    }

protected:
    int mLevel;
};



} // namespace core
} //namespace umc