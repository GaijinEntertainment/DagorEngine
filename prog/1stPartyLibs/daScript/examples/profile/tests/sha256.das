// options log_nodes=true

options solid_context

require math
require strings

include ../config.das

let
    primes = [[uint
        0x428a2f98; 0x71374491; 0xb5c0fbcf; 0xe9b5dba5;
        0x3956c25b; 0x59f111f1; 0x923f82a4; 0xab1c5ed5;
        0xd807aa98; 0x12835b01; 0x243185be; 0x550c7dc3;
        0x72be5d74; 0x80deb1fe; 0x9bdc06a7; 0xc19bf174;
        0xe49b69c1; 0xefbe4786; 0x0fc19dc6; 0x240ca1cc;
        0x2de92c6f; 0x4a7484aa; 0x5cb0a9dc; 0x76f988da;
        0x983e5152; 0xa831c66d; 0xb00327c8; 0xbf597fc7;
        0xc6e00bf3; 0xd5a79147; 0x06ca6351; 0x14292967;
        0x27b70a85; 0x2e1b2138; 0x4d2c6dfc; 0x53380d13;
        0x650a7354; 0x766a0abb; 0x81c2c92e; 0x92722c85;
        0xa2bfe8a1; 0xa81a664b; 0xc24b8b70; 0xc76c51a3;
        0xd192e819; 0xd6990624; 0xf40e3585; 0x106aa070;
        0x19a4c116; 0x1e376c08; 0x2748774c; 0x34b0bcb5;
        0x391c0cb3; 0x4ed8aa4a; 0x5b9cca4f; 0x682e6ff3;
        0x748f82ee; 0x78a5636f; 0x84c87814; 0x8cc70208;
        0x90befffa; 0xa4506ceb; 0xbef9a3f7; 0xc67178f2
    ]]

def toHex(hash:uint[8])
    let st = build_string() <| $(var writer)
        for i in hash
            format(writer,"%08x",i)
    return st

[unsafe_deref,hint(alwaysinline,unsafe_range_check,noalias=data,noalias=hash)]
def digestBlock(data:uint?; var hash:uint[8])
    var digest:uint[64]
    for j in range(16)
        unsafe
            digest[j] = data[j]
    for j in range(16,64)
        let v0 = digest[j-15]
        let s0 = (v0 >>> 7u) ^ (v0 >>> 18u) ^ (v0>>3u)
        let v1 = digest[j-2]
        let s1 = (v1 >>> 17u) ^ (v1 >>> 19u) ^ (v1>>10u)
        digest[j] = digest[j-16] + s0 + digest[j-7] + s1
    var a = hash[0]
    var b = hash[1]
    var c = hash[2]
    var d = hash[3]
    var e = hash[4]
    var f = hash[5]
    var g = hash[6]
    var h = hash[7]
    for i in range(64)
        let s0 = (a >>> 2u) ^ (a >>> 13u) ^ (a >>> 22u)
        let maj = (a&b) ^ (a&c) ^ (b&c)
        let t2 = s0 + maj
        let s1 = (e >>> 6u) ^ (e >>> 11u) ^ (e >>> 25u)
        let ch = (e & f) ^ ((~e) & g)
        let t1 = h + s1 + ch + primes[i] + digest[i]
        h = g
        g = f
        f = e
        e = d + t1
        d = c
        c = b
        b = a
        a = t1 + t2
    hash[0] += a
    hash[1] += b
    hash[2] += c
    hash[3] += d
    hash[4] += e
    hash[5] += f
    hash[6] += g
    hash[7] += h

[unsafe_deref,hint(noalias=msg)]
def sha256(msg:array<uint8>)
    var hash = [[uint
        0x6a09e667;
        0xbb67ae85;
        0x3c6ef372;
        0xa54ff53a;
        0x510e527f;
        0x9b05688c;
        0x1f83d9ab;
        0x5be0cd19
    ]]
    let len = length(msg) / 64
    for i in range(len)
        unsafe
            digestBlock(reinterpret<uint?>(addr(msg[i*64])),hash)
    return toHex(hash)

[export,no_jit,no_aot]
def main
    let input <- [{for i in range(1024); uint8('.')}]
    let dt = profile(20,"sha256") <|
        for i in range(1024)
            sha256(input)
    verify(sha256(input)=="8adcaee60bb05a9964a1df12d2f007adcb8f3fa20ff7d1ecfde0a2ac301ff412")
    print("\t{1.0/dt} mb/sec\n")
    if is_cpp_time()
        run_files([[TestFile
            luauFile = "sha256.luau",
            luaJitFile = "sha256.lua",
            dotNetFile = "sha256.cs",
        ]])
