options gen2
require pugixml/PUGIXML_boost
require math
require dastest/testing_boost public

// ============ Test structs ============

struct Point {
    x : float
    y : float
}

struct Person {
    name : string
    age : int
    active : bool
}

struct Book {
    title : string
    year : int
    price : float
}

struct Library {
    name : string
    books : array<Book>
}

struct Address {
    city : string
    zip : int
}

struct Employee {
    name : string
    address : Address
}

// ============ XML() serialize tests ============

[test]
def test_xml_serialize_simple(t : T?) {
    t |> run("serialize simple struct fields to XML") <| @(t : T?) {
        let p = Person(name = "Alice", age = 30, active = true)
        with_doc() <| $(doc) {
            var dnode = doc as xml_node
            var root = append_child(dnode, "person")
            XML(root, p)
            t |> equal(node_text(root, "name"), "Alice")
            t |> equal(child(root, "age").text as int, 30)
            t |> equal(child(root, "active").text as bool, true)
        }
    }
}

[test]
def test_xml_serialize_floats(t : T?) {
    t |> run("serialize float fields") <| @(t : T?) {
        let pt = Point(x = 3.14, y = 2.72)
        with_doc() <| $(doc) {
            var dnode = doc as xml_node
            var root = append_child(dnode, "point")
            XML(root, pt)
            t |> success(abs(child(root, "x").text as float -3.14) < 0.01)
            t |> success(abs(child(root, "y").text as float -2.72) < 0.01)
        }
    }
}

[test]
def test_xml_serialize_nested(t : T?) {
    t |> run("serialize nested struct") <| @(t : T?) {
        let emp = Employee(name = "Bob", address = Address(city = "NYC", zip = 10001))
        with_doc() <| $(doc) {
            var dnode = doc as xml_node
            var root = append_child(dnode, "employee")
            XML(root, emp)
            t |> equal(node_text(root, "name"), "Bob")
            let address_node = child(root, "address")
            t |> success(address_node.ok)
            t |> equal(node_text(address_node, "city"), "NYC")
            t |> equal(child(address_node, "zip").text as int, 10001)
        }
    }
}

[test]
def test_xml_serialize_array(t : T?) {
    t |> run("serialize struct with array field") <| @(t : T?) {
        var lib : Library
        lib.name = "City Library"
        lib.books |> push_clone(Book(title = "Book A", year = 2000, price = 19.99))
        lib.books |> push_clone(Book(title = "Book B", year = 2010, price = 29.99))
        with_doc() <| $(doc) {
            var dnode = doc as xml_node
            var root = append_child(dnode, "library")
            XML(root, lib)
            t |> equal(node_text(root, "name"), "City Library")
            let books_node = child(root, "books")
            t |> success(books_node.ok)
            // Array items are children named "item"
            let b1 = books_node.first_child
            t |> success(b1.ok)
            t |> equal(node_text(b1, "title"), "Book A")
            t |> equal(child(b1, "year").text as int, 2000)
            let b2 = b1.next_sibling
            t |> success(b2.ok)
            t |> equal(node_text(b2, "title"), "Book B")
            t |> equal(child(b2, "year").text as int, 2010)
        }
    }
}

[test]
def test_xml_convenience_string(t : T?) {
    t |> run("to_XML(value, root) returns XML string") <| @(t : T?) {
        let p = Point(x = 1.0, y = 2.0)
        let xml_str = to_XML(p, "point")
        parse_xml(xml_str) <| $(doc, ok) {
            t |> success(ok)
            let root = doc.document_element
            t |> equal(root.name, "point")
            t |> success(abs(child(root, "x").text as float -1.0) < 0.01)
            t |> success(abs(child(root, "y").text as float -2.0) < 0.01)
        }
    }
}

[test]
def test_xml_convenience_default_root(t : T?) {
    t |> run("to_XML(value) uses 'root' as default element name") <| @(t : T?) {
        let p = Person(name = "Test", age = 1, active = false)
        let xml_str = to_XML(p)
        parse_xml(xml_str) <| $(doc, ok) {
            t |> success(ok)
            t |> equal(doc.document_element.name, "root")
        }
    }
}

// ============ from_XML() deserialize tests ============

[test]
def test_from_xml_simple(t : T?) {
    t |> run("deserialize simple struct from XML") <| @(t : T?) {
        parse_xml("<person><name>Bob</name><age>25</age><active>false</active></person>") <| $(doc, ok) {
            t |> success(ok)
            let p = from_XML(doc.document_element, type<Person>)
            t |> equal(p.name, "Bob")
            t |> equal(p.age, 25)
            t |> equal(p.active, false)
        }
    }
}

[test]
def test_from_xml_floats(t : T?) {
    t |> run("deserialize float fields") <| @(t : T?) {
        parse_xml("<pt><x>3.14</x><y>2.72</y></pt>") <| $(doc, ok) {
            t |> success(ok)
            let pt = from_XML(doc.document_element, type<Point>)
            t |> success(abs(pt.x - 3.14) < 0.01)
            t |> success(abs(pt.y - 2.72) < 0.01)
        }
    }
}

[test]
def test_from_xml_nested(t : T?) {
    t |> run("deserialize nested struct") <| @(t : T?) {
        let xml = "<emp><name>Carol</name><address><city>LA</city><zip>90001</zip></address></emp>"
        parse_xml(xml) <| $(doc, ok) {
            t |> success(ok)
            let emp = from_XML(doc.document_element, type<Employee>)
            t |> equal(emp.name, "Carol")
            t |> equal(emp.address.city, "LA")
            t |> equal(emp.address.zip, 90001)
        }
    }
}

[test]
def test_from_xml_array(t : T?) {
    t |> run("deserialize struct with array") <| @(t : T?) {
        let xml = "<lib><name>Town Lib</name><books><item><title>Alpha</title><year>2001</year><price>9.99</price></item><item><title>Beta</title><year>2002</year><price>19.99</price></item></books></lib>"
        parse_xml(xml) <| $(doc, ok) {
            t |> success(ok)
            let lib = from_XML(doc.document_element, type<Library>)
            t |> equal(lib.name, "Town Lib")
            t |> equal(length(lib.books), 2)
            t |> equal(lib.books[0].title, "Alpha")
            t |> equal(lib.books[0].year, 2001)
            t |> equal(lib.books[1].title, "Beta")
            t |> equal(lib.books[1].year, 2002)
        }
    }
}

[test]
def test_from_xml_missing_fields(t : T?) {
    t |> run("missing XML elements yield default values") <| @(t : T?) {
        parse_xml("<person><name>Partial</name></person>") <| $(doc, ok) {
            t |> success(ok)
            let p = from_XML(doc.document_element, type<Person>)
            t |> equal(p.name, "Partial")
            t |> equal(p.age, 0)
            t |> equal(p.active, false)
        }
    }
}

[test]
def test_from_xml_empty_array(t : T?) {
    t |> run("empty container element yields empty array") <| @(t : T?) {
        parse_xml("<lib><name>Empty</name><books></books></lib>") <| $(doc, ok) {
            t |> success(ok)
            let lib = from_XML(doc.document_element, type<Library>)
            t |> equal(lib.name, "Empty")
            t |> equal(length(lib.books), 0)
        }
    }
}

// ============ Roundtrip tests ============

[test]
def test_roundtrip_simple(t : T?) {
    t |> run("roundtrip struct -> XML -> struct") <| @(t : T?) {
        let original = Person(name = "Diana", age = 42, active = true)
        let xml_str = to_XML(original, "person")
        parse_xml(xml_str) <| $(doc, ok) {
            t |> success(ok)
            let restored = from_XML(doc.document_element, type<Person>)
            t |> equal(restored.name, original.name)
            t |> equal(restored.age, original.age)
            t |> equal(restored.active, original.active)
        }
    }
}

[test]
def test_roundtrip_nested(t : T?) {
    t |> run("roundtrip nested struct") <| @(t : T?) {
        let original = Employee(name = "Eve", address = Address(city = "SF", zip = 94102))
        let xml_str = to_XML(original, "employee")
        parse_xml(xml_str) <| $(doc, ok) {
            t |> success(ok)
            let restored = from_XML(doc.document_element, type<Employee>)
            t |> equal(restored.name, original.name)
            t |> equal(restored.address.city, original.address.city)
            t |> equal(restored.address.zip, original.address.zip)
        }
    }
}

[test]
def test_roundtrip_with_array(t : T?) {
    t |> run("roundtrip struct with array field") <| @(t : T?) {
        var original : Library
        original.name = "Round Lib"
        original.books |> push_clone(Book(title = "Gamma", year = 2020, price = 39.99))
        original.books |> push_clone(Book(title = "Delta", year = 2021, price = 49.99))
        let xml_str = to_XML(original, "library")
        parse_xml(xml_str) <| $(doc, ok) {
            t |> success(ok)
            let restored = from_XML(doc.document_element, type<Library>)
            t |> equal(restored.name, original.name)
            t |> equal(length(restored.books), 2)
            t |> equal(restored.books[0].title, "Gamma")
            t |> equal(restored.books[0].year, 2020)
            t |> success(abs(restored.books[0].price - 39.99) < 0.01)
            t |> equal(restored.books[1].title, "Delta")
            t |> equal(restored.books[1].year, 2021)
        }
    }
}

[test]
def test_roundtrip_floats(t : T?) {
    t |> run("roundtrip preserves float precision") <| @(t : T?) {
        let original = Point(x = 1.5, y = -3.25)
        let xml_str = to_XML(original, "point")
        parse_xml(xml_str) <| $(doc, ok) {
            t |> success(ok)
            let restored = from_XML(doc.document_element, type<Point>)
            t |> success(abs(restored.x - original.x) < 0.001)
            t |> success(abs(restored.y - original.y) < 0.001)
        }
    }
}

// ============ from_XML(string) convenience tests ============

[test]
def test_from_xml_string_simple(t : T?) {
    t |> run("from_XML(string) one-step deserialize") <| @(t : T?) {
        let xml_str = "<person><name>Alice</name><age>30</age><active>true</active></person>"
        let p = from_XML(xml_str, type<Person>)
        t |> equal(p.name, "Alice")
        t |> equal(p.age, 30)
        t |> equal(p.active, true)
    }
}

[test]
def test_from_xml_string_nested(t : T?) {
    t |> run("from_XML(string) nested struct") <| @(t : T?) {
        let xml_str = "<e><name>Bob</name><address><city>NYC</city><zip>10001</zip></address></e>"
        let emp = from_XML(xml_str, type<Employee>)
        t |> equal(emp.name, "Bob")
        t |> equal(emp.address.city, "NYC")
        t |> equal(emp.address.zip, 10001)
    }
}

[test]
def test_from_xml_string_roundtrip(t : T?) {
    t |> run("to_XML then from_XML(string) roundtrip") <| @(t : T?) {
        let original = Person(name = "Diana", age = 42, active = true)
        let xml_str = to_XML(original, "person")
        let restored = from_XML(xml_str, type<Person>)
        t |> equal(restored.name, original.name)
        t |> equal(restored.age, original.age)
        t |> equal(restored.active, original.active)
    }
}

[test]
def test_from_xml_string_with_array(t : T?) {
    t |> run("from_XML(string) struct with array") <| @(t : T?) {
        var original : Library
        original.name = "Test Lib"
        original.books |> push_clone(Book(title = "A", year = 2020, price = 10.0))
        original.books |> push_clone(Book(title = "B", year = 2021, price = 20.0))
        let xml_str = to_XML(original, "lib")
        let restored = from_XML(xml_str, type<Library>)
        t |> equal(restored.name, "Test Lib")
        t |> equal(length(restored.books), 2)
        t |> equal(restored.books[0].title, "A")
        t |> equal(restored.books[1].title, "B")
    }
}
