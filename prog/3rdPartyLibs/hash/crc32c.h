#pragma once
#ifndef _CRC32C_H_
#define _CRC32C_H_

/*
    Computes CRC-32C (Castagnoli) checksum. Uses Intel's CRC32 instruction if it is available.
    Otherwise it uses a very fast software fallback.
*/

#ifdef __cplusplus
extern "C"
{
#endif

// Use hw if available, fallback to sw
unsigned crc32c_append(
    unsigned crc,               // Initial CRC value. Typically it's 0.
                                // You can supply non-trivial initial value here.
                                // Initial value can be used to chain CRC from multiple buffers.
    const unsigned char *input, // Data to be put through the CRC algorithm.
    int length);                // Length of the data in the input buffer.

unsigned crc32c_append_hw(unsigned crc,
                          const unsigned char *input,
                          int length);

unsigned crc32c_append_sw(unsigned crc,
                          const unsigned char *input,
                          int length);

#ifdef __cplusplus
}
#endif


#endif
