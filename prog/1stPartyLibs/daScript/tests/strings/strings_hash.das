options gen2
require dastest/testing_boost
require daslib/faker
require daslib/fuzzer
require strings

[test]
def test_hash_das_string(t : T?) {
    var h = 0ul
    var h2 = 0ul
    using() $(var st : das_string) {
        st := "hello"
        h = hash(st)
        h2 = hash(string(st))
    }
    t |> equal(h, h2)
}

[test]
def test_hash_builder(t : T?) {
    var fake <- Faker()
    fuzz() {
        let str1 = fake |> any_string
        if (str1 != "") {
            let strHash = hash(str1)
            let hashBuilder = build_hash() $(bldr) {
                bldr |> write(str1)
            }
            t |> equal(strHash, hashBuilder)
        }
    }
}


[test]
def test_hash_string_reference(t : T?) {
    // Pins hash(string) to known hash_blockz64 (FNV-1a) values across lengths and
    // parities, including the empty string (-> FNV basis). Under JIT this validates
    // the inlined hash intrinsic; under interp, the bound builtin. Both must agree.
    t |> equal(hash(""), 0xcbf29ce484222325ul)
    t |> equal(hash("a"), 0xaf63dc4c8601ec8cul)
    t |> equal(hash("ab"), 0xaf81dc4c8634e68cul)
    t |> equal(hash("abc"), 0x6e9045080be4681dul)
    t |> equal(hash("abcd"), 0x6e2c45080b3a7c1dul)
    t |> equal(hash("abcde"), 0x6fb5c4ab14617fe8ul)
    t |> equal(hash("hello world"), 0x408bf6d7388fe711ul)
    t |> equal(hash("this/is/a/fairly/long/identifier/alpha"), 0xa31f63a5100a123dul)
    t |> equal(hash("x"), 0xaf63f54c86021707ul)
    t |> equal(hash("key_0"), 0x63f98bc40fe415bbul)
    t |> equal(hash("key_15"), 0x64248ac4102d2508ul)
}

[test]
def test_hash_string_runtime(t : T?) {
    // Same pins as test_hash_string_reference, but keys are built at runtime and read
    // back through a heap array so the JIT hash intrinsic cannot const-fold them — this
    // exercises the actual 16-bit-load loop (not the folded immediate), bit-for-bit.
    var ks <- [for (i in range(16)); "key_{i}"]
    t |> equal(hash(ks[0]), 0x63f98bc40fe415bbul)    // "key_0"
    t |> equal(hash(ks[15]), 0x64248ac4102d2508ul)   // "key_15"
    delete ks
}

[test]
def test_hash_builder4(t : T?) {
    var fake <- Faker()
    fuzz() {
        let str1 = fake |> any_string
        let str2 = fake |> any_string
        let str3 = fake |> any_string
        let str4 = fake |> any_string
        if (str1 != "" && str2 != "" && str3 != "" && str4 != "") {
            let strHash = hash("{str1}{str2}{str3}{str4}")
            let hashBuilder = build_hash() $(bldr) {
                bldr |> write(str1)
                bldr |> write(str2)
                bldr |> write(str3)
                bldr |> write(str4)
            }
            t |> equal(strHash, hashBuilder)
        }
    }
}
