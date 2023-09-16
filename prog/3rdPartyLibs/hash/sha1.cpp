#ifndef _CRT_SECURE_NO_DEPRECATE
#define _CRT_SECURE_NO_DEPRECATE 1
#endif

#include "sha1.h"
#include <openssl/sha.h>
#include <debug/dag_assert.h>

// SHA-1 context setup
void sha1_starts(sha1_context *ctx)
{
  G_STATIC_ASSERT(sizeof(sha1_context) >= sizeof(SHA_CTX));
  G_STATIC_ASSERT(offsetof(sha1_context, total) >= sizeof(SHA_CTX));
  SHA1_Init((SHA_CTX*)ctx);
  ctx->total[0] = ctx->total[1] = 0;
}

// SHA-1 process buffer
void sha1_update(sha1_context *ctx, const unsigned char *input, int ilen)
{
  SHA1_Update((SHA_CTX*)ctx, input, ilen);
  if ((ctx->total[0] += ilen) < (unsigned)ilen)
    ctx->total[1] ++;
}

// SHA-1 final digest
void sha1_finish(sha1_context *ctx, unsigned char output[20])
{
  SHA1_Final(output, (SHA_CTX*)ctx);
}

// Output = SHA-1( input buffer )
void sha1_csum(const unsigned char *input, int ilen, unsigned char output[20])
{
  SHA_CTX ctx;

  SHA1_Init(&ctx);
  SHA1_Update(&ctx, input, ilen);
  SHA1_Final(output, &ctx);
}
