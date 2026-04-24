options gen2
require pugixml/PUGIXML_boost
require dastest/testing_boost public

// ============ Test types ============

variant IntOrString {
    i_val : int
    s_val : string
}

struct WithVariant {
    tag_name : string
    data : IntOrString
}

// ============ Variant tests ============

[test]
def test_xml_serialize_variant_int(t : T?) {
    t |> run("serialize variant (int case) to XML") <| @(t : T?) {
        var v : IntOrString
        unsafe { v.i_val = 42; }
        with_doc() <| $(doc) {
            var dnode = doc as xml_node
            var root = append_child(dnode, "val")
            XML(root, v)
            t |> equal(node_attr_int(root, "_variant"), 0)
            t |> equal(child(root, "i_val").text as int, 42)
        }
    }
}

[test]
def test_xml_serialize_variant_string(t : T?) {
    t |> run("serialize variant (string case) to XML") <| @(t : T?) {
        var v : IntOrString
        unsafe {
            set_variant_index(v, typeinfo variant_index<s_val>(v))
            v.s_val = "hello"
        }
        with_doc() <| $(doc) {
            var dnode = doc as xml_node
            var root = append_child(dnode, "val")
            XML(root, v)
            t |> equal(node_attr_int(root, "_variant"), 1)
            t |> equal(node_text(root, "s_val"), "hello")
        }
    }
}

[test]
def test_xml_serialize_struct_with_variant(t : T?) {
    t |> run("serialize struct containing variant") <| @(t : T?) {
        var wv : WithVariant
        wv.tag_name = "test"
        unsafe {
            set_variant_index(wv.data, typeinfo variant_index<i_val>(wv.data))
            wv.data.i_val = 99
        }
        with_doc() <| $(doc) {
            var dnode = doc as xml_node
            var root = append_child(dnode, "item")
            XML(root, wv)
            t |> equal(node_text(root, "tag_name"), "test")
            let data_node = child(root, "data")
            t |> success(data_node.ok)
            t |> equal(node_attr_int(data_node, "_variant"), 0)
            t |> equal(child(data_node, "i_val").text as int, 99)
        }
    }
}

[test]
def test_from_xml_variant_int(t : T?) {
    t |> run("deserialize variant (int case) from XML") <| @(t : T?) {
        parse_xml("<val _variant=\"0\"><i_val>77</i_val></val>") <| $(doc, ok) {
            t |> success(ok)
            let root = doc.document_element
            let v = from_XML(root, type<IntOrString>)
            t |> success(v is i_val)
            t |> equal(v as i_val, 77)
        }
    }
}

[test]
def test_from_xml_variant_string(t : T?) {
    t |> run("deserialize variant (string case) from XML") <| @(t : T?) {
        parse_xml("<val _variant=\"1\"><s_val>world</s_val></val>") <| $(doc, ok) {
            t |> success(ok)
            let root = doc.document_element
            let v = from_XML(root, type<IntOrString>)
            t |> success(v is s_val)
            t |> equal(v as s_val, "world")
        }
    }
}

[test]
def test_roundtrip_variant_int(t : T?) {
    t |> run("roundtrip variant (int case)") <| @(t : T?) {
        var original : IntOrString
        unsafe { original.i_val = 123; }
        let xml_str = to_XML(original, "val")
        parse_xml(xml_str) <| $(doc, ok) {
            t |> success(ok)
            let restored = from_XML(doc.document_element, type<IntOrString>)
            t |> success(restored is i_val)
            t |> equal(restored as i_val, 123)
        }
    }
}

[test]
def test_roundtrip_variant_string(t : T?) {
    t |> run("roundtrip variant (string case)") <| @(t : T?) {
        var original : IntOrString
        unsafe {
            set_variant_index(original, typeinfo variant_index<s_val>(original))
            original.s_val = "roundtrip"
        }
        let xml_str = to_XML(original, "val")
        parse_xml(xml_str) <| $(doc, ok) {
            t |> success(ok)
            let restored = from_XML(doc.document_element, type<IntOrString>)
            t |> success(restored is s_val)
            t |> equal(restored as s_val, "roundtrip")
        }
    }
}

[test]
def test_roundtrip_struct_with_variant(t : T?) {
    t |> run("roundtrip struct with variant field") <| @(t : T?) {
        var original : WithVariant
        original.tag_name = "entry"
        unsafe {
            set_variant_index(original.data, typeinfo variant_index<s_val>(original.data))
            original.data.s_val = "payload"
        }
        let xml_str = to_XML(original, "item")
        parse_xml(xml_str) <| $(doc, ok) {
            t |> success(ok)
            let restored = from_XML(doc.document_element, type<WithVariant>)
            t |> equal(restored.tag_name, "entry")
            t |> success(restored.data is s_val)
            t |> equal(restored.data as s_val, "payload")
        }
    }
}
