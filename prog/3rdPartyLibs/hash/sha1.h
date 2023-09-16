/**
 * \file sha1.h
 */
#ifndef _SHA1_H
#define _SHA1_H

#ifdef __cplusplus
extern "C" {
#endif

// SHA-1 context structure
struct sha1_context { unsigned storage[24]; unsigned total[2]; };

// SHA-1 context setup
void sha1_starts(sha1_context *ctx);

// SHA-1 process buffer
void sha1_update(sha1_context *ctx, const unsigned char *input, int ilen);

// SHA-1 final digest
void sha1_finish(sha1_context *ctx, unsigned char output[20]);

// Output = SHA-1( input buffer )
void sha1_csum(const unsigned char *input, int ilen, unsigned char output[20]);

#ifdef __cplusplus
}
#endif

#endif /* sha1.h */
