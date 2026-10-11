options gen2

require dastest/testing_boost public

def private byte_at(s : string; i : int) : int {
    unsafe {
        let p = reinterpret<uint8?>(s)
        return int(p[i])
    }
}

def private bytes_of(s : string) : int4 {
    return int4(length(s), byte_at(s, 1), byte_at(s, 2), byte_at(s, 3))
}

def private hex_escape() : int4 {
    return bytes_of("a\x00b")
}

def private unicode_escape() : int4 {
    return bytes_of("a\u0000b")
}

def private null_then_hex_digit() : int2 {
    let s = "\x00a"
    return int2(byte_at(s, 0), byte_at(s, 1))
}

def private returned_blob() : string {
    return "q\x00r"
}

def private blob_in_builder() : string {
    return "{"a\x00b"}tail"
}

let private g_blob = "g\x00h"

def private blob_key_lookups() : int3 {
    let tab <- { "k\x00a" => 7 }
    return int3(tab?["k\x00a"] ?? 0, tab?["k\x00b"] ?? 0, tab?["k"] ?? 0)
}

def private same_blob_twice() : int2 {
    let first = "d\x00e"
    let second = "d\x00e"
    return int2(byte_at(first, 2), byte_at(second, 2))
}

def private passed_blob(s : string) : int {
    return byte_at(s, 2)
}

def private blobs_compare() : bool {
    return "c\x00x" == "c\x00y"
}

[test]
def test_null_constant_handling(t : T?) {
    t |> run("a table key reads a blob up to its first null, the constant key and the lookup alike") @(t : T?) {
        t |> equal(blob_key_lookups(), int3(7, 7, 7))
    }
    t |> run("each copy of a repeated blob constant keeps its bytes") @(t : T?) {
        t |> equal(same_blob_twice(), int2('e', 'e'))
    }
    t |> run("a global blob keeps its bytes") @(t : T?) {
        t |> equal(bytes_of(g_blob).yz, int2(0, 'h'))
    }
    t |> run("a blob passed as an argument keeps its bytes") @(t : T?) {
        t |> equal(passed_blob("p\x00q"), 'q')
    }
    t |> run("blobs compare up to their first null") @(t : T?) {
        t |> success(blobs_compare())
    }
}

[test]
def test_null_escapes(t : T?) {
    t |> run("\\x00 and \\u0000 each embed one null byte") @(t : T?) {
        let want = int4(1, 0, 'b', 0)
        t |> equal(hex_escape(), want)
        t |> equal(unicode_escape(), want)
    }
    t |> run("\\x00 reads two hex digits, a hex digit after it stays") @(t : T?) {
        t |> equal(null_then_hex_digit(), int2(0, 'a'))
    }
    t |> run("a returned constant keeps the bytes past the null") @(t : T?) {
        t |> equal(bytes_of(returned_blob()).yz, int2(0, 'r'))
    }
}

[test]
def test_null_in_builder(t : T?) {
    t |> run("a builder reads a blob element up to its first null") @(t : T?) {
        t |> equal(blob_in_builder(), "atail")
    }
}
