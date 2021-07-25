#include "gzip/CodecGzip.h"


namespace umc {
namespace core {

class DecoderGzip : public CodecGzip {
public:
    // by default refuse operation if compressed data is > 1GB
    DecoderGzip(std::size_t max_bytes = 1000000000) :
        CodecGzip(max_bytes) {
        clear();
    }

    template <typename Tout>
    void decompress(Tout& output, const char* data, size_t size) {

#ifdef DEBUG
        // Verify if size (long type) input will fit into unsigned int, type used for zlib's avail_in
        std::uint64_t size_64 = size * 2;
        if (size_64 > std::numeric_limits<unsigned int>::max()) {
            finish();
            throw std::runtime_error("size arg is too large to fit into unsigned int type x2");
        }
#endif
        if (size > mMaxBytes || (size * 2) > mMaxBytes) {
            finish(output);
            throw std::runtime_error("size may use more memory than intended when decompressing");
        }

        mStream.next_in = reinterpret_cast<z_const Bytef*>(data);
        mStream.avail_in = static_cast<unsigned int>(size);
        mStream.avail_out = 0;

        size_t usedsz = output.size();
        size_t increase = size << 1;
        increase = increase < 128 ? 128 : increase;

        do {
            if (0 == mStream.avail_out) {
                std::size_t resize_to = usedsz + increase;
                if (resize_to > mMaxBytes) {
                    finish(output);
                    throw std::runtime_error("size of output string will use more memory then intended when decompressing");
                }
                output.resize(usedsz + increase);
                //output.resize(usedsz + size / 2 + 1024);
                mStream.avail_out = static_cast<unsigned int>(increase);
                mStream.next_out = reinterpret_cast<Bytef*>((char*)output.data() + usedsz);
            }

            int ret = inflate(&mStream, Z_NO_FLUSH);
            if (ret != Z_STREAM_END && ret != Z_OK && ret != Z_BUF_ERROR) {
                std::string error_msg = mStream.msg;
                finish(output);
                throw std::runtime_error(error_msg);
            }
            usedsz += (increase - mStream.avail_out);
        } while (mStream.avail_in > 0);
        output.resize(usedsz);
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
        // (8 to 15) + 32 to automatically detect core/zlib header
        constexpr int window_bits = 15 + 32; // auto with windowbits of 15

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
        if (inflateInit2(&mStream, window_bits) != Z_OK) {
            throw std::runtime_error("inflate init failed");
        }
#pragma GCC diagnostic pop
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
        inflateEnd(&mStream);
        output.resize(usedsz);
    }
};


} // namespace core
} //namespace umc