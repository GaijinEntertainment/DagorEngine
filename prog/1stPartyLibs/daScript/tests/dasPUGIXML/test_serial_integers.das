options gen2
require pugixml/PUGIXML_boost
require dastest/testing_boost public

// ============ Test structs ============

struct WithInts {
    i8  : int8
    u8  : uint8
    i16 : int16
    u16 : uint16
    i64 : int64
    u64 : uint64
}

// ============ integer type tests ============

[test]
def test_xml_serialize_int8(t : T?) {
    t |> run("serialize int8 value") <| @(t : T?) {
        var s : WithInts
        s.i8 = int8(-42)
        let xml_str = to_XML(s, "item")
        parse_xml(xml_str) <| $(doc, ok) {
            t |> success(ok)
            let root = doc.document_element
            t |> equal(as_int(child(root, "i8").text, 0), -42)
        }
    }
}

[test]
def test_xml_serialize_uint8(t : T?) {
    t |> run("serialize uint8 value") <| @(t : T?) {
        var s : WithInts
        s.u8 = uint8(200)
        let xml_str = to_XML(s, "item")
        parse_xml(xml_str) <| $(doc, ok) {
            t |> success(ok)
            let root = doc.document_element
            t |> equal(as_uint(child(root, "u8").text, 0u), 200u)
        }
    }
}

[test]
def test_xml_serialize_int16(t : T?) {
    t |> run("serialize int16 value") <| @(t : T?) {
        var s : WithInts
        s.i16 = int16(-1000)
        let xml_str = to_XML(s, "item")
        parse_xml(xml_str) <| $(doc, ok) {
            t |> success(ok)
            let root = doc.document_element
            t |> equal(as_int(child(root, "i16").text, 0), -1000)
        }
    }
}

[test]
def test_xml_serialize_uint16(t : T?) {
    t |> run("serialize uint16 value") <| @(t : T?) {
        var s : WithInts
        s.u16 = uint16(50000)
        let xml_str = to_XML(s, "item")
        parse_xml(xml_str) <| $(doc, ok) {
            t |> success(ok)
            let root = doc.document_element
            t |> equal(as_uint(child(root, "u16").text, 0u), 50000u)
        }
    }
}

[test]
def test_xml_serialize_int64(t : T?) {
    t |> run("serialize int64 value") <| @(t : T?) {
        var s : WithInts
        s.i64 = 9223372036854775807l
        let xml_str = to_XML(s, "item")
        parse_xml(xml_str) <| $(doc, ok) {
            t |> success(ok)
            let root = doc.document_element
            t |> equal(as_int64(child(root, "i64").text, 0l), 9223372036854775807l)
        }
    }
}

[test]
def test_xml_serialize_uint64(t : T?) {
    t |> run("serialize uint64 value") <| @(t : T?) {
        var s : WithInts
        s.u64 = 18446744073709551615ul
        let xml_str = to_XML(s, "item")
        parse_xml(xml_str) <| $(doc, ok) {
            t |> success(ok)
            let root = doc.document_element
            t |> equal(as_uint64(child(root, "u64").text, 0ul), 18446744073709551615ul)
        }
    }
}

[test]
def test_from_xml_int8(t : T?) {
    t |> run("deserialize int8 value") <| @(t : T?) {
        let xml_str = "<item><i8>-128</i8><u8>0</u8><i16>0</i16><u16>0</u16><i64>0</i64><u64>0</u64></item>"
        parse_xml(xml_str) <| $(doc, ok) {
            t |> success(ok)
            let r = from_XML(doc.document_element, type<WithInts>)
            t |> equal(int(r.i8), -128)
        }
    }
}

[test]
def test_from_xml_uint8(t : T?) {
    t |> run("deserialize uint8 value") <| @(t : T?) {
        let xml_str = "<item><i8>0</i8><u8>255</u8><i16>0</i16><u16>0</u16><i64>0</i64><u64>0</u64></item>"
        parse_xml(xml_str) <| $(doc, ok) {
            t |> success(ok)
            let r = from_XML(doc.document_element, type<WithInts>)
            t |> equal(int(r.u8), 255)
        }
    }
}

[test]
def test_from_xml_int64(t : T?) {
    t |> run("deserialize int64 value") <| @(t : T?) {
        let xml_str = "<item><i8>0</i8><u8>0</u8><i16>0</i16><u16>0</u16><i64>-9223372036854775807</i64><u64>0</u64></item>"
        parse_xml(xml_str) <| $(doc, ok) {
            t |> success(ok)
            let r = from_XML(doc.document_element, type<WithInts>)
            t |> equal(r.i64, -9223372036854775807l)
        }
    }
}

[test]
def test_roundtrip_all_int_types(t : T?) {
    t |> run("roundtrip all integer types") <| @(t : T?) {
        var original : WithInts
        original.i8  = int8(-100)
        original.u8  = uint8(200)
        original.i16 = int16(-30000)
        original.u16 = uint16(60000)
        original.i64 = -1234567890123456789l
        original.u64 = 12345678901234567890ul
        let xml_str = to_XML(original, "item")
        parse_xml(xml_str) <| $(doc, ok) {
            t |> success(ok)
            let restored = from_XML(doc.document_element, type<WithInts>)
            t |> equal(int(restored.i8), -100)
            t |> equal(int(restored.u8), 200)
            t |> equal(int(restored.i16), -30000)
            t |> equal(int(restored.u16), 60000)
            t |> equal(restored.i64, -1234567890123456789l)
            t |> equal(restored.u64, 12345678901234567890ul)
        }
    }
}

[test]
def test_roundtrip_int_defaults(t : T?) {
    t |> run("roundtrip integer default values") <| @(t : T?) {
        var original : WithInts
        let xml_str = to_XML(original, "item")
        parse_xml(xml_str) <| $(doc, ok) {
            t |> success(ok)
            let restored = from_XML(doc.document_element, type<WithInts>)
            t |> equal(int(restored.i8), 0)
            t |> equal(int(restored.u8), 0)
            t |> equal(int(restored.i16), 0)
            t |> equal(int(restored.u16), 0)
            t |> equal(restored.i64, 0l)
            t |> equal(restored.u64, 0ul)
        }
    }
}

[test]
def test_from_xml_standalone_int8(t : T?) {
    t |> run("from_XML standalone int8") <| @(t : T?) {
        let xml_str = "<val>-42</val>"
        parse_xml(xml_str) <| $(doc, ok) {
            t |> success(ok)
            let v = from_XML(doc.document_element, type<int8>)
            t |> equal(int(v), -42)
        }
    }
}

[test]
def test_from_xml_standalone_uint64(t : T?) {
    t |> run("from_XML standalone uint64") <| @(t : T?) {
        let xml_str = "<val>18446744073709551615</val>"
        parse_xml(xml_str) <| $(doc, ok) {
            t |> success(ok)
            let v = from_XML(doc.document_element, type<uint64>)
            t |> equal(v, 18446744073709551615ul)
        }
    }
}

[test]
def test_xml_array_of_int16(t : T?) {
    t |> run("roundtrip array of int16") <| @(t : T?) {
        var arr : array<int16>
        arr |> push(int16(-1))
        arr |> push(int16(0))
        arr |> push(int16(32767))
        let xml_str = to_XML(arr, "list")
        parse_xml(xml_str) <| $(doc, ok) {
            t |> success(ok)
            let restored <- from_XML(doc.document_element, type<array<int16>>)
            t |> equal(length(restored), 3)
            t |> equal(int(restored[0]), -1)
            t |> equal(int(restored[1]), 0)
            t |> equal(int(restored[2]), 32767)
        }
    }
}
