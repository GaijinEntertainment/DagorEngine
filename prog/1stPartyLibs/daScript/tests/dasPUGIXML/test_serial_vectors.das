options gen2
require pugixml/PUGIXML_boost
require math
require dastest/testing_boost public

// ============ Test structs ============

struct WithVectors {
    pos   : float3
    size  : int2
    color : uint4
    span  : range
    span64 : range64
}

// ============ vector type tests ============

[test]
def test_xml_serialize_float2(t : T?) {
    t |> run("serialize float2") <| @(t : T?) {
        let v = float2(1.5, 2.5)
        let xml_str = to_XML(v, "vec")
        parse_xml(xml_str) <| $(doc, ok) {
            t |> success(ok)
            let root = doc.document_element
            t |> equal(as_float(child(root, "x").text, 0.0), 1.5)
            t |> equal(as_float(child(root, "y").text, 0.0), 2.5)
        }
    }
}

[test]
def test_xml_serialize_float3(t : T?) {
    t |> run("serialize float3") <| @(t : T?) {
        let v = float3(1.0, 2.0, 3.0)
        let xml_str = to_XML(v, "vec")
        parse_xml(xml_str) <| $(doc, ok) {
            t |> success(ok)
            let root = doc.document_element
            t |> equal(as_float(child(root, "x").text, 0.0), 1.0)
            t |> equal(as_float(child(root, "y").text, 0.0), 2.0)
            t |> equal(as_float(child(root, "z").text, 0.0), 3.0)
        }
    }
}

[test]
def test_xml_serialize_float4(t : T?) {
    t |> run("serialize float4") <| @(t : T?) {
        let v = float4(1.0, 2.0, 3.0, 4.0)
        let xml_str = to_XML(v, "vec")
        parse_xml(xml_str) <| $(doc, ok) {
            t |> success(ok)
            let root = doc.document_element
            t |> equal(as_float(child(root, "x").text, 0.0), 1.0)
            t |> equal(as_float(child(root, "y").text, 0.0), 2.0)
            t |> equal(as_float(child(root, "z").text, 0.0), 3.0)
            t |> equal(as_float(child(root, "w").text, 0.0), 4.0)
        }
    }
}

[test]
def test_xml_serialize_int2(t : T?) {
    t |> run("serialize int2") <| @(t : T?) {
        let v = int2(10, 20)
        let xml_str = to_XML(v, "vec")
        parse_xml(xml_str) <| $(doc, ok) {
            t |> success(ok)
            let root = doc.document_element
            t |> equal(as_int(child(root, "x").text, 0), 10)
            t |> equal(as_int(child(root, "y").text, 0), 20)
        }
    }
}

[test]
def test_xml_serialize_int4(t : T?) {
    t |> run("serialize int4") <| @(t : T?) {
        let v = int4(1, 2, 3, 4)
        let xml_str = to_XML(v, "vec")
        parse_xml(xml_str) <| $(doc, ok) {
            t |> success(ok)
            let root = doc.document_element
            t |> equal(as_int(child(root, "x").text, 0), 1)
            t |> equal(as_int(child(root, "y").text, 0), 2)
            t |> equal(as_int(child(root, "z").text, 0), 3)
            t |> equal(as_int(child(root, "w").text, 0), 4)
        }
    }
}

[test]
def test_xml_serialize_range(t : T?) {
    t |> run("serialize range") <| @(t : T?) {
        let v = range(5, 15)
        let xml_str = to_XML(v, "rng")
        parse_xml(xml_str) <| $(doc, ok) {
            t |> success(ok)
            let root = doc.document_element
            t |> equal(as_int(child(root, "x").text, 0), 5)
            t |> equal(as_int(child(root, "y").text, 0), 15)
        }
    }
}

[test]
def test_xml_serialize_urange(t : T?) {
    t |> run("serialize urange") <| @(t : T?) {
        let v = urange(0u, 100u)
        let xml_str = to_XML(v, "rng")
        parse_xml(xml_str) <| $(doc, ok) {
            t |> success(ok)
            let root = doc.document_element
            t |> equal(as_uint(child(root, "x").text, 0u), 0u)
            t |> equal(as_uint(child(root, "y").text, 0u), 100u)
        }
    }
}

[test]
def test_xml_serialize_range64(t : T?) {
    t |> run("serialize range64") <| @(t : T?) {
        let v = range64(100l, 9999999999l)
        let xml_str = to_XML(v, "rng")
        parse_xml(xml_str) <| $(doc, ok) {
            t |> success(ok)
            let root = doc.document_element
            t |> equal(as_int64(child(root, "x").text, 0l), 100l)
            t |> equal(as_int64(child(root, "y").text, 0l), 9999999999l)
        }
    }
}

[test]
def test_xml_serialize_urange64(t : T?) {
    t |> run("serialize urange64") <| @(t : T?) {
        let v = urange64(0ul, 18446744073709551615ul)
        let xml_str = to_XML(v, "rng")
        parse_xml(xml_str) <| $(doc, ok) {
            t |> success(ok)
            let root = doc.document_element
            t |> equal(as_uint64(child(root, "x").text, 0ul), 0ul)
            t |> equal(as_uint64(child(root, "y").text, 0ul), 18446744073709551615ul)
        }
    }
}

[test]
def test_roundtrip_float2(t : T?) {
    t |> run("roundtrip float2") <| @(t : T?) {
        let original = float2(3.14, -2.71)
        let xml_str = to_XML(original, "vec")
        parse_xml(xml_str) <| $(doc, ok) {
            t |> success(ok)
            let restored = from_XML(doc.document_element, type<float2>)
            t |> success(abs(restored.x - 3.14) < 0.01)
            t |> success(abs(restored.y + 2.71) < 0.01)
        }
    }
}

[test]
def test_roundtrip_float3(t : T?) {
    t |> run("roundtrip float3") <| @(t : T?) {
        let original = float3(1.0, 2.0, 3.0)
        let xml_str = to_XML(original, "vec")
        parse_xml(xml_str) <| $(doc, ok) {
            t |> success(ok)
            let restored = from_XML(doc.document_element, type<float3>)
            t |> equal(restored, float3(1.0, 2.0, 3.0))
        }
    }
}

[test]
def test_roundtrip_float4(t : T?) {
    t |> run("roundtrip float4") <| @(t : T?) {
        let original = float4(1.0, 2.0, 3.0, 4.0)
        let xml_str = to_XML(original, "vec")
        parse_xml(xml_str) <| $(doc, ok) {
            t |> success(ok)
            let restored = from_XML(doc.document_element, type<float4>)
            t |> equal(restored, float4(1.0, 2.0, 3.0, 4.0))
        }
    }
}

[test]
def test_roundtrip_int2(t : T?) {
    t |> run("roundtrip int2") <| @(t : T?) {
        let original = int2(-5, 10)
        let xml_str = to_XML(original, "vec")
        parse_xml(xml_str) <| $(doc, ok) {
            t |> success(ok)
            let restored = from_XML(doc.document_element, type<int2>)
            t |> equal(restored, int2(-5, 10))
        }
    }
}

[test]
def test_roundtrip_int3(t : T?) {
    t |> run("roundtrip int3") <| @(t : T?) {
        let original = int3(1, -2, 3)
        let xml_str = to_XML(original, "vec")
        parse_xml(xml_str) <| $(doc, ok) {
            t |> success(ok)
            let restored = from_XML(doc.document_element, type<int3>)
            t |> equal(restored, int3(1, -2, 3))
        }
    }
}

[test]
def test_roundtrip_int4(t : T?) {
    t |> run("roundtrip int4") <| @(t : T?) {
        let original = int4(10, 20, 30, 40)
        let xml_str = to_XML(original, "vec")
        parse_xml(xml_str) <| $(doc, ok) {
            t |> success(ok)
            let restored = from_XML(doc.document_element, type<int4>)
            t |> equal(restored, int4(10, 20, 30, 40))
        }
    }
}

[test]
def test_roundtrip_uint4(t : T?) {
    t |> run("roundtrip uint4") <| @(t : T?) {
        let original = uint4(1u, 2u, 3u, 4u)
        let xml_str = to_XML(original, "vec")
        parse_xml(xml_str) <| $(doc, ok) {
            t |> success(ok)
            let restored = from_XML(doc.document_element, type<uint4>)
            t |> equal(restored, uint4(1u, 2u, 3u, 4u))
        }
    }
}

[test]
def test_roundtrip_range(t : T?) {
    t |> run("roundtrip range") <| @(t : T?) {
        let original = range(5, 15)
        let xml_str = to_XML(original, "rng")
        parse_xml(xml_str) <| $(doc, ok) {
            t |> success(ok)
            let restored = from_XML(doc.document_element, type<range>)
            t |> equal(restored.x, 5)
            t |> equal(restored.y, 15)
        }
    }
}

[test]
def test_roundtrip_urange(t : T?) {
    t |> run("roundtrip urange") <| @(t : T?) {
        let original = urange(10u, 50u)
        let xml_str = to_XML(original, "rng")
        parse_xml(xml_str) <| $(doc, ok) {
            t |> success(ok)
            let restored = from_XML(doc.document_element, type<urange>)
            t |> equal(restored.x, 10u)
            t |> equal(restored.y, 50u)
        }
    }
}

[test]
def test_roundtrip_range64(t : T?) {
    t |> run("roundtrip range64") <| @(t : T?) {
        let original = range64(-1000000000l, 1000000000l)
        let xml_str = to_XML(original, "rng")
        parse_xml(xml_str) <| $(doc, ok) {
            t |> success(ok)
            let restored = from_XML(doc.document_element, type<range64>)
            t |> equal(restored.x, -1000000000l)
            t |> equal(restored.y, 1000000000l)
        }
    }
}

[test]
def test_roundtrip_urange64(t : T?) {
    t |> run("roundtrip urange64") <| @(t : T?) {
        let original = urange64(0ul, 18446744073709551615ul)
        let xml_str = to_XML(original, "rng")
        parse_xml(xml_str) <| $(doc, ok) {
            t |> success(ok)
            let restored = from_XML(doc.document_element, type<urange64>)
            t |> equal(restored.x, 0ul)
            t |> equal(restored.y, 18446744073709551615ul)
        }
    }
}

[test]
def test_roundtrip_struct_with_vectors(t : T?) {
    t |> run("roundtrip struct with vector fields") <| @(t : T?) {
        var original : WithVectors
        original.pos = float3(1.0, 2.0, 3.0)
        original.size = int2(800, 600)
        original.color = uint4(255u, 128u, 0u, 255u)
        original.span = range(10, 20)
        original.span64 = range64(-100l, 100l)
        let xml_str = to_XML(original, "item")
        parse_xml(xml_str) <| $(doc, ok) {
            t |> success(ok)
            let restored = from_XML(doc.document_element, type<WithVectors>)
            t |> equal(restored.pos, float3(1.0, 2.0, 3.0))
            t |> equal(restored.size, int2(800, 600))
            t |> equal(restored.color, uint4(255u, 128u, 0u, 255u))
            t |> equal(restored.span.x, 10)
            t |> equal(restored.span.y, 20)
            t |> equal(restored.span64.x, -100l)
            t |> equal(restored.span64.y, 100l)
        }
    }
}

[test]
def test_xml_array_of_float2(t : T?) {
    t |> run("roundtrip array of float2") <| @(t : T?) {
        var arr : array<float2>
        arr |> push(float2(1.0, 2.0))
        arr |> push(float2(3.0, 4.0))
        let xml_str = to_XML(arr, "list")
        parse_xml(xml_str) <| $(doc, ok) {
            t |> success(ok)
            let restored <- from_XML(doc.document_element, type<array<float2>>)
            t |> equal(length(restored), 2)
            t |> equal(restored[0], float2(1.0, 2.0))
            t |> equal(restored[1], float2(3.0, 4.0))
        }
    }
}

[test]
def test_roundtrip_vector_defaults(t : T?) {
    t |> run("roundtrip vector default values") <| @(t : T?) {
        var original : WithVectors
        let xml_str = to_XML(original, "item")
        parse_xml(xml_str) <| $(doc, ok) {
            t |> success(ok)
            let restored = from_XML(doc.document_element, type<WithVectors>)
            t |> equal(restored.pos, float3(0.0, 0.0, 0.0))
            t |> equal(restored.size, int2(0, 0))
            t |> equal(restored.color, uint4(0u, 0u, 0u, 0u))
            t |> equal(restored.span.x, 0)
            t |> equal(restored.span.y, 0)
        }
    }
}
