// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <crypto/cipher.h>
#include <openssl/opensslv.h>
#include <openssl/aes.h>
#include <openssl/evp.h>
#include <openssl/modes.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/string.h>
#include <EASTL/string_view.h>
#include <debug/dag_assert.h>
#include "bcryptUtils.h"

#if DAGOR_DBGLEVEL > 1 || _TARGET_PC_WIN
#define DBG_ASSERTF G_ASSERTF
#else
#define DBG_ASSERTF(...) ((void)0)
#endif

#if OPENSSL_VERSION_NUMBER >= 0x10100000
#define EVP_CIPHER_CTX_OPAQUE 1
#endif

G_STATIC_ASSERT((int)crypto::ENCRYPT == AES_ENCRYPT);
G_STATIC_ASSERT((int)crypto::DECRYPT == AES_DECRYPT);

namespace crypto
{

#if USE_BCRYPT

struct BCryptAlgCloser
{
  typedef BCRYPT_ALG_HANDLE pointer;
  void operator()(BCRYPT_ALG_HANDLE handle)
  {
    if (handle)
      BCryptCloseAlgorithmProvider(handle, 0);
  }
};
struct BCryptKeyDestroyer
{
  typedef BCRYPT_KEY_HANDLE pointer;
  void operator()(BCRYPT_KEY_HANDLE handle)
  {
    if (handle)
      BCryptDestroyKey(handle);
  }
};

template <int KEY_SIZE, int BLOCK_SIZE,
  void mode_f(const uint8_t *in, uint8_t *out, size_t len, const void *key, uint8_t *ivec, int *num, int enc, block128_f block)>
class AESBCryptOpenSSLModeCipher final : public ISymmetricCipher // Block function is BCrypt, but mode function is OpenSSL
{
  eastl::unique_ptr<void, BCryptAlgCloser> aesAlg;
  eastl::unique_ptr<void, BCryptKeyDestroyer> aesKey;
  uint8_t key[KEY_SIZE];
  uint8_t ivec[BLOCK_SIZE] = {0};
  int num = 0;
  EncryptionDir enc = crypto::ENCRYPT;

public:
  AESBCryptOpenSSLModeCipher()
  {
    BCRYPT_ALG_HANDLE hAlg = nullptr;
    BCRYPT_DO(BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_AES_ALGORITHM, NULL, 0));
    aesAlg.reset(hAlg);
  }

  static void __cdecl bcrypt_block_f(const uint8_t *in, uint8_t *out, const void *ptr)
  {
    const auto self = (const AESBCryptOpenSSLModeCipher *)ptr;
    ULONG outlen = 0;
    BCRYPT_DO(BCryptEncrypt(self->aesKey.get(), (uint8_t *)in, BLOCK_SIZE, NULL, NULL, 0, out, BLOCK_SIZE, &outlen, 0));
    G_ASSERT(outlen == BLOCK_SIZE);
  }

  void setup(dag::ConstSpan<uint8_t> in_key, dag::ConstSpan<uint8_t> in_iv, EncryptionDir edir) override
  {
    G_ASSERT(!in_key.empty() || !in_iv.empty());
    if (!in_key.empty() && (!aesKey || memcmp(key, in_key.data(), sizeof(key)) != 0))
    {
      G_ASSERT(data_size(in_key) >= sizeof(key));
      memcpy(key, in_key.data(), sizeof(key));
      BCRYPT_KEY_HANDLE hKey = nullptr;
      BCRYPT_DO(BCryptGenerateSymmetricKey(aesAlg.get(), &hKey, nullptr, 0, key, sizeof(key), 0));
      aesKey.reset(hKey);
      // openssl modes are used
      BCRYPT_DO(BCryptSetProperty(aesKey.get(), BCRYPT_CHAINING_MODE, (PBYTE)BCRYPT_CHAIN_MODE_ECB, sizeof(BCRYPT_CHAIN_MODE_ECB), 0));
    }
    if (!in_iv.empty())
    {
      G_ASSERT(data_size(in_iv) >= sizeof(ivec));
      memcpy(ivec, in_iv.data(), sizeof(ivec));
    }
    enc = edir;
    num = 0;
  }

  void update(uint8_t *out, int *out_len, const uint8_t *in, int in_len) override
  {
    G_ASSERT(aesKey);
    mode_f(in, out, in_len, this, ivec, &num, (int)enc, bcrypt_block_f);
    *out_len = in_len;
  }

  bool finalize(uint8_t *, int *out_len) override
  {
    *out_len = 0;
    return true;
  }
};

template <int KEY_SIZE, int BLOCK_SIZE>
class AESBCryptBlockCipher final : public ISymmetricCipher // Full BCrypt cipher (padding implemented manually as BCrypt doesn't
                                                           // support partial updates)
{
  static constexpr int BLOCK_MASK = BLOCK_SIZE - 1;
  eastl::unique_ptr<void, BCryptAlgCloser> aesAlg;
  eastl::unique_ptr<void, BCryptKeyDestroyer> aesKey;
  const eastl::wstring_view chainMode;
  uint8_t key[KEY_SIZE];
  EncryptionDir enc = crypto::ENCRYPT;
  uint8_t ivec[BLOCK_SIZE] = {0};
  uint8_t padBuf[BLOCK_SIZE];
  int padBufLen = 0;

public:
  AESBCryptBlockCipher(eastl::wstring_view chain_mode) : chainMode(chain_mode)
  {
    BCRYPT_ALG_HANDLE hAlg = nullptr;
    BCRYPT_DO(BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_AES_ALGORITHM, NULL, 0));
    aesAlg.reset(hAlg);

#if DAGOR_DBGLEVEL > 0
    DWORD blockLen = 0, _1;
    BCRYPT_DO(BCryptGetProperty(hAlg, BCRYPT_BLOCK_LENGTH, (PBYTE)&blockLen, sizeof(blockLen), &_1, 0));
    G_ASSERT(blockLen == BLOCK_SIZE);
#endif
  }

  void setup(dag::ConstSpan<uint8_t> in_key, dag::ConstSpan<uint8_t> in_iv, EncryptionDir edir) override
  {
    G_ASSERT(!in_key.empty() || !in_iv.empty());
    if (!in_key.empty() && (!aesKey || memcmp(key, in_key.data(), sizeof(key)) != 0))
    {
      G_ASSERT(data_size(in_key) >= sizeof(key));
      memcpy(key, in_key.data(), sizeof(key));
      BCRYPT_KEY_HANDLE hKey = nullptr;
      BCRYPT_DO(BCryptGenerateSymmetricKey(aesAlg.get(), &hKey, nullptr, 0, key, sizeof(key), 0));
      aesKey.reset(hKey);
      BCRYPT_DO(
        BCryptSetProperty(aesKey.get(), BCRYPT_CHAINING_MODE, (PBYTE)chainMode.data(), sizeof(wchar_t) * (chainMode.size() + 1), 0));
    }
    if (!in_iv.empty())
    {
      G_ASSERT(data_size(in_iv) >= sizeof(ivec));
      memcpy(ivec, in_iv.data(), sizeof(ivec));
    }
    enc = edir;
    padBufLen = 0;
  }

  int doCipher(uint8_t *out, const uint8_t *in, int in_len)
  {
    ULONG encrypted;
    BCRYPT_DO(((enc == ENCRYPT) ? BCryptEncrypt : BCryptDecrypt)(aesKey.get(), (uint8_t *)in, in_len, NULL, ivec, BLOCK_SIZE, out,
      in_len, &encrypted, 0));
    G_ASSERT(encrypted == in_len);
    return (int)encrypted;
  }

  void doEncrypt(uint8_t *out, int *out_len, const uint8_t *in, int in_len)
  {
    if (padBufLen == 0 && (in_len & BLOCK_MASK) == 0)
    {
      *out_len = doCipher(out, in, in_len);
      return;
    }

    if (padBufLen)
    {
      if (padBufLen + in_len < BLOCK_SIZE)
      {
        memcpy(padBuf + padBufLen, in, in_len);
        padBufLen += in_len;
        *out_len = 0;
        return;
      }
      else
      {
        int addInPad = BLOCK_SIZE - padBufLen;
        memcpy(padBuf + padBufLen, in, addInPad);
        *out_len = doCipher(out, padBuf, BLOCK_SIZE);
        in_len -= addInPad;
        in += addInPad;
        out += BLOCK_SIZE;
      }
    }
    else
      *out_len = 0;

    int plen = in_len & BLOCK_MASK;
    in_len -= plen;
    if (in_len > 0)
      *out_len += doCipher(out, in, in_len);

    if (plen)
      memcpy(padBuf, in + in_len, plen);
    padBufLen = plen;
  }

  void update(uint8_t *out, int *out_len, const uint8_t *in, int in_len) override
  {
    G_ASSERT(aesKey); // setup wasn't called?
    doEncrypt(out, out_len, in, in_len);
    if (enc == ENCRYPT)
      return;
    else // TODO: implement decrypting in several calls
    {
      uint8_t n = out[*out_len - 1];
      G_ASSERT(n <= BLOCK_SIZE);
      *out_len -= n;
    }
  }

  bool finalize(uint8_t *out, int *out_len) override
  {
    if (enc == ENCRYPT)
    {
      int n = BLOCK_SIZE - padBufLen;
      for (int i = padBufLen; i < BLOCK_SIZE; ++i)
        padBuf[i] = n;
      *out_len = doCipher(out, padBuf, BLOCK_SIZE);
    }
    else
    {
      G_ASSERT(!padBufLen); // TODO: implement decrypting in several calls
      *out_len = 0;
    }
    return true;
  }
};

#endif // USE_BCRYPT

struct OpensslCipherCtxDeleter
{
  void operator()(EVP_CIPHER_CTX *ctx) { EVP_CIPHER_CTX_free(ctx); }
};

template <const EVP_CIPHER *cipher_fn()>
class AESOpenSSLCipher final : public ISymmetricCipher
{
#ifdef EVP_CIPHER_CTX_OPAQUE
  eastl::unique_ptr<EVP_CIPHER_CTX, OpensslCipherCtxDeleter> cipherCtx;
  EVP_CIPHER_CTX *getCipherCtx() { return cipherCtx.get(); }
#else
  EVP_CIPHER_CTX cipherCtx;
  EVP_CIPHER_CTX *getCipherCtx() { return &cipherCtx; }
#endif
public:
  AESOpenSSLCipher()
#ifdef EVP_CIPHER_CTX_OPAQUE
    :
    cipherCtx(EVP_CIPHER_CTX_new())
#endif
  {
    G_VERIFY(EVP_EncryptInit(getCipherCtx(), cipher_fn(), /*key*/ NULL, /*iv*/ NULL));
  }

#ifndef EVP_CIPHER_CTX_OPAQUE
  ~AESOpenSSLCipher() { EVP_CIPHER_CTX_cleanup(getCipherCtx()); }
#endif

  void setup(dag::ConstSpan<uint8_t> key, dag::ConstSpan<uint8_t> iv, EncryptionDir edir) override
  {
    G_ASSERT(!key.empty() || !iv.empty());
    DBG_ASSERTF(key.empty() || data_size(key) >= EVP_CIPHER_CTX_key_length(getCipherCtx()), "key size (%d) shall be >= %d!",
      data_size(key), EVP_CIPHER_CTX_key_length(getCipherCtx()));
    DBG_ASSERTF(iv.empty() || data_size(iv) >= EVP_CIPHER_CTX_iv_length(getCipherCtx()), "iv size (%d) shall be >= %d!", data_size(iv),
      EVP_CIPHER_CTX_iv_length(getCipherCtx()));
    G_VERIFY(EVP_CipherInit(getCipherCtx(), /*cipher*/ NULL, key.empty() ? nullptr : key.data(), iv.empty() ? nullptr : iv.data(),
      (int)edir));
  }

  void update(uint8_t *out, int *out_len, const uint8_t *in, int in_len) override
  {
    G_VERIFY(EVP_CipherUpdate(getCipherCtx(), out, out_len, in, in_len));
  }

  bool finalize(uint8_t *out, int *out_len) override { return EVP_CipherFinal(getCipherCtx(), out, out_len); }

  void getAuthTag(dag::Span<uint8_t> out_auth_tag) override
  {
    G_VERIFY(EVP_CIPHER_CTX_ctrl(getCipherCtx(), EVP_CTRL_GCM_GET_TAG, out_auth_tag.size(), out_auth_tag.data()));
  }

  void setAuthTag(dag::ConstSpan<uint8_t> in_auth_tag) override
  {
    G_VERIFY(EVP_CIPHER_CTX_ctrl(getCipherCtx(), EVP_CTRL_GCM_SET_TAG, in_auth_tag.size(), (void *)in_auth_tag.data()));
  }
};

ISymmetricCipher *create_cipher_aes(Mode mode, KeySize ks)
{
  switch (ks)
  {
    case KeySize::_128:
    {
      switch (mode)
      {
#if USE_BCRYPT
        case Mode::CFB: return new AESBCryptOpenSSLModeCipher<128 / 8, 16, CRYPTO_cfb128_encrypt>();
        case Mode::CBC: return new AESBCryptBlockCipher<128 / 8, 16>(BCRYPT_CHAIN_MODE_CBC);
#else
        case Mode::CFB: return new AESOpenSSLCipher<EVP_aes_128_cfb>();
        case Mode::CBC: return new AESOpenSSLCipher<EVP_aes_128_cbc>();
#endif
        case Mode::GCM: return new AESOpenSSLCipher<EVP_aes_128_gcm>();
      }
    }
  }
  G_ASSERT(0); // shouldn't be here
  return nullptr;
}

} // namespace crypto
