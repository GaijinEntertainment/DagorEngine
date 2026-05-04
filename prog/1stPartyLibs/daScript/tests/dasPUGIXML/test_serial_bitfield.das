options gen2
require pugixml/PUGIXML_boost
require dastest/testing_boost public

// ============ Test types ============

bitfield Perms {
    readable
    writable
    executable
}

bitfield Flags8 : uint8 {
    a
    b
    c
}

bitfield Flags16 : uint16 {
    x
    y
    z
}

bitfield Flags64 : uint64 {
    lo
    mid
    hi
}

struct WithBitfield {
    name : string
    perms : Perms
}

struct WithBitfield8 {
    flags : Flags8
}

struct WithBitfield16 {
    flags : Flags16
}

struct WithBitfield64 {
    flags : Flags64
}

// ============ bitfield tests ============

[test]
def test_xml_serialize_bitfield(t : T?) {
    t |> run("serialize struct with bitfield") <| @(t : T?) {
        var s : WithBitfield
        s.name = "script"
        s.perms = Perms.readable | Perms.executable
        let xml_str = to_XML(s, "item")
        parse_xml(xml_str) <| $(doc, ok) {
            t |> success(ok)
            let root = doc.document_element
            t |> equal(as_uint(child(root, "perms").text, 0u), 5u)
        }
    }
}

[test]
def test_from_xml_bitfield(t : T?) {
    t |> run("deserialize struct with bitfield") <| @(t : T?) {
        let xml_str = "<item><name>doc</name><perms>3</perms></item>"
        parse_xml(xml_str) <| $(doc, ok) {
            t |> success(ok)
            let r = from_XML(doc.document_element, type<WithBitfield>)
            t |> equal(r.name, "doc")
            t |> equal(uint(r.perms), 3u)
        }
    }
}

[test]
def test_roundtrip_bitfield(t : T?) {
    t |> run("roundtrip struct with bitfield") <| @(t : T?) {
        var original : WithBitfield
        original.name = "binary"
        original.perms = Perms.readable | Perms.writable | Perms.executable
        let xml_str = to_XML(original, "item")
        parse_xml(xml_str) <| $(doc, ok) {
            t |> success(ok)
            let restored = from_XML(doc.document_element, type<WithBitfield>)
            t |> equal(restored.name, "binary")
            t |> equal(uint(restored.perms), 7u)
        }
    }
}

[test]
def test_roundtrip_bitfield_empty(t : T?) {
    t |> run("roundtrip bitfield with no bits set") <| @(t : T?) {
        var original : WithBitfield
        original.name = "none"
        let xml_str = to_XML(original, "item")
        parse_xml(xml_str) <| $(doc, ok) {
            t |> success(ok)
            let restored = from_XML(doc.document_element, type<WithBitfield>)
            t |> equal(uint(restored.perms), 0u)
        }
    }
}

[test]
def test_roundtrip_bitfield8(t : T?) {
    t |> run("roundtrip bitfield8") <| @(t : T?) {
        var original : WithBitfield8
        original.flags = Flags8.a | Flags8.c
        let xml_str = to_XML(original, "item")
        parse_xml(xml_str) <| $(doc, ok) {
            t |> success(ok)
            let root = doc.document_element
            t |> equal(as_uint(child(root, "flags").text, 0u), 5u)
            let restored = from_XML(doc.document_element, type<WithBitfield8>)
            t |> equal(uint(restored.flags), 5u)
        }
    }
}

[test]
def test_roundtrip_bitfield16(t : T?) {
    t |> run("roundtrip bitfield16") <| @(t : T?) {
        var original : WithBitfield16
        original.flags = Flags16.x | Flags16.y | Flags16.z
        let xml_str = to_XML(original, "item")
        parse_xml(xml_str) <| $(doc, ok) {
            t |> success(ok)
            let root = doc.document_element
            t |> equal(as_uint(child(root, "flags").text, 0u), 7u)
            let restored = from_XML(doc.document_element, type<WithBitfield16>)
            t |> equal(uint(restored.flags), 7u)
        }
    }
}

[test]
def test_roundtrip_bitfield64(t : T?) {
    t |> run("roundtrip bitfield64") <| @(t : T?) {
        var original : WithBitfield64
        original.flags = Flags64.lo | Flags64.hi
        let xml_str = to_XML(original, "item")
        parse_xml(xml_str) <| $(doc, ok) {
            t |> success(ok)
            let root = doc.document_element
            let v = as_uint64(child(root, "flags").text, 0ul)
            t |> equal(v, 5ul)
            let restored = from_XML(doc.document_element, type<WithBitfield64>)
            t |> equal(uint64(restored.flags), uint64(original.flags))
        }
    }
}

[test]
def test_roundtrip_bitfield64_high_bit(t : T?) {
    t |> run("roundtrip bitfield64 high bit") <| @(t : T?) {
        var original : WithBitfield64
        original.flags = bitfield64(1ul << 63ul)
        let xml_str = to_XML(original, "item")
        parse_xml(xml_str) <| $(doc, ok) {
            t |> success(ok)
            let restored = from_XML(doc.document_element, type<WithBitfield64>)
            t |> equal(uint64(restored.flags), 1ul << 63ul)
        }
    }
}

[test]
def test_roundtrip_bitfield64_empty(t : T?) {
    t |> run("roundtrip bitfield64 empty") <| @(t : T?) {
        var original : WithBitfield64
        let xml_str = to_XML(original, "item")
        parse_xml(xml_str) <| $(doc, ok) {
            t |> success(ok)
            let restored = from_XML(doc.document_element, type<WithBitfield64>)
            t |> equal(uint64(restored.flags), 0ul)
        }
    }
}
