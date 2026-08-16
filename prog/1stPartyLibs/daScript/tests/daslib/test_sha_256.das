options gen2

require dastest/testing_boost public
require daslib/sha_256
require strings

[test]
def test_nist_vectors(t : T?) {
    t |> run("empty string") @(t : T?) {
        t |> equal(sha256_hex(""), "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855")
    }
    t |> run("abc") @(t : T?) {
        t |> equal(sha256_hex("abc"), "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad")
    }
    t |> run("two-block 448-bit message") @(t : T?) {
        t |> equal(sha256_hex("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"),
            "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1")
    }
    t |> run("four-block 896-bit message") @(t : T?) {
        t |> equal(sha256_hex("abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu"),
            "cf5b16a778af8380036ce59e7b0492370b249b11e8f07a51afac45037afee9d1")
    }
    t |> run("one million a") @(t : T?) {
        var st = sha256_init()
        for (_i in range(1000000)) {
            sha256_update(st, uint8('a'))
        }
        t |> equal(sha256_hex_digest(sha256_final(st)),
            "cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0")
    }
}

[test]
def test_padding_boundaries(t : T?) {
    // These lengths straddle the padding split (0x80 + length must fit or spill
    // into a second block). Digests come from an INDEPENDENT SHA-256: the two
    // overloads share one per-byte path, so comparing them cannot catch this.
    t |> run("digests at the padding boundaries match known values") @(tt : T?) {
        for (c in fixed_array(
                (55, "d5e285683cd4efc02d021a5c62014694958901005d6f71e89e0989fac77e4072"),
                (56, "04c26261370ee7541549d16dee320c723e3fd14671e66a099afe0a377c16888e"),
                (63, "75220b47218278e656f2013bb8f0c455a25eaf01e86c64924e9d48d89776d6f2"),
                (64, "7ce100971f64e7001e8fe5a51973ecdfe1ced42befe7ee8d5fd6219506b5393c"),
                (65, "9537c5fdf120482f7d58d25e9ed583f52c02b4e304ea814db1633ad565aed7e9"),
                (119, "000b48d4edf0fa7bee3c6236ecd2785baa5db4eeb8bb54341b029e0d9fa5fb0c"),
                (120, "13f05a0b594787f5ecd315edc96141bd3243203d1b7d4f0836f37308b276ba98"))) {
            var bytes : array<uint8>
            var str = ""
            for (_i in range(c._0)) {
                bytes |> push(uint8('x'))
                str += "x"
            }
            tt |> equal(sha256_hex(str), c._1)
            tt |> equal(sha256_hex(bytes), c._1)
            delete bytes
        }
    }
}

[test]
def test_streaming_equals_oneshot(t : T?) {
    t |> run("split updates match one-shot") @(t : T?) {
        let msg = "the quick brown fox jumps over the lazy dog"
        var st = sha256_init()
        peek_data(msg) $(bytes) {
            for (b in bytes) {
                sha256_update(st, b)
            }
        }
        t |> equal(sha256_hex_digest(sha256_final(st)), sha256_hex(msg))
    }
}
