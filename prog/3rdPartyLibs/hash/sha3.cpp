#ifndef _CRT_SECURE_NO_DEPRECATE
#define _CRT_SECURE_NO_DEPRECATE 1
#endif

#include "sha3.h"
#define sha3_context sha3_context_iuf  // Avoid naming conflict
extern "C" {
#include "SHA3IUF/sha3.h"
}
#undef sha3_context
#include <debug/dag_assert.h>
#include <string.h>

// SHA-3-256 context setup
void sha3_256_starts(sha3_context *ctx)
{
  G_STATIC_ASSERT(sizeof(sha3_context) >= sizeof(sha3_context_iuf));

  sha3_context_iuf *iuf_ctx = (sha3_context_iuf*)ctx;
  sha3_Init256(iuf_ctx);

  ctx->variant = 256;
}

// SHA-3 process buffer
void sha3_update(sha3_context *ctx, const unsigned char *input, int ilen)
{
  sha3_context_iuf *iuf_ctx = (sha3_context_iuf*)ctx;
  sha3_Update(iuf_ctx, input, ilen);
}

// SHA-3-256 final digest
void sha3_256_finish(sha3_context *ctx, unsigned char output[SHA3_256_DIGEST_LENGTH])
{
  sha3_context_iuf *iuf_ctx = (sha3_context_iuf*)ctx;
  const void *result = sha3_Finalize(iuf_ctx);
  memcpy(output, result, SHA3_256_DIGEST_LENGTH);
}

// Output = SHA-3-256( input buffer )
void sha3_256_csum(const unsigned char *input, int ilen, unsigned char output[SHA3_256_DIGEST_LENGTH])
{
  sha3_HashBuffer(256, SHA3_FLAGS_NONE, input, ilen, output, SHA3_256_DIGEST_LENGTH);
}