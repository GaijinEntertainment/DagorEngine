//
// Dagor Tech 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

// calculates hash for a string "key"
unsigned get_hash(const char *key, unsigned int init_val);

unsigned get_crc32(const char *s);
bool get_file_crc32(const char *path, unsigned &crc);


class IGenSave;
enum
{
  HASH_SAVECB_CRC32,
  HASH_SAVECB_MD5,
  HASH_SAVECB_SHA1
};

//! creates "write stream callback" designed for hash computation; only IGenSave::write is usable
IGenSave *create_hash_computer_cb(int hash_savecb_type, IGenSave *copy_cwr = NULL);
//! copies current computed hash to specified buffer, optionally resets structures for new hash computations; returns size of hash
int get_computed_hash(IGenSave *hash_cb, void *out_data, int out_data_sz, bool reset = true);
//! deallocates data allocated in create_hash_computer_cb()
void destory_hash_computer_cb(IGenSave *hash_cb);
