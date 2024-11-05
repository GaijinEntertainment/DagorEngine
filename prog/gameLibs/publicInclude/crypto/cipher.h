//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <stdint.h>
#include <generic/dag_span.h>

namespace crypto
{

enum class Mode // https://en.wikipedia.org/wiki/Block_cipher_mode_of_operation
{
  CFB, // Cipher feedback (stream)
  CBC, // Cipher block chaining (block)
  GCM  // Galois/counter (stream)
};

enum class KeySize // In bits
{
  _128,
};

enum EncryptionDir
{
  DECRYPT,
  ENCRYPT
};

#ifdef _MSC_VER
#define ABSTRACT_INTERFACE class __declspec(novtable)
#else
#define ABSTRACT_INTERFACE class
#endif

ABSTRACT_INTERFACE ISymmetricCipher
{
public:
  virtual ~ISymmetricCipher() = default;
  virtual void setup(dag::ConstSpan<uint8_t> key, dag::ConstSpan<uint8_t> iv, EncryptionDir edir) = 0;
  virtual void update(uint8_t * out, int *out_len, const uint8_t *in, int in_len) = 0;
  virtual bool finalize(uint8_t * out, int *out_len) = 0;
  // Only for some modes. Authentication tag is also known as Message authentication code (MAC)
  virtual void getAuthTag(dag::Span<uint8_t> /*out_auth_tag*/) {}
  virtual void setAuthTag(dag::ConstSpan<uint8_t> /*in_auth_tag*/) {}
};
#undef ABSTRACT_INTERFACE

ISymmetricCipher *create_cipher_aes(Mode mode, KeySize ks = KeySize::_128); // also known as "rijndael"

} // namespace crypto
