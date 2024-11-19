//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

bool verify_digital_signature_with_key(const void **buffers, const unsigned *buf_sizes, const unsigned char *signature,
  int signature_size, const unsigned char *public_key, int public_key_len);

bool verify_digital_signature_with_key_and_algo(const char *hash_algorithm, const void **buffers, const unsigned *buf_sizes,
  const unsigned char *signature, int signature_size, const unsigned char *public_key, int public_key_len);


namespace digital_signature
{

class Checker
{
public:
  Checker(const char *hash_algorithm, const unsigned char *public_key, int public_key_len);
  ~Checker();

  Checker(Checker &&other);
  Checker &operator=(Checker &&other);

  Checker(const Checker &) = delete;
  Checker &operator=(const Checker &) = delete;

  bool append(const void *buffer, unsigned buf_size);
  bool check(const unsigned char *signature, unsigned signature_size);

private:
  void *opaque[2] = {nullptr};
  bool failed = false;
  bool done = false;
};

} // namespace digital_signature
