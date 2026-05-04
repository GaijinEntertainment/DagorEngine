options gen2
require pugixml/PUGIXML_boost
require dastest/testing_boost public

// ============ Test types ============

enum Color {
    red
    green
    blue
}

struct WithEnum {
    name : string
    color : Color
}

// ============ enum tests ============

[test]
def test_xml_serialize_enum(t : T?) {
    t |> run("serialize struct with enum field") <| @(t : T?) {
        var s : WithEnum
        s.name = "sky"
        s.color = Color.blue
        let xml_str = to_XML(s, "item")
        parse_xml(xml_str) <| $(doc, ok) {
            t |> success(ok)
            let root = doc.document_element
            t |> equal(as_string(child(root, "color").text, ""), "blue")
        }
    }
}

[test]
def test_from_xml_enum(t : T?) {
    t |> run("deserialize struct with enum field") <| @(t : T?) {
        let xml_str = "<item><name>sky</name><color>green</color></item>"
        parse_xml(xml_str) <| $(doc, ok) {
            t |> success(ok)
            let r = from_XML(doc.document_element, type<WithEnum>)
            t |> equal(r.name, "sky")
            t |> equal(r.color, Color.green)
        }
    }
}

[test]
def test_roundtrip_enum(t : T?) {
    t |> run("roundtrip struct with enum") <| @(t : T?) {
        var original : WithEnum
        original.name = "grass"
        original.color = Color.green
        let xml_str = to_XML(original, "item")
        parse_xml(xml_str) <| $(doc, ok) {
            t |> success(ok)
            let restored = from_XML(doc.document_element, type<WithEnum>)
            t |> equal(restored.name, "grass")
            t |> equal(restored.color, Color.green)
        }
    }
}

[test]
def test_roundtrip_enum_default(t : T?) {
    t |> run("roundtrip enum with default value") <| @(t : T?) {
        var original : WithEnum
        original.name = "fire"
        // color defaults to Color.red (first value = 0)
        let xml_str = to_XML(original, "item")
        parse_xml(xml_str) <| $(doc, ok) {
            t |> success(ok)
            let restored = from_XML(doc.document_element, type<WithEnum>)
            t |> equal(restored.color, Color.red)
        }
    }
}
