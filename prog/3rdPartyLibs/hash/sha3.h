/**
 * \file sha3.h
 */
#ifndef _SHA3_H
#define _SHA3_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef SHA3_256_DIGEST_LENGTH
#define SHA3_256_DIGEST_LENGTH 32
#endif

// SHA-3 context structure
struct sha3_context { unsigned char storage[224]; int variant; };

// SHA-3 context setup
void sha3_256_starts(sha3_context *ctx);

// SHA-3 process buffer
void sha3_update(sha3_context *ctx, const unsigned char *input, int ilen);

// SHA-3 final digest
void sha3_256_finish(sha3_context *ctx, unsigned char output[SHA3_256_DIGEST_LENGTH]);

// Output = SHA-3( input buffer )
void sha3_256_csum(const unsigned char *input, int ilen, unsigned char output[SHA3_256_DIGEST_LENGTH]);

#ifdef __cplusplus
}
#endif

#endif /* sha3.h */