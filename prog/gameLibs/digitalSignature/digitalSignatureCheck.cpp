// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <openssl/evp.h>
#include <openssl/pem.h>
#include <digitalSignature/digitalSignatureCheck.h>
#include <utility>


namespace digital_signature
{

enum
{
  PKEY = 0,
  CTX = 1
};

Checker::Checker(Checker &&other) { *this = std::move(other); }

Checker &Checker::operator=(Checker &&other)
{
  opaque[PKEY] = other.opaque[PKEY];
  opaque[CTX] = other.opaque[CTX];
  failed = other.failed;
  done = other.done;

  other.opaque[PKEY] = nullptr;
  other.opaque[CTX] = nullptr;
  other.failed = true;
  other.done = true;

  return *this;
}

Checker::~Checker()
{
  EVP_PKEY *pkey = (EVP_PKEY *)opaque[PKEY];
  EVP_MD_CTX *ctx = (EVP_MD_CTX *)opaque[CTX];

  if (pkey)
    EVP_PKEY_free(pkey);
  if (ctx)
    EVP_MD_CTX_destroy(ctx);
}

Checker::Checker(const char *hash_algorithm, const unsigned char *public_key, int public_key_len)
{
  failed = true;

  const unsigned char *p = public_key;
  EVP_PKEY *pkey = d2i_PUBKEY(NULL, &p, public_key_len);
  opaque[PKEY] = pkey;
  if (!pkey)
    return;

  const EVP_MD *md = (hash_algorithm && *hash_algorithm) ? EVP_get_digestbyname(hash_algorithm) : EVP_sha256();
  if (!md)
    return;

  EVP_MD_CTX *ctx = EVP_MD_CTX_create();
  opaque[CTX] = ctx;
  if (!ctx)
    return;

  if (!EVP_VerifyInit(ctx, md))
    return;

  failed = false;
}

bool Checker::append(const void *buffer, unsigned buf_size)
{
  if (failed || done)
    return false;

  EVP_MD_CTX *ctx = (EVP_MD_CTX *)opaque[CTX];
  if (!EVP_VerifyUpdate(ctx, buffer, buf_size))
    failed = true;

  return !failed;
}

bool Checker::check(const unsigned char *signature, unsigned signature_size)
{
  if (failed || done)
    return false;

  EVP_PKEY *pkey = (EVP_PKEY *)opaque[PKEY];
  EVP_MD_CTX *ctx = (EVP_MD_CTX *)opaque[CTX];
  done = true;
  return EVP_VerifyFinal(ctx, signature, signature_size, pkey) == 1;
}

} // namespace digital_signature


bool verify_digital_signature_with_key_and_algo(const char *hash_algorithm, const void **buffers, const unsigned *buf_sizes,
  const unsigned char *signature, int signature_size, const unsigned char *public_key, int public_key_len)
{
  if (!buffers || !buf_sizes || !signature)
    return false;

  digital_signature::Checker checker(hash_algorithm, public_key, public_key_len);
  for (int i = 0; buffers[i]; ++i)
    if (!checker.append(buffers[i], buf_sizes[i]))
      return false;
  return checker.check(signature, signature_size);
}


bool verify_digital_signature_with_key(const void **buffers, const unsigned *buf_sizes, const unsigned char *signature,
  int signature_size, const unsigned char *public_key, int public_key_len)
{
  return verify_digital_signature_with_key_and_algo(nullptr, buffers, buf_sizes, signature, signature_size, public_key,
    public_key_len);
}
