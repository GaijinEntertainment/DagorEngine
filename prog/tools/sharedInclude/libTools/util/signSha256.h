//
// Dagor Tech 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

// requires AddIncludes += $(Root)/prog/3rdPartyLibs/openssl/include ;
// requires UseProgLibs += 3rdPartyLibs/openssl ; (and sometimes 3rdPartyLibs/arc/zlib-1.2.7)

#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <ioSys/dag_fileIo.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_direct.h>
#include <libTools/util/iLogWriter.h>

static inline bool add_digital_signature(const char *fn, const char *private_key, ILogWriter &log)
{
  bool retval = true;
  unsigned char signature[1024];
  EVP_MD_CTX *ctx = NULL;
  EVP_PKEY *pkey = NULL;
  if (!private_key || !*private_key)
    return true;
  FILE *fp = NULL;
  int readed = 0, sign_size = 0;
  unsigned real_sign_size = 0;
  FullFileLoadCB crd(fn);

  if (!crd.fileHandle)
    goto done;
  fp = fopen(private_key, "r");
  if (!fp)
  {
    log.addMessage(log.ERROR, "can't open private key '%s'", private_key);
    retval = false;
    goto done;
  }
  pkey = PEM_read_PrivateKey(fp, NULL, NULL, NULL);
  if (!pkey)
  {
    log.addMessage(log.ERROR, "error reading private key from '%s'", private_key);
    ERR_print_errors_fp(stdout);
    retval = false;
    goto done;
  }
  fclose(fp);
  fp = NULL;

  if (sizeof(signature) < EVP_PKEY_size(pkey))
  {
    log.addMessage(log.ERROR, "EVP_PKEY_size=%d is too big", EVP_PKEY_size(pkey));
    retval = false;
    goto done;
  }
  memset(signature, 0, sizeof(signature));
  ctx = EVP_MD_CTX_create();
  if (EVP_SignInit(ctx, EVP_sha256()))
  {
    unsigned char buf[4096];
    while (int sz = crd.tryRead(buf, sizeof(buf)))
      if (!EVP_SignUpdate(ctx, buf, sz))
        goto sign_fail;
    const char *fileName = dd_get_fname(fn);
    if (!fileName || !*fileName)
    {
      log.addMessage(log.ERROR, "could not determine vrom file name, infname: '%s'", fn);
      retval = false;
      goto done;
    }
    if (!EVP_SignFinal(ctx, signature, &real_sign_size, pkey))
      goto sign_fail;
    crd.close();

    FullFileSaveCB cwr(fn, DF_WRITE | DF_APPEND);
    cwr.write(signature, real_sign_size);
    cwr.close();
    log.addMessage(log.NOTE, "written digital signature '%s' of size %d; file name: '%s'", "sha256", real_sign_size, fn);
  }
  else
  {
  sign_fail:
    log.addMessage(log.ERROR, "failed to sign %s", fn);
    retval = false;
    ERR_print_errors_fp(stdout);
  }
done:
  if (ctx)
    EVP_MD_CTX_destroy(ctx);
  if (pkey)
    EVP_PKEY_free(pkey);
  if (fp)
    fclose(fp);
  return retval;
}

static inline bool verify_digital_signature_mem(const void *data, int data_len, const unsigned char *signature, int signature_size,
  const unsigned char *public_key, int public_key_len)
{
  const unsigned char *p = public_key;
  EVP_PKEY *pkey = d2i_PUBKEY(NULL, &p, public_key_len);
  if (!pkey)
    return false;

  EVP_MD_CTX *ctx = EVP_MD_CTX_create();
  bool verified = true;
  if (EVP_VerifyInit(ctx, EVP_sha256()))
  {
    verified = EVP_VerifyUpdate(ctx, data, data_len) != 0;
    if (verified)
      verified = EVP_VerifyFinal(ctx, signature, signature_size, pkey) == 1;
  }
  else
    verified = false;

  EVP_PKEY_free(pkey);
  EVP_MD_CTX_destroy(ctx);

  return verified;
}

static inline bool verify_digital_signature(const char *fn, const char *public_key, ILogWriter &log)
{
  Tab<unsigned char> key_data;
  if (strcmp(public_key, "*gaijin") == 0)
  {
    static const unsigned char public_key_gaijin[] =
      "\x30\x82\x01\x20\x30\x0d\x06\x09\x2a\x86\x48\x86\xf7\x0d\x01\x01\x01\x05\x00\x03\x82\x01\x0d\x00\x30\x82\x01\x08\x02\x82\x01"
      "\x01\x00\xcc\xa6\x0b\xdb\x1d\x6e\xeb\x16\x09\x8e\x88\x0f\xa8\x1c\x8f\x7b\x89\x8b\x9b\xbe\xb4\x5f\x14\x75\xd6\x28\xa6\xae\x84"
      "\x95\x38\x2e\xb6\xfe\x92\xc9\x72\xca\x0c\x82\xed\x02\xfc\x56\x79\xc8\x07\xd5\x9c\xcc\x89\x57\xe9\x27\xe0\xdd\xd8\x79\x4c\x38"
      "\x12\x43\x15\xe2\x10\x97\x93\x59\x0c\xbe\xe1\xe8\xde\x64\xbc\x59\x6f\x8e\x03\x70\x18\x31\x0e\xc2\xcb\x91\xa8\x95\x65\x2d\xc7"
      "\xff\xc0\xe9\xdf\x12\x26\x87\xa4\x18\xfe\x7d\xa1\x79\x01\x7c\xca\x6f\x5f\x0a\x91\x44\x2a\xb4\x83\xc0\xc5\x13\x7b\xa7\x32\x8e"
      "\xb8\xae\x5e\x07\x2a\x25\xbc\x1c\x37\xff\x09\xc3\x79\xe4\x8d\x39\x50\x3b\x0e\xbb\x03\x19\xd4\x3c\x77\x93\xb2\xd5\xeb\x4c\xda"
      "\x00\xe4\x07\x97\x3a\x11\x57\x23\xf3\x8e\xc1\x5f\x64\x6e\x8c\xd9\xf2\xd2\x22\x7a\x57\x5e\xc3\x08\x2d\xc3\xf2\x37\xdd\x4a\x40"
      "\x4e\x82\x16\x91\x9d\x9e\x96\x1d\x42\x97\xe2\x4f\xe1\x40\x04\xf9\x16\x92\xb9\x93\xdd\x8b\x60\x0b\x62\x30\xc1\xe7\xd6\xef\x22"
      "\xc4\x51\x34\xc5\xe6\xf9\xf2\x1b\x32\x26\xc0\xed\xe9\xe6\x68\x6f\x21\x9b\x39\xf5\xd2\xda\x37\x96\xeb\xb2\x87\xa5\x42\x78\xc0"
      "\x31\x36\x4a\xdf\x85\x3a\x46\xf5\x81\x2b\x02\x01\x03";
    key_data = make_span_const(public_key_gaijin, sizeof(public_key_gaijin) - 1);
  }
  else if (file_ptr_t fp = df_open(public_key, DF_READ | DF_IGNORE_MISSING))
  {
    key_data.resize(df_length(fp));
    df_read(fp, key_data.data(), data_size(key_data));
    df_close(fp);
  }
  else
  {
    log.addMessage(log.ERROR, "failed to open public key %s", public_key);
    return false;
  }

  FullFileLoadCB crd(fn, DF_READ | DF_IGNORE_MISSING);
  if (crd.fileHandle)
  {
    int flen = 0;
    if (const void *fdata = df_mmap(crd.fileHandle, &flen))
    {
      static const int SHA256_SIG_SZ = 256;
      const void *buf[] = {fdata, NULL};
      unsigned buf_sz[] = {unsigned(flen) - SHA256_SIG_SZ, 0};
      bool sign_ok = verify_digital_signature_mem(fdata, flen - SHA256_SIG_SZ, flen - SHA256_SIG_SZ + (unsigned char *)fdata,
        SHA256_SIG_SZ, key_data.data(), data_size(key_data));
      df_unmap(fdata, flen);
      if (sign_ok)
        return true;
    }
  }
  else
    log.addMessage(log.ERROR, "failed to open %s", fn);
  return false;
}
