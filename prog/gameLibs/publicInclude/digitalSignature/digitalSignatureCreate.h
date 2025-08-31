//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <generic/dag_tab.h>


namespace digital_signature
{

enum class KeyFormat
{
  DER,
  PEM
};

class Signer
{
public:
  Signer() = default;

  Signer(const char *hash_algorithm, const unsigned char *private_key, int private_key_len, const char *passphrase,
    KeyFormat key_fmt = KeyFormat::PEM);
  ~Signer();

  Signer(Signer &&other) noexcept;
  Signer &operator=(Signer &&other) noexcept;

  Signer(const Signer &) = delete;
  Signer &operator=(const Signer &) = delete;

  bool append(const void *buffer, unsigned buf_size);
  Tab<unsigned char> finalize();

private:
  void *opaquePkey = nullptr;
  void *opaqueCtx = nullptr;
  bool failed = true;
  bool done = false;
};

} // namespace digital_signature
