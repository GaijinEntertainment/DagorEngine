options gen2
require dastest/testing_boost public
require daslib/base64
require daslib/faker
require daslib/fuzzer
require daslib/strings

// def public base64_decode ( _in:string ) : string
// def public base64_encode ( _inp:string )

[test]
def test_boundaries(t : T?) {
    let example = "hello, world"
    let reference = "aGVsbG8sIHdvcmxk"
    let encoded = base64_encode(example)
    let decoded = base64_decode(encoded).text
    t |> equal(encoded, reference)
    t |> equal(example, decoded)
    t |> run("faker base64 encode and decode") @@(t : T?) {
        var fake <- Faker()
        fuzz(100) {
            let fake_string = fake |> long_string
            let enc = base64_encode(fake_string)
            let dec = base64_decode(enc)
            t |> success(dec.size != -1, "decoding failed")
            t |> equal(fake_string, dec.text)
        }
    }
}

[test]
def test_unpadded_decode(t : T?) {
    let samples = fixed_array("T", "Th", "The", "The ", "The q", "The qu", "The qui", "The quic", "The quick")
    for (src in samples) {
        let unpadded = replace(base64_encode(src), "=", "")
        let dec = base64_decode(unpadded)
        t |> equal(dec.size, length(src), "size of unpadded '{unpadded}'")
        t |> equal(dec.text, src, "text of unpadded '{unpadded}'")
    }
    t |> equal(base64_decode("QUJDRA").text, "ABCD")
    t |> equal(base64_decode("QUJDREU").text, "ABCDE")
    t |> success(BASE64_DECODE_OUT_SIZE(6) >= 4, "6 unpadded chars carry 4 bytes")
    t |> success(BASE64_DECODE_OUT_SIZE(7) >= 5, "7 unpadded chars carry 5 bytes")
    t |> equal(BASE64_DECODE_OUT_SIZE(8), 6)
}

[test]
def test_decode_rejects_bad_input(t : T?) {
    var out : array<uint8>
    t |> equal(base64_decode("QQ*Q", out), -1)
    t |> equal(length(out), 0, "a rejected decode leaves no partial output")
}

[test]
def test_decode_url_safe_alphabet(t : T?) {
    t |> equal(base64_decode("PDw/Pz8+Pj4=").text, "<<???>>>")
    t |> equal(base64_decode("PDw_Pz8-Pj4").text, "<<???>>>", "- and _ are the URL-safe spelling of + and /")
    t |> equal(base64_decode("PDw_Pz8-Pj4=").text, "<<???>>>")
    t |> equal(base64_decode("eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9").text, "\{\"alg\":\"HS256\",\"typ\":\"JWT\"\}")
}

[test]
def test_decode_rejects_orphan_quantum(t : T?) {
    var out : array<uint8>
    t |> equal(base64_decode("Q", out), -1, "one leftover character cannot carry a byte")
    t |> equal(length(out), 0)
    t |> equal(base64_decode("QUJDR", out), -1)
    t |> equal(base64_decode("QUJDREUJQ", out), -1)
    t |> equal(base64_decode("QUJDR").size, -1)
    t |> equal(base64_decode("QUJD", out), 3, "a whole quantum still decodes")
    t |> equal(base64_decode("QUJDRA", out), 4, "so does a 2-over tail")
    t |> equal(base64_decode("QUJDREU", out), 5, "and a 3-over tail")
}

[test]
def test_decode_empty_input_clears_output(t : T?) {
    var out : array<uint8>
    t |> equal(base64_decode("QUJD", out), 3)
    t |> equal(length(out), 3)
    t |> equal(base64_decode("", out), 0)
    t |> equal(length(out), 0, "an empty input decodes to no bytes, not the previous call's")
    let dec = base64_decode("")
    t |> equal(dec.size, 0)
    t |> equal(dec.text, "")
}


