// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <digitalSignature/digitalSignatureCreate.h>
#include <utility>


namespace digital_signature
{

static int password_cb_stub(char * /*buf*/, int /*size*/, int /*rwflag*/, void * /*u*/) { return -1; }

Signer::Signer(Signer &&other) noexcept { *this = std::move(other); }

Signer &Signer::operator=(Signer &&other) noexcept
{
  opaquePkey = other.opaquePkey;
  opaqueCtx = other.opaqueCtx;
  failed = other.failed;
  done = other.done;

  other.opaquePkey = nullptr;
  other.opaqueCtx = nullptr;
  other.failed = true;
  other.done = true;

  return *this;
}

Signer::~Signer()
{
  EVP_PKEY *pkey = (EVP_PKEY *)opaquePkey;
  EVP_MD_CTX *ctx = (EVP_MD_CTX *)opaqueCtx;

  if (pkey)
    EVP_PKEY_free(pkey);
  if (ctx)
    EVP_MD_CTX_destroy(ctx);
}

Signer::Signer(const char *hash_algorithm, const unsigned char *private_key, int private_key_len, const char *passphrase,
  KeyFormat key_fmt)
{
  failed = true;

  BIO *memBuf = BIO_new_mem_buf(private_key, private_key_len);
  if (!memBuf)
    return;
  if (key_fmt == KeyFormat::PEM)
    opaquePkey = PEM_read_bio_PrivateKey(memBuf, NULL, passphrase ? NULL : &password_cb_stub, (void *)passphrase);
  else
    opaquePkey = d2i_PKCS8PrivateKey_bio(memBuf, NULL, passphrase ? NULL : &password_cb_stub, (void *)passphrase);
  BIO_free(memBuf);
  if (!opaquePkey)
    return;

  const EVP_MD *md = (hash_algorithm && *hash_algorithm) ? EVP_get_digestbyname(hash_algorithm) : EVP_sha256();
  if (!md)
    return;

  EVP_MD_CTX *ctx = EVP_MD_CTX_create();
  opaqueCtx = ctx;
  if (!ctx)
    return;

  if (!EVP_SignInit(ctx, md))
    return;

  failed = false;
}

bool Signer::append(const void *buffer, unsigned buf_size)
{
  if (failed || done)
    return false;

  EVP_MD_CTX *ctx = (EVP_MD_CTX *)opaqueCtx;
  if (!EVP_SignUpdate(ctx, buffer, buf_size))
    failed = true;

  return !failed;
}

Tab<unsigned char> Signer::finalize()
{
  Tab<unsigned char> result;
  if (failed || done)
    return result;

  EVP_PKEY *pkey = (EVP_PKEY *)opaquePkey;
  const int maxSize = EVP_PKEY_size(pkey);
  if (maxSize <= 0)
  {
    failed = true;
    return result;
  }

  result.resize(maxSize);
  EVP_MD_CTX *ctx = (EVP_MD_CTX *)opaqueCtx;
  unsigned actualSize = 0;
  if (!EVP_SignFinal(ctx, result.data(), &actualSize, pkey))
  {
    failed = true;
    return {};
  }

  done = true;
  result.resize(actualSize);
  return result;
}

} // namespace digital_signature
