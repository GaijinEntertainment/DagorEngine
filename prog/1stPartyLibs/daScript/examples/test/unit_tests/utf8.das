require daslib/utf8_utils
require strings


[export]
def test
    let s1 = "_xcd 1 \n\t~A"
    let s2 = "Юникод \n\t~A"

    let arr: array<uint8> <- utf8_encode(1070u) // Ю
    assert(arr[0] == uint8(208))
    assert(arr[1] == uint8(174))

    let codepoints1 <- utf8_decode(s1)
    assert(length(codepoints1) == 11)
    assert(codepoints1[10] == 65u)

    var codepoints2: array<uint> <- utf8_decode(s2)
    assert(length(codepoints2) == 11)
    assert(codepoints2[10] == 65u)
    assert(codepoints2[1] == 1085u)

    let s3 = string(utf8_encode(codepoints2))
    assert(s3 == s2)

    codepoints2 |> utf8_decode(s2)
    assert(codepoints2[21] == 65u)

    assert(utf8_length(s2) == utf8_length(s1))
    assert(utf8_length("") == 0)
    verify(is_utf8_string_valid(s1))
    verify(is_utf8_string_valid(""))

    let invalidStr <- [{ uint8 uint8(0x1F); uint8(0xFF); uint8(0xFF) }]
    verify(!is_utf8_string_valid(invalidStr))

    return true


