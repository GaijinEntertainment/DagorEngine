options gen2
require pugixml/PUGIXML_boost
require dastest/testing_boost public

// ============ Test structs ============

struct WithTable {
    title : string
    scores : table<string; int>
}

// ============ table tests ============

[test]
def test_xml_serialize_table_string_int(t : T?) {
    t |> run("serialize table<string;int> to XML") <| @(t : T?) {
        var tab : table<string; int>
        unsafe {
            tab["alpha"] = 10
            tab["beta"] = 20
        }
        let xml_str = to_XML(tab, "scores")
        parse_xml(xml_str) <| $(doc, ok) {
            t |> success(ok)
            var ch = doc.document_element.first_child
            t |> success(ch.ok)
            t |> equal(string(ch.name), "entry")
        }
    }
}

[test]
def test_xml_serialize_struct_with_table(t : T?) {
    t |> run("serialize struct containing table") <| @(t : T?) {
        var s : WithTable
        s.title = "game"
        unsafe {
            s.scores["alice"] = 100
            s.scores["bob"] = 200
        }
        let xml_str = to_XML(s, "item")
        parse_xml(xml_str) <| $(doc, ok) {
            t |> success(ok)
            let root = doc.document_element
            let title_ch = child(root, "title")
            t |> equal(as_string(title_ch.text, ""), "game")
        }
    }
}

[test]
def test_from_xml_table_string_int(t : T?) {
    t |> run("deserialize table<string;int> from XML") <| @(t : T?) {
        let xml_str = "<scores><entry><_key>x</_key><_val>42</_val></entry><entry><_key>y</_key><_val>99</_val></entry></scores>"
        parse_xml(xml_str) <| $(doc, ok) {
            t |> success(ok)
            var tab <- from_XML(doc.document_element, type<table<string; int>>)
            t |> equal(length(tab), 2)
            unsafe {
                t |> equal(tab["x"], 42)
                t |> equal(tab["y"], 99)
            }
        }
    }
}

[test]
def test_from_xml_table_string_string(t : T?) {
    t |> run("deserialize table<string;string> from XML") <| @(t : T?) {
        let xml_str = "<m><entry><_key>name</_key><_val>Alice</_val></entry></m>"
        parse_xml(xml_str) <| $(doc, ok) {
            t |> success(ok)
            var tab <- from_XML(doc.document_element, type<table<string; string>>)
            t |> equal(length(tab), 1)
            unsafe {
                t |> equal(tab["name"], "Alice")
            }
        }
    }
}

[test]
def test_roundtrip_table_string_int(t : T?) {
    t |> run("roundtrip table<string;int>") <| @(t : T?) {
        var original : table<string; int>
        unsafe {
            original["a"] = 1
            original["b"] = 2
            original["c"] = 3
        }
        let xml_str = to_XML(original, "tab")
        parse_xml(xml_str) <| $(doc, ok) {
            t |> success(ok)
            var restored <- from_XML(doc.document_element, type<table<string; int>>)
            t |> equal(length(restored), 3)
            unsafe {
                t |> equal(restored["a"], 1)
                t |> equal(restored["b"], 2)
                t |> equal(restored["c"], 3)
            }
        }
    }
}

[test]
def test_roundtrip_struct_with_table(t : T?) {
    t |> run("roundtrip struct with table field") <| @(t : T?) {
        var original : WithTable
        original.title = "test"
        unsafe {
            original.scores["p1"] = 50
            original.scores["p2"] = 75
        }
        let xml_str = to_XML(original, "item")
        parse_xml(xml_str) <| $(doc, ok) {
            t |> success(ok)
            var restored <- from_XML(doc.document_element, type<WithTable>)
            t |> equal(restored.title, "test")
            t |> equal(length(restored.scores), 2)
            unsafe {
                t |> equal(restored.scores["p1"], 50)
                t |> equal(restored.scores["p2"], 75)
            }
        }
    }
}

[test]
def test_roundtrip_table_empty(t : T?) {
    t |> run("roundtrip empty table") <| @(t : T?) {
        var original : table<string; int>
        let xml_str = to_XML(original, "tab")
        parse_xml(xml_str) <| $(doc, ok) {
            t |> success(ok)
            var restored <- from_XML(doc.document_element, type<table<string; int>>)
            t |> equal(length(restored), 0)
        }
    }
}

// ============ key-only table (set) tests ============

[test]
def test_xml_serialize_set(t : T?) {
    t |> run("serialize table<string> (set) to XML") <| @(t : T?) {
        var s : table<string>
        s |> insert("apple")
        s |> insert("banana")
        let xml_str = to_XML(s, "fruits")
        parse_xml(xml_str) <| $(doc, ok) {
            t |> success(ok)
            let root = doc.document_element
            var count = 0
            for_each_child(root) <| $(ch) {
                t |> equal(string(ch.name), "item")
                count++
            }
            t |> equal(count, 2)
        }
    }
}

[test]
def test_from_xml_set(t : T?) {
    t |> run("deserialize table<int> (set) from XML") <| @(t : T?) {
        let xml_str = "<s><item>10</item><item>20</item><item>30</item></s>"
        parse_xml(xml_str) <| $(doc, ok) {
            t |> success(ok)
            var s <- from_XML(doc.document_element, type<table<int>>)
            t |> equal(length(s), 3)
            t |> success(s |> key_exists(10))
            t |> success(s |> key_exists(20))
            t |> success(s |> key_exists(30))
        }
    }
}

[test]
def test_roundtrip_set(t : T?) {
    t |> run("roundtrip table<string> (set)") <| @(t : T?) {
        var original : table<string>
        original |> insert("x")
        original |> insert("y")
        original |> insert("z")
        let xml_str = to_XML(original, "set")
        parse_xml(xml_str) <| $(doc, ok) {
            t |> success(ok)
            var restored <- from_XML(doc.document_element, type<table<string>>)
            t |> equal(length(restored), 3)
            t |> success(restored |> key_exists("x"))
            t |> success(restored |> key_exists("y"))
            t |> success(restored |> key_exists("z"))
        }
    }
}

[test]
def test_roundtrip_set_empty(t : T?) {
    t |> run("roundtrip empty set") <| @(t : T?) {
        var original : table<int>
        let xml_str = to_XML(original, "set")
        parse_xml(xml_str) <| $(doc, ok) {
            t |> success(ok)
            var restored <- from_XML(doc.document_element, type<table<int>>)
            t |> equal(length(restored), 0)
        }
    }
}
