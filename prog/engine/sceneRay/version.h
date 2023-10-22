#pragma once
#define LEGACY_VERSION                 42
#define CURRENT_VERSION                43
#define ZSTD_DEFAULT_COMPRESSION_LEVEL 9
enum FrtCompression
{
  ZlibCompression = 6,
  NoCompression = 7,
  ZstdCompression = 0
};

inline bool is_supported_compression(FrtCompression c) { return c == NoCompression || c == ZlibCompression || c == ZstdCompression; }

inline uint16_t get_full_version(FrtCompression c) { return (CURRENT_VERSION << 3) | uint16_t(c); }
