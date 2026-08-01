This folder collects small, mostly single-file, vendored hashing/checksum/cipher
implementations used by the engine. sha1.cpp/sha1.h and sha256.cpp/sha256.h are
internal Dagor wrappers around OpenSSL, not vendored code.

Upstream sources:
- xxhash.c/xxhash.h/xxh3.h/xxh_x86dispatch*: xxHash, https://github.com/Cyan4973/xxHash, v0.8.3
- md5.c/md5.h: L. Peter Deutsch's MD5 (RFC 1321), see LICENSE
- crc32.c/crc32.h: Gary S. Brown's CRC-32, see LICENSE
- crc32c.c/crc32c.h/crc32c_data.h: Mark Adler's CRC-32C, https://github.com/madler/brotli/blob/master/crc32c.c
- rijndael-alg-fst.*/rijndael-api-fst.*: public-domain Rijndael/AES reference implementation
  by Rijmen, Bosselaers, Barreto
- murmur_hash.h: MurmurHash3, https://github.com/jwerle/murmurhash.c
- mum_hash.h: MUM hash, https://github.com/vnmakarov/mum-hash
- wyhash.h: wyhash, https://github.com/wangyi-fudan/wyhash
- rapidhash.h: rapidhash V3, https://github.com/Nicoshev/rapidhash
- sha3.cpp/sha3.h: internal wrapper around SHA3IUF (see SHA3IUF/README.md)

BLAKE3/ and SHA3IUF/ are separate vendored libraries with their own license/README;
see those subfolders.
