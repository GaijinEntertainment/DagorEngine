/**
 * \file sha256.h
 */
#ifndef _SHA256_H
#define _SHA256_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef SHA256_DIGEST_LENGTH
#define SHA256_DIGEST_LENGTH 32
#endif

// SHA-256 context structure
struct sha256_context { unsigned int storage[28]; unsigned int total[2]; };

// SHA-1 context setup
void sha256_starts(sha256_context *ctx);

// SHA-256 process buffer
void sha256_update(sha256_context *ctx, const unsigned char *input, int ilen);

// SHA-256 final digest
void sha256_finish(sha256_context *ctx, unsigned char output[SHA256_DIGEST_LENGTH]);

// Output = SHA-256( input buffer )
void sha256_csum(const unsigned char *input, int ilen, unsigned char output[SHA256_DIGEST_LENGTH]);

#ifdef __cplusplus
}
#endif

#endif /* sha256.h */
