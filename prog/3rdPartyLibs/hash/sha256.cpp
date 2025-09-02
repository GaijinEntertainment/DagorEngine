#ifndef _CRT_SECURE_NO_DEPRECATE
#define _CRT_SECURE_NO_DEPRECATE 1
#endif

#include "sha256.h"
#undef SHA256_DIGEST_LENGTH // To avoid redefine warning
#include <openssl/sha.h>
#include <debug/dag_assert.h>

// SHA-256 context setup
void sha256_starts(sha256_context *ctx)
{
  G_STATIC_ASSERT(sizeof(sha256_context) >= sizeof(SHA256_CTX));
  G_STATIC_ASSERT(offsetof(sha256_context, total) >= sizeof(SHA256_CTX));
  SHA256_Init((SHA256_CTX*)ctx);
  ctx->total[0] = ctx->total[1] = 0;
}

// SHA-256 process buffer
void sha256_update(sha256_context *ctx, const unsigned char *input, int ilen)
{
  SHA256_Update((SHA256_CTX*)ctx, input, ilen);
  if ((ctx->total[0] += ilen) < (unsigned)ilen)
    ctx->total[1] ++;
}

// SHA-256 final digest
void sha256_finish(sha256_context *ctx, unsigned char output[SHA256_DIGEST_LENGTH])
{
  SHA256_Final(output, (SHA256_CTX*)ctx);
}

// Output = SHA-256( input buffer )
void sha256_csum(const unsigned char *input, int ilen, unsigned char output[SHA256_DIGEST_LENGTH])
{
  SHA256_CTX ctx;

  SHA256_Init(&ctx);
  SHA256_Update(&ctx, input, ilen);
  SHA256_Final(output, &ctx);
}
