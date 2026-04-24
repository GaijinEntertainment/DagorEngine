// Tutorial PUGIXML-02: Building XML
//
// This tutorial covers:
//   - Creating documents from scratch with with_doc
//   - The tag() builder EDSL — pipe + block nesting
//   - Attribute chaining with attr()
//   - Builder helpers: add_child, add_child_ex, add_attr
//   - Serializing documents and nodes to strings with to_string
//   - Converting nodes to strings with operator as string
//
// requires: daslib/PUGIXML_boost (re-exports pugixml)
// Run: daslang.exe tutorials/dasPUGIXML/02_building_xml.das

options gen2
options persistent_heap

require pugixml/PUGIXML_boost

// ──────────────────────────────────────────────────────────────────────────
// Section 1 — Creating a document with with_doc
// ──────────────────────────────────────────────────────────────────────────
//
// with_doc creates an empty xml_document, invokes a block with it,
// then automatically frees the document. No manual delete needed.

def creating_document() {
    print("=== Creating a document ===\n")

    with_doc() <| $(doc) {
        // Add a root element using the low-level pugixml API
        var dnode = doc as xml_node
        var root = append_child(dnode, "greeting")
        set(root.text, "Hello, World!")
        print(to_string(doc))
    }
    // output: <?xml version="1.0"?>
    // output: <greeting>Hello, World!</greeting>
}

// ──────────────────────────────────────────────────────────────────────────
// Section 2 — The tag() builder EDSL
// ──────────────────────────────────────────────────────────────────────────
//
// tag() is the core builder function. It creates child elements using
// pipe + block syntax for nesting. Three forms:
//   tag(name, block)  — create element, invoke block for content
//   tag(name, text)   — create element with text content
//   tag(name)         — create bare element
//
// tag() works on both xml_document? and xml_node.

def tag_builder() {
    print("\n=== Tag builder EDSL ===\n")

    with_doc() <| $(doc) {
        // Build nested XML with pipe + block
        doc |> tag("library") <| $(var library) {
            library |> tag("book") <| $(var book) {
                book |> tag("title", "The C Programming Language")
                book |> tag("author", "Brian W. Kernighan")
                book |> tag("year", "1988")
            }
            library |> tag("book") <| $(var book) {
                book |> tag("title", "Structure and Interpretation")
                book |> tag("author", "Harold Abelson")
                book |> tag("year", "1996")
            }
        }
        print(to_string(doc))
    }
    // output: <?xml version="1.0"?>
    // output: <library>
    // output:   <book>
    // output:     <title>The C Programming Language</title>
    // output:     <author>Brian W. Kernighan</author>
    // output:     <year>1988</year>
    // output:   </book>
    // output:   <book>
    // output:     <title>Structure and Interpretation</title>
    // output:     <author>Harold Abelson</author>
    // output:     <year>1996</year>
    // output:   </book>
    // output: </library>
}

// ──────────────────────────────────────────────────────────────────────────
// Section 3 — Attribute chaining with attr()
// ──────────────────────────────────────────────────────────────────────────
//
// attr() appends an attribute and returns the parent node, enabling
// fluent chaining with |> pipes. Overloads for string, int, float, bool.

def attribute_chaining() {
    print("\n=== Attribute chaining ===\n")

    with_doc() <| $(doc) {
        doc |> tag("config") <| $(var config) {
            // Chain multiple attributes on one element
            var res = config |> tag("display")
            res |> attr("width", 1920) |> attr("height", 1080) |> attr("fullscreen", true)

            var audio = config |> tag("audio")
            audio |> attr("volume", 0.8) |> attr("muted", false)
        }
        print(to_string(doc))
    }
    // output: <?xml version="1.0"?>
    // output: <config>
    // output:   <display width="1920" height="1080" fullscreen="true" />
    // output:   <audio volume="0.800000012" muted="false" />
    // output: </config>
}

// ──────────────────────────────────────────────────────────────────────────
// Section 4 — Combining tag + attr
// ──────────────────────────────────────────────────────────────────────────
//
// tag() and attr() compose naturally for real-world XML documents.

def combined_building() {
    print("\n=== Combined tag + attr ===\n")

    with_doc() <| $(doc) {
        doc |> tag("catalog") <| $(var catalog) {
            catalog |> tag("product") <| $(var p) {
                p |> attr("id", "A001") |> attr("category", "electronics")
                p |> tag("name", "Wireless Mouse")
                p |> tag("price", "29.99")
            }
            catalog |> tag("product") <| $(var p) {
                p |> attr("id", "B002") |> attr("category", "books")
                p |> tag("name", "daslang Handbook")
                p |> tag("price", "49.99")
            }
        }
        print(to_string(doc))
    }
}

// ──────────────────────────────────────────────────────────────────────────
// Section 5 — Builder helpers: add_child, add_child_ex, add_attr
// ──────────────────────────────────────────────────────────────────────────
//
// These lower-level helpers are useful when you want to add elements
// or attributes without the pipe+block pattern.

def builder_helpers() {
    print("\n=== Builder helpers ===\n")

    with_doc() <| $(doc) {
        var dnode = doc as xml_node
        var root = append_child(dnode, "record")

        // add_child — element with text content
        add_child(root, "name", "Alice")
        add_child(root, "age", "30")

        // add_child_ex — element with one attribute
        add_child_ex(root, "skill", "name", "daslang")

        // add_attr — typed attributes
        add_attr(root, "id", "user-001")
        add_attr(root, "score", 95)
        add_attr(root, "rating", 4.5)
        add_attr(root, "active", true)

        print(to_string(doc))
    }
    // output: <?xml version="1.0"?>
    // output: <record id="user-001" score="95" rating="4.5" active="true">
    // output:   <name>Alice</name>
    // output:   <age>30</age>
    // output:   <skill name="daslang" />
    // output: </record>
}

// ──────────────────────────────────────────────────────────────────────────
// Section 6 — Serializing to string
// ──────────────────────────────────────────────────────────────────────────
//
// to_string works on both xml_document? and xml_node.
// operator as string converts an xml_node to its XML representation.

def serializing() {
    print("\n=== Serializing to string ===\n")

    with_doc() <| $(doc) {
        doc |> tag("root") <| $(var root) {
            root |> tag("child", "hello")
        }

        // Whole document — includes <?xml version="1.0"?>
        let doc_str = to_string(doc)
        print("Document:\n{doc_str}")

        // Single node — no XML declaration
        let root_node = doc.document_element
        let node_str = to_string(root_node)
        print("Node only:\n{node_str}")

        // operator as string on xml_node — same as to_string(node)
        let via_as = root_node as string
        print("Via as string:\n{via_as}")
    }
}

// ──────────────────────────────────────────────────────────────────────────
// Section 7 — Building XML programmatically with loops
// ──────────────────────────────────────────────────────────────────────────

def dynamic_building() {
    print("\n=== Dynamic building ===\n")

    let names = fixed_array("Alice", "Bob", "Charlie")
    let scores = fixed_array(95, 87, 92)

    with_doc() <| $(doc) {
        doc |> tag("leaderboard") <| $(var board) {
            for (i in range(3)) {
                board |> tag("player") <| $(var p) {
                    p |> attr("rank", i + 1)
                    p |> tag("name", names[i])
                    p |> tag("score", "{scores[i]}")
                }
            }
        }
        print(to_string(doc))
    }
}

[export]
def main() {
    creating_document()
    tag_builder()
    attribute_chaining()
    combined_building()
    builder_helpers()
    serializing()
    dynamic_building()
}
