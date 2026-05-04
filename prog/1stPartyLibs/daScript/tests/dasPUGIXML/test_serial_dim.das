options gen2
require pugixml/PUGIXML_boost
require dastest/testing_boost public

// ============ Test structs ============

struct WithDim {
    tag_name : string
    values : int[3]
}

// ============ Fixed array (dim) tests ============

[test]
def test_xml_serialize_dim(t : T?) {
    t |> run("serialize fixed array to XML") <| @(t : T?) {
        var arr = fixed_array(10, 20, 30)
        with_doc() <| $(doc) {
            var dnode = doc as xml_node
            var root = append_child(dnode, "arr")
            XML(root, arr)
            var ch = root.first_child
            t |> equal(ch.text as int, 10)
            ch = ch.next_sibling
            t |> equal(ch.text as int, 20)
            ch = ch.next_sibling
            t |> equal(ch.text as int, 30)
        }
    }
}

[test]
def test_xml_serialize_struct_with_dim(t : T?) {
    t |> run("serialize struct containing fixed array") <| @(t : T?) {
        var wd : WithDim
        wd.tag_name = "data"
        wd.values = fixed_array(1, 2, 3)
        with_doc() <| $(doc) {
            var dnode = doc as xml_node
            var root = append_child(dnode, "item")
            XML(root, wd)
            t |> equal(node_text(root, "tag_name"), "data")
            let vals_node = child(root, "values")
            t |> success(vals_node.ok)
            var ch = vals_node.first_child
            t |> equal(ch.text as int, 1)
            ch = ch.next_sibling
            t |> equal(ch.text as int, 2)
            ch = ch.next_sibling
            t |> equal(ch.text as int, 3)
        }
    }
}

[test]
def test_from_xml_dim(t : T?) {
    t |> run("deserialize fixed array from XML") <| @(t : T?) {
        parse_xml("<arr><item>5</item><item>10</item><item>15</item></arr>") <| $(doc, ok) {
            t |> success(ok)
            let root = doc.document_element
            let arr = from_XML(root, type<int[3]>)
            t |> equal(arr[0], 5)
            t |> equal(arr[1], 10)
            t |> equal(arr[2], 15)
        }
    }
}

[test]
def test_from_xml_dim_strings(t : T?) {
    t |> run("deserialize fixed array of strings from XML") <| @(t : T?) {
        parse_xml("<arr><item>a</item><item>b</item></arr>") <| $(doc, ok) {
            t |> success(ok)
            let root = doc.document_element
            let arr = from_XML(root, type<string[2]>)
            t |> equal(arr[0], "a")
            t |> equal(arr[1], "b")
        }
    }
}

[test]
def test_roundtrip_dim(t : T?) {
    t |> run("roundtrip fixed array") <| @(t : T?) {
        var original = fixed_array(100, 200, 300)
        let xml_str = to_XML(original, "arr")
        parse_xml(xml_str) <| $(doc, ok) {
            t |> success(ok)
            let restored = from_XML(doc.document_element, type<int[3]>)
            t |> equal(restored[0], 100)
            t |> equal(restored[1], 200)
            t |> equal(restored[2], 300)
        }
    }
}

[test]
def test_roundtrip_struct_with_dim(t : T?) {
    t |> run("roundtrip struct with fixed array field") <| @(t : T?) {
        var original : WithDim
        original.tag_name = "xyz"
        original.values = fixed_array(7, 8, 9)
        let xml_str = to_XML(original, "item")
        parse_xml(xml_str) <| $(doc, ok) {
            t |> success(ok)
            let restored = from_XML(doc.document_element, type<WithDim>)
            t |> equal(restored.tag_name, "xyz")
            t |> equal(restored.values[0], 7)
            t |> equal(restored.values[1], 8)
            t |> equal(restored.values[2], 9)
        }
    }
}
