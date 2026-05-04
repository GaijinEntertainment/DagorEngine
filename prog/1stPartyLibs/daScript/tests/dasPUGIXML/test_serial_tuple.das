options gen2
require pugixml/PUGIXML_boost
require math
require dastest/testing_boost public

// ============ Test structs ============

struct WithTuple {
    tag_name : string
    pair : tuple<int; int>
}

// ============ Tuple tests ============

[test]
def test_xml_serialize_tuple(t : T?) {
    t |> run("serialize tuple to XML") <| @(t : T?) {
        let tup = ("hello" => 42)
        with_doc() <| $(doc) {
            var dnode = doc as xml_node
            var root = append_child(dnode, "pair")
            XML(root, tup)
            t |> equal(node_text(root, "_0"), "hello")
            t |> equal(child(root, "_1").text as int, 42)
        }
    }
}

[test]
def test_xml_serialize_tuple_3(t : T?) {
    t |> run("serialize 3-element tuple to XML") <| @(t : T?) {
        var tup : tuple<name : string; score : float; rank : int>
        tup.name = "Alice"
        tup.score = 9.5
        tup.rank = 1
        with_doc() <| $(doc) {
            var dnode = doc as xml_node
            var root = append_child(dnode, "entry")
            XML(root, tup)
            t |> equal(node_text(root, "name"), "Alice")
            t |> success(abs(child(root, "score").text as float -9.5) < 0.01)
            t |> equal(child(root, "rank").text as int, 1)
        }
    }
}

[test]
def test_xml_serialize_struct_with_tuple(t : T?) {
    t |> run("serialize struct containing tuple") <| @(t : T?) {
        var wt : WithTuple
        wt.tag_name = "coords"
        wt.pair._0 = 10
        wt.pair._1 = 20
        with_doc() <| $(doc) {
            var dnode = doc as xml_node
            var root = append_child(dnode, "item")
            XML(root, wt)
            t |> equal(node_text(root, "tag_name"), "coords")
            let pair_node = child(root, "pair")
            t |> success(pair_node.ok)
            t |> equal(child(pair_node, "_0").text as int, 10)
            t |> equal(child(pair_node, "_1").text as int, 20)
        }
    }
}

[test]
def test_from_xml_tuple(t : T?) {
    t |> run("deserialize tuple from XML") <| @(t : T?) {
        parse_xml("<pair><_0>world</_0><_1>99</_1></pair>") <| $(doc, ok) {
            t |> success(ok)
            let root = doc.document_element
            let tup = from_XML(root, type<tuple<string; int>>)
            t |> equal(tup._0, "world")
            t |> equal(tup._1, 99)
        }
    }
}

[test]
def test_from_xml_named_tuple(t : T?) {
    t |> run("deserialize named tuple from XML") <| @(t : T?) {
        parse_xml("<entry><name>Bob</name><score>8.5</score></entry>") <| $(doc, ok) {
            t |> success(ok)
            let root = doc.document_element
            let tup = from_XML(root, type<tuple<name : string; score : float>>)
            t |> equal(tup.name, "Bob")
            t |> success(abs(tup.score - 8.5) < 0.01)
        }
    }
}

[test]
def test_roundtrip_tuple(t : T?) {
    t |> run("roundtrip tuple serialize/deserialize") <| @(t : T?) {
        let original = ("test" => 777)
        let xml_str = to_XML(original, "pair")
        parse_xml(xml_str) <| $(doc, ok) {
            t |> success(ok)
            let restored = from_XML(doc.document_element, type<tuple<string; int>>)
            t |> equal(restored._0, "test")
            t |> equal(restored._1, 777)
        }
    }
}

[test]
def test_roundtrip_struct_with_tuple(t : T?) {
    t |> run("roundtrip struct with tuple field") <| @(t : T?) {
        var original : WithTuple
        original.tag_name = "pos"
        original.pair._0 = 100
        original.pair._1 = 200
        let xml_str = to_XML(original, "item")
        parse_xml(xml_str) <| $(doc, ok) {
            t |> success(ok)
            let restored = from_XML(doc.document_element, type<WithTuple>)
            t |> equal(restored.tag_name, "pos")
            t |> equal(restored.pair._0, 100)
            t |> equal(restored.pair._1, 200)
        }
    }
}
