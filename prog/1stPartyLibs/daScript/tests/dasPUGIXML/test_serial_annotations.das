options gen2
require pugixml/PUGIXML_boost
require dastest/testing_boost public

// ============ Test types ============

struct Point {
    x : float
    y : float
}

enum Priority {
    low
    medium
    high
}

struct RenamedFields {
    @rename = "type" _type : string
    @rename = "class" _class : int
    value : float
}

struct EnumIntFields {
    normal : Priority
    @enum_as_int as_int : Priority
}

struct MixedAnnotations {
    @rename = "display_name" name : string
    @enum_as_int pri : Priority
    score : int
}

// ============ Annotation tests ============

[test]
def test_xml_rename_serialize(t : T?) {
    t |> run("rename annotation changes element name in XML") <| @(t : T?) {
        let s = RenamedFields(_type = "widget", _class = 3, value = 1.5)
        with_doc() <| $(doc) {
            var dnode = doc as xml_node
            var root = append_child(dnode, "item")
            XML(root, s)
            // Fields should use renamed element names
            t |> success(child(root, "type").ok)
            t |> success(child(root, "class").ok)
            t |> success(child(root, "value").ok)
            // Original names should NOT exist
            t |> success(!child(root, "_type").ok)
            t |> success(!child(root, "_class").ok)
        }
    }
}

[test]
def test_xml_rename_deserialize(t : T?) {
    t |> run("rename annotation reads from renamed element") <| @(t : T?) {
        let xml_str = "<item><type>button</type><class>5</class><value>2.0</value></item>"
        parse_xml(xml_str) <| $(doc, ok) {
            t |> success(ok)
            let result = from_XML(doc.document_element, type<RenamedFields>)
            t |> equal(result._type, "button")
            t |> equal(result._class, 5)
            t |> equal(result.value, 2.0)
        }
    }
}

[test]
def test_xml_rename_roundtrip(t : T?) {
    t |> run("rename annotation roundtrips correctly") <| @(t : T?) {
        let original = RenamedFields(_type = "dialog", _class = 7, value = 3.14)
        let xml_str = to_XML(original, "item")
        parse_xml(xml_str) <| $(doc, ok) {
            t |> success(ok)
            let restored = from_XML(doc.document_element, type<RenamedFields>)
            t |> equal(restored._type, original._type)
            t |> equal(restored._class, original._class)
            t |> equal(restored.value, original.value)
        }
    }
}

[test]
def test_xml_enum_as_int_serialize(t : T?) {
    t |> run("enum_as_int writes integer value") <| @(t : T?) {
        let s = EnumIntFields(normal = Priority.high, as_int = Priority.medium)
        with_doc() <| $(doc) {
            var dnode = doc as xml_node
            var root = append_child(dnode, "item")
            XML(root, s)
            // normal enum serializes as string name
            t |> equal(as_string(child(root, "normal").text, ""), "high")
            // @enum_as_int serializes as integer
            t |> equal(as_int(child(root, "as_int").text, -1), 1)
        }
    }
}

[test]
def test_xml_enum_as_int_zero(t : T?) {
    t |> run("enum_as_int zero value") <| @(t : T?) {
        let s = EnumIntFields(normal = Priority.low, as_int = Priority.low)
        with_doc() <| $(doc) {
            var dnode = doc as xml_node
            var root = append_child(dnode, "item")
            XML(root, s)
            t |> equal(as_string(child(root, "normal").text, ""), "low")
            t |> equal(as_int(child(root, "as_int").text, -1), 0)
        }
    }
}

[test]
def test_xml_mixed_annotations_serialize(t : T?) {
    t |> run("mixed annotations: rename + enum_as_int") <| @(t : T?) {
        let s = MixedAnnotations(name = "test", pri = Priority.high, score = 42)
        with_doc() <| $(doc) {
            var dnode = doc as xml_node
            var root = append_child(dnode, "item")
            XML(root, s)
            // @rename = "display_name" on name field
            t |> success(child(root, "display_name").ok)
            t |> success(!child(root, "name").ok)
            t |> equal(as_string(child(root, "display_name").text, ""), "test")
            // @enum_as_int on pri
            t |> equal(as_int(child(root, "pri").text, -1), 2)
            // no annotation on score
            t |> equal(as_int(child(root, "score").text, -1), 42)
        }
    }
}

[test]
def test_xml_mixed_annotations_roundtrip(t : T?) {
    t |> run("mixed annotations roundtrip") <| @(t : T?) {
        let original = MixedAnnotations(name = "config", pri = Priority.medium, score = 99)
        let xml_str = to_XML(original, "item")
        // Verify the XML uses renamed elements
        parse_xml(xml_str) <| $(doc, ok) {
            t |> success(ok)
            let root = doc.document_element
            t |> success(child(root, "display_name").ok)
            t |> equal(as_string(child(root, "display_name").text, ""), "config")
            // Deserialize back (from_XML also respects @rename)
            let restored = from_XML(root, type<MixedAnnotations>)
            t |> equal(restored.name, original.name)
            t |> equal(restored.pri, original.pri)
            t |> equal(restored.score, original.score)
        }
    }
}

[test]
def test_xml_annotation_defaults(t : T?) {
    t |> run("fields without annotations still work") <| @(t : T?) {
        let p = Point(x = 1.0, y = 2.0)
        let xml_str = to_XML(p, "point")
        parse_xml(xml_str) <| $(doc, ok) {
            t |> success(ok)
            let restored = from_XML(doc.document_element, type<Point>)
            t |> equal(restored.x, 1.0)
            t |> equal(restored.y, 2.0)
        }
    }
}
