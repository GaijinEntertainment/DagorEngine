// Tutorial PUGIXML-01: Parsing & Navigation
//
// This tutorial covers:
//   - Parsing XML from a string with parse_xml
//   - Loading XML from a file with open_xml
//   - Iterating child elements with for_each_child and each_child
//   - Iterating attributes with for_each_attribute and each_attribute
//   - Quick accessors: node_text, node_attr, node_attr_int, node_attr_float, node_attr_bool
//   - Attribute access with operator [] and is/as type operators
//   - Text content access with xml_text is/as
//
// requires: daslib/PUGIXML_boost (re-exports pugixml)
// Run: daslang.exe tutorials/dasPUGIXML/01_parsing_and_navigation.das

options gen2
options persistent_heap

require pugixml/PUGIXML_boost

// ──────────────────────────────────────────────────────────────────────────
// Section 1 — Parsing XML from a string
// ──────────────────────────────────────────────────────────────────────────
//
// parse_xml parses an XML string inside a block. The document is freed
// automatically when the block ends — no manual delete needed.

def parsing_from_string() {
    print("=== Parsing from string ===\n")

    let xml_str = "<greeting lang=\"en\">Hello, XML!</greeting>"

    parse_xml(xml_str) <| $(doc, ok) {
        if (!ok) {
            print("parse error!\n")
            return
        }
        let root = doc.document_element
        print("tag: {root.name}\n")
        // output: tag: greeting
        print("text: {root.text as string}\n")
        // output: text: Hello, XML!
        let lang = root["lang"] as string
        print("lang: {lang}\n")
        // output: lang: en
    }
}

// ──────────────────────────────────────────────────────────────────────────
// Section 2 — Loading XML from a file
// ──────────────────────────────────────────────────────────────────────────
//
// open_xml loads an XML file. Same RAII pattern — the document is freed
// when the block finishes.

def loading_from_file() {
    print("\n=== Loading from file ===\n")

    open_xml("tutorials/dasPUGIXML/books.xml") <| $(doc, ok) {
        if (!ok) {
            print("could not open books.xml\n")
            return
        }
        let root = doc.document_element
        print("root tag: {root.name}\n")
        // output: root tag: library

        // Count children manually
        var count = 0
        var ch = root.first_child
        while (ch.ok) {
            count++
            ch = ch.next_sibling
        }
        print("books: {count}\n")
        // output: books: 3
    }
}

// ──────────────────────────────────────────────────────────────────────────
// Section 3 — Iterating children and attributes
// ──────────────────────────────────────────────────────────────────────────
//
// Block-based: for_each_child, for_each_attribute — invoke a block per
// element. Iterator-based: each_child, each_attribute — return lazy
// iterators for use in for loops.

def iterating() {
    print("\n=== Iterating children and attributes ===\n")

    let xml_str = "<config><setting name=\"width\" value=\"1920\"/><setting name=\"height\" value=\"1080\"/><setting name=\"fullscreen\" value=\"true\"/></config>"

    parse_xml(xml_str) <| $(doc, ok) {
        if (!ok) { return ; }
        let root = doc.document_element

        // Block-based — for_each_child with a callback
        print("All children (for_each_child):\n")
        root |> for_each_child() <| $(ch) {
            print("  <{ch.name}>\n")
        }
        // output: All children (for_each_child):
        // output:   <setting>
        // output:   <setting>
        // output:   <setting>

        // Iterator-based — each_child in a for loop
        print("All children (each_child):\n")
        for (ch in each_child(root)) {
            print("  <{ch.name}>\n")
        }
        // output: All children (each_child):
        // output:   <setting>
        // output:   <setting>
        // output:   <setting>

        // Block-based — for_each_attribute
        let first = root.first_child
        print("Attributes (for_each_attribute):\n")
        first |> for_each_attribute() <| $(a) {
            print("  {a.name} = {a as string}\n")
        }
        // output: Attributes (for_each_attribute):
        // output:   name = width
        // output:   value = 1920

        // Iterator-based — each_attribute
        print("Attributes (each_attribute):\n")
        for (a in each_attribute(first)) {
            print("  {a.name} = {a as string}\n")
        }
        // output: Attributes (each_attribute):
        // output:   name = width
        // output:   value = 1920
    }

    // Named child iteration — both styles
    let xml2 = "<root><a>1</a><b>2</b><a>3</a><b>4</b></root>"
    parse_xml(xml2) <| $(doc, ok) {
        if (!ok) { return ; }
        let root = doc.document_element

        print("Only <a> (for_each_child):\n")
        root |> for_each_child("a") <| $(ch) {
            print("  {ch.text as string}\n")
        }
        // output: Only <a> (for_each_child):
        // output:   1
        // output:   3

        print("Only <a> (each_child):\n")
        for (ch in each_child(root, "a")) {
            print("  {ch.text as string}\n")
        }
        // output: Only <a> (each_child):
        // output:   1
        // output:   3
    }
}

// ──────────────────────────────────────────────────────────────────────────
// Section 4 — Quick accessors
// ──────────────────────────────────────────────────────────────────────────
//
// node_text reads the text of a named child element.
// node_attr reads an attribute value as string.
// node_attr_int / node_attr_float / node_attr_bool read typed values.
// All accept a default value for missing data.

def quick_accessors() {
    print("\n=== Quick accessors ===\n")

    let xml_str = "<item id=\"42\" price=\"9.99\" active=\"true\"><name>Widget</name><desc>A useful widget</desc></item>"

    parse_xml(xml_str) <| $(doc, ok) {
        if (!ok) { return ; }
        let root = doc.document_element

        // node_text — get child element text by name
        let name = node_text(root, "name")
        print("name: {name}\n")
        // output: name: Widget

        let desc = node_text(root, "desc", "no description")
        print("desc: {desc}\n")
        // output: desc: A useful widget

        let missing = node_text(root, "category", "uncategorized")
        print("category: {missing}\n")
        // output: category: uncategorized

        // node_attr — get attribute as string
        let id_str = node_attr(root, "id")
        print("id (string): {id_str}\n")
        // output: id (string): 42

        // Typed attribute access
        let id = node_attr_int(root, "id")
        print("id (int): {id}\n")
        // output: id (int): 42

        let price = node_attr_float(root, "price")
        print("price: {price}\n")
        // output: price: 9.99

        let active = node_attr_bool(root, "active")
        print("active: {active}\n")
        // output: active: true

        // Defaults for missing attributes
        let weight = node_attr_float(root, "weight", 1.0)
        print("weight (default): {weight}\n")
        // output: weight (default): 1
    }
}

// ──────────────────────────────────────────────────────────────────────────
// Section 5 — Attribute access with operator [] and is/as
// ──────────────────────────────────────────────────────────────────────────
//
// PUGIXML_boost overloads operator [] on xml_node to return an
// xml_attribute. The is/as operators convert attributes and text
// values to typed values.

def typed_access() {
    print("\n=== Typed access with is/as ===\n")

    let xml_str = "<point x=\"10\" y=\"20\" label=\"origin\" visible=\"true\"/>"

    parse_xml(xml_str) <| $(doc, ok) {
        if (!ok) { return ; }
        let root = doc.document_element

        // operator [] returns xml_attribute
        let x_attr = root["x"]
        print("x exists: {x_attr.ok}\n")
        // output: x exists: true

        // as converts the attribute value
        let x_val = root["x"] as int
        print("x as int: {x_val}\n")
        // output: x as int: 10
        let y_val = root["y"] as float
        print("y as float: {y_val}\n")
        // output: y as float: 20
        let label_val = root["label"] as string
        print("label as string: {label_val}\n")
        // output: label as string: origin
        let vis_val = root["visible"] as bool
        print("visible as bool: {vis_val}\n")
        // output: visible as bool: true

        // Missing attributes return defaults
        let z_val = root["z"] as int
        print("z as int: {z_val}\n")
        // output: z as int: 0
    }

    // Text content is/as
    let xml2 = "<values><count>42</count><ratio>3.14</ratio><flag>true</flag></values>"
    parse_xml(xml2) <| $(doc, ok) {
        if (!ok) { return ; }
        let root = doc.document_element

        let count_node = child(root, "count")
        print("count text is int: {count_node.text is int}\n")
        // output: count text is int: true
        print("count text as int: {count_node.text as int}\n")
        // output: count text as int: 42

        let ratio_node = child(root, "ratio")
        print("ratio as float: {ratio_node.text as float}\n")
        // output: ratio as float: 3.14

        let flag_node = child(root, "flag")
        print("flag as bool: {flag_node.text as bool}\n")
        // output: flag as bool: true
    }
}

// ──────────────────────────────────────────────────────────────────────────
// Section 6 — Practical example: reading the books catalog
// ──────────────────────────────────────────────────────────────────────────

def reading_books() {
    print("\n=== Reading books catalog ===\n")

    open_xml("tutorials/dasPUGIXML/books.xml") <| $(doc, ok) {
        if (!ok) {
            print("could not open books.xml\n")
            return
        }
        let library = doc.document_element

        library |> for_each_child("book") <| $(book) {
            let title = node_text(book, "title")
            let year = node_text(book, "year")
            let isbn = book["isbn"] as string
            let price = child(book, "price").text as float

            // Count authors
            var author_count = 0
            book |> for_each_child("author") <| $(a) {
                author_count++
            }

            print("{title} ({year}) - ISBN {isbn}, ${price}, {author_count} author(s)\n")
        }
        // output: The C Programming Language (1988) - ISBN 978-0131103627, $67.99, 2 author(s)
        // output: Design Patterns (1994) - ISBN 978-0201633610, $54.99, 4 author(s)
        // output: Head First Design Patterns (2004) - ISBN 978-0596007126, $39.99, 2 author(s)
    }
}

[export]
def main() {
    parsing_from_string()
    loading_from_file()
    iterating()
    quick_accessors()
    typed_access()
    reading_books()
}
