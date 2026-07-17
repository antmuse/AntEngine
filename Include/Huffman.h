#pragma once
#include "Config.h"

namespace app {

/**
 * @brief http2 huffman encoder
 * @param src the src buf.
 * @param len the len of src buf.
 * @param dest the dest buf.
 * @param lower convert to low chars.
 * @return the len of encoded buf, else 0 if fail(need bigger dest cache).
 */
usz AppHuffEncode(const u8* src, usz len, u8* dest, bool lower);

/**
 * @brief http2 huffman decoder
 * @param src the src buf.
 * @param len the len of src buf.
 * @param dest the tail of dest buf.
 * @param state last state of decode. pass 0 if first step decode.
 * @param last true if the \src is the last buf.
 * @return true if decode success, else fail.
 */
bool AppHuffDecode(const u8* src, usz len, u8** dest, u8* state, bool last);

} // namespace app
