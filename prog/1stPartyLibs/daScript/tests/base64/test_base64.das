options gen2
require dastest/testing_boost public
require daslib/base64
require daslib/faker
require daslib/fuzzer

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


