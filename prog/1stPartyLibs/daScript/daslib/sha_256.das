options gen2
options indenting = 4

module sha_256 shared public

require strings
require daslib/fio

//! FIPS 180-4 SHA-256.
//!
//! Content-addressing and integrity checks need a collision-resistant hash;
//! the builtin 64-bit ``hash`` is not one (collisions are trivially
//! computable). This module is pure daslang — it runs identically under
//! interpreter, JIT, AOT, and wasm.
//!
//! One-shot: ``sha256_hex("abc")`` or ``sha256_hex(bytes)`` → 64-char
//! lowercase hex. Streaming: ``sha256_init`` / ``sha256_update`` /
//! ``sha256_final`` for data that arrives in pieces.

let private K = fixed_array<uint>(
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
    0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
    0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
    0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
    0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
    0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
    0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
    0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
)

struct Sha256State {
    //! Streaming hash state. Create with ``sha256_init``; do not mutate directly.
    h : uint[8] //! Current hash values (eight 32-bit words).
    buf : uint8[64] //! Pending input block.
    bufLen : int //! Bytes buffered in the pending block.
    totalBytes : uint64 //! Total bytes absorbed so far.
}

def sha256_init : Sha256State {
    //! Fresh streaming state (FIPS 180-4 initial hash values).
    var st : Sha256State
    st.h = fixed_array<uint>(
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19)
    return st
}

def private rotr(x : uint; n : uint) : uint {
    return (x >> n) | (x << (32u - n))
}

def private compress(var st : Sha256State) {
    var w : uint[64]
    for (i in range(16)) {
        w[i] = ((uint(st.buf[i * 4]) << 24u)
            | (uint(st.buf[i * 4 + 1]) << 16u)
            | (uint(st.buf[i * 4 + 2]) << 8u)
            | uint(st.buf[i * 4 + 3]))
    }
    for (i in range(16, 64)) {
        let s0 = rotr(w[i - 15], 7u) ^ rotr(w[i - 15], 18u) ^ (w[i - 15] >> 3u)
        let s1 = rotr(w[i - 2], 17u) ^ rotr(w[i - 2], 19u) ^ (w[i - 2] >> 10u)
        w[i] = w[i - 16] + s0 + w[i - 7] + s1
    }
    var a = st.h[0]
    var b = st.h[1]
    var c = st.h[2]
    var d = st.h[3]
    var e = st.h[4]
    var f = st.h[5]
    var g = st.h[6]
    var hh = st.h[7]
    for (i in range(64)) {
        let s1 = rotr(e, 6u) ^ rotr(e, 11u) ^ rotr(e, 25u)
        let ch = (e & f) ^ (~e & g)
        let temp1 = hh + s1 + ch + K[i] + w[i]
        let s0 = rotr(a, 2u) ^ rotr(a, 13u) ^ rotr(a, 22u)
        let maj = (a & b) ^ (a & c) ^ (b & c)
        let temp2 = s0 + maj
        hh = g
        g = f
        f = e
        e = d + temp1
        d = c
        c = b
        b = a
        a = temp1 + temp2
    }
    st.h[0] += a
    st.h[1] += b
    st.h[2] += c
    st.h[3] += d
    st.h[4] += e
    st.h[5] += f
    st.h[6] += g
    st.h[7] += hh
}

def sha256_update(var st : Sha256State; b : uint8) {
    //! Absorb one byte.
    st.buf[st.bufLen] = b
    st.bufLen ++
    st.totalBytes ++
    if (st.bufLen == 64) {
        compress(st)
        st.bufLen = 0
    }
}

def sha256_update(var st : Sha256State; data : array<uint8> const# implicit) {
    //! Absorb a byte array (accepts temporary views — peek_data/fmap payloads).
    for (b in data) {
        sha256_update(st, b)
    }
}

def sha256_final(var st : Sha256State) : uint[8] {
    //! Apply FIPS 180-4 padding and return the digest as eight big-endian words.
    //! The state is consumed — reuse requires a fresh ``sha256_init``.
    let bitLen = st.totalBytes * 8ul
    sha256_update(st, uint8(0x80))
    while (st.bufLen != 56) {
        sha256_update(st, uint8(0))
    }
    for (shift in fixed_array<uint64>(56ul, 48ul, 40ul, 32ul, 24ul, 16ul, 8ul, 0ul)) {
        st.buf[st.bufLen] = uint8(bitLen >> shift)
        st.bufLen ++
    }
    compress(st)
    return st.h
}

def sha256_hex_digest(digest : uint[8]) : string {
    //! Digest words → 64-char lowercase hex.
    return build_string() $(writer) {
        for (word in digest) {
            fmt(writer, ":08x", word)
        }
    }
}

def sha256_hex(s : string) : string {
    //! SHA-256 of the string's bytes (no terminator), lowercase hex.
    var st = sha256_init()
    peek_data(s) $(bytes) {
        for (b in bytes) {
            sha256_update(st, b)
        }
    }
    return sha256_hex_digest(sha256_final(st))
}

def sha256_hex(data : array<uint8> const# implicit) : string {
    //! SHA-256 of a byte array, lowercase hex (accepts temporary views).
    var st = sha256_init()
    sha256_update(st, data)
    return sha256_hex_digest(sha256_final(st))
}

[temp_string_result]
def sha256_file_hex(path : string) : string {
    //! SHA-256 of a file's bytes, lowercase hex; "" when the file is
    //! unreadable or empty.
    var hex = ""
    fopen(path, "rb") $(f) {
        if (f != null) {
            fmap(f) $(var data : array<uint8>#) {
                hex = sha256_hex(data)
            }
        }
    }
    return hex
}
