options gen2
options no_unused_block_arguments = false
options no_unused_function_arguments = false
require dastest/testing_boost public
require daslib/json_boost
require strings

def utf8_bytes(s : string) : string {
    return build_string() $(var w) {
        peek_data(s) $(raw) {
            var first = true
            for (b in raw) {
                if (!first) {
                    write_char(w, ' ')
                }
                first = false
                write(w, int(b))
            }
        }
    }
}

def decode(t : T?; text : string) : string {
    var error : string
    var js = read_json(text, error)
    t |> success(js != null, "parsed {text}")
    if (js == null) {
        t |> failure("{text}: {error}")
        return ""
    }
    t |> success(js is _string, "decoded a string")
    return js as _string
}

[test]
def test_surrogate_pairs(t : T?) {
    t |> run("emoji pair decodes to one 4-byte code point") @(t : T?) {
        let s = decode(t, "\"\\ud83d\\ude00\"")
        t |> equal(utf8_bytes(s), "240 159 152 128")
        t |> equal(length(s), 4)
    }
    t |> run("musical symbol pair") @(t : T?) {
        let s = decode(t, "\"\\ud834\\udd1e\"")
        t |> equal(utf8_bytes(s), "240 157 132 158")
    }
    t |> run("last code point") @(t : T?) {
        let s = decode(t, "\"\\udbff\\udfff\"")
        t |> equal(utf8_bytes(s), "244 143 191 191")
    }
    t |> run("pair surrounded by ASCII") @(t : T?) {
        let s = decode(t, "\"a\\ud83d\\ude00b\"")
        t |> equal(utf8_bytes(s), "97 240 159 152 128 98")
    }
    t |> run("two pairs in a row") @(t : T?) {
        let s = decode(t, "\"\\ud83d\\ude00\\ud83d\\ude00\"")
        t |> equal(utf8_bytes(s), "240 159 152 128 240 159 152 128")
    }
}

[test]
def test_lone_surrogates(t : T?) {
    t |> run("lone high surrogate becomes U+FFFD") @(t : T?) {
        let s = decode(t, "\"\\ud83d\"")
        t |> equal(utf8_bytes(s), "239 191 189")
    }
    t |> run("lone low surrogate becomes U+FFFD") @(t : T?) {
        let s = decode(t, "\"\\ude00\"")
        t |> equal(utf8_bytes(s), "239 191 189")
    }
    t |> run("high surrogate followed by a literal char") @(t : T?) {
        let s = decode(t, "\"\\ud83dx\"")
        t |> equal(utf8_bytes(s), "239 191 189 120")
    }
    t |> run("high surrogate followed by a non-u escape") @(t : T?) {
        let s = decode(t, "\"\\ud83d\\n\"")
        t |> equal(utf8_bytes(s), "239 191 189 10")
    }
    t |> run("high surrogate followed by a BMP escape") @(t : T?) {
        let s = decode(t, "\"\\ud83d\\u0041\"")
        t |> equal(utf8_bytes(s), "239 191 189 65")
    }
    t |> run("two high surrogates then a low one") @(t : T?) {
        let s = decode(t, "\"\\ud83d\\ud83d\\ude00\"")
        t |> equal(utf8_bytes(s), "239 191 189 240 159 152 128")
    }
    t |> run("pair split by a literal char") @(t : T?) {
        let s = decode(t, "\"\\ud83dx\\ude00\"")
        t |> equal(utf8_bytes(s), "239 191 189 120 239 191 189")
    }
}

[test]
def test_bmp_escapes_unchanged(t : T?) {
    t |> run("ASCII") @(t : T?) {
        let s = decode(t, "\"\\u0041\"")
        t |> equal(utf8_bytes(s), "65")
    }
    t |> run("two-byte") @(t : T?) {
        let s = decode(t, "\"\\u00e9\"")
        t |> equal(utf8_bytes(s), "195 169")
    }
    t |> run("three-byte") @(t : T?) {
        let s = decode(t, "\"\\u20ac\"")
        t |> equal(utf8_bytes(s), "226 130 172")
    }
    t |> run("just below the surrogate block") @(t : T?) {
        let s = decode(t, "\"\\ud7ff\"")
        t |> equal(utf8_bytes(s), "237 159 191")
    }
    t |> run("just above the surrogate block") @(t : T?) {
        let s = decode(t, "\"\\ue000\"")
        t |> equal(utf8_bytes(s), "238 128 128")
    }
    t |> run("malformed escape is preserved verbatim") @(t : T?) {
        let s = decode(t, "\"\\u12zz\"")
        t |> equal(s, "\\u12zz")
    }
}
