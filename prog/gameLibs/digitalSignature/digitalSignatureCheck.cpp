#include <openssl/evp.h>
#include <openssl/pem.h>
#include <digitalSignature/digitalSignatureCheck.h>


bool verify_digital_signature_with_key_and_algo(const char *hash_algorithm, const void **buffers, const unsigned *buf_sizes,
  const unsigned char *signature, int signature_size, const unsigned char *public_key, int public_key_len)
{
  if (!buffers || !buf_sizes || !signature)
    return false;

  const unsigned char *p = public_key;
  EVP_PKEY *pkey = d2i_PUBKEY(NULL, &p, public_key_len);
  if (!pkey)
    return false;

  bool verified = true;
  EVP_MD_CTX *ctx = EVP_MD_CTX_create();

  const EVP_MD *md = (hash_algorithm && *hash_algorithm) ? EVP_get_digestbyname(hash_algorithm) : EVP_sha256();

  if (md && EVP_VerifyInit(ctx, md))
  {
    for (int i = 0; buffers[i] && verified; ++i)
      verified = EVP_VerifyUpdate(ctx, buffers[i], buf_sizes[i]) != 0;
    if (verified)
      verified = EVP_VerifyFinal(ctx, signature, signature_size, pkey) == 1;
  }
  else
    verified = false;

  EVP_PKEY_free(pkey);
  EVP_MD_CTX_destroy(ctx);

  return verified;
}


bool verify_digital_signature_with_key(const void **buffers, const unsigned *buf_sizes, const unsigned char *signature,
  int signature_size, const unsigned char *public_key, int public_key_len)
{
  return verify_digital_signature_with_key_and_algo(nullptr, buffers, buf_sizes, signature, signature_size, public_key,
    public_key_len);
}
