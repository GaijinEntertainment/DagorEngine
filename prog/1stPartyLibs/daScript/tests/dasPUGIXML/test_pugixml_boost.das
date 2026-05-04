options gen2
require pugixml/PUGIXML_boost
require math
require strings
require dastest/testing_boost public

[test]
def test_for_each_child(t : T?) {
    parse_xml("<root><a>1</a><b>2</b><c>3</c></root>") <| $(doc, ok) {
        t |> success(ok)
        let root = doc.document_element
        var names : array<string>
        for_each_child(root) <| $(ch) {
            names |> push(ch.name)
        }
        t |> equal(length(names), 3)
        t |> equal(names[0], "a")
        t |> equal(names[1], "b")
        t |> equal(names[2], "c")
    }
}

[test]
def test_for_each_child_named(t : T?) {
    parse_xml("<root><item>1</item><other/><item>2</item><item>3</item></root>") <| $(doc, ok) {
        t |> success(ok)
        let root = doc.document_element
        var count = 0
        for_each_child(root, "item") <| $(ch) {
            count++
            t |> success(ch.ok)
        }
        t |> equal(count, 3)
    }
}

[test]
def test_for_each_attribute(t : T?) {
    parse_xml("<root x=\"1\" y=\"2\" z=\"3\"/>") <| $(doc, ok) {
        t |> success(ok)
        let root = doc.document_element
        var attrs : array<string>
        for_each_attribute(root) <| $(a) {
            attrs |> push(a.name)
        }
        t |> equal(length(attrs), 3)
        t |> equal(attrs[0], "x")
        t |> equal(attrs[1], "y")
        t |> equal(attrs[2], "z")
    }
}

[test]
def test_for_each_xpath(t : T?) {
    parse_xml("<root><item key=\"a\"/><item key=\"b\"/></root>") <| $(doc, ok) {
        t |> success(ok)
        var ns = select_nodes(doc.document_element, "item")
        var count = 0
        for_each(ns) <| $(xn) {
            count++
            t |> success(xn.ok)
        }
        t |> equal(count, 2)
        unsafe { delete ns; }
    }
}

[test]
def test_each_xpath(t : T?) {
    t |> run("basic iteration") <| @(t : T?) {
        parse_xml("<root><a>1</a><b>2</b><c>3</c></root>") <| $(doc, ok) {
            t |> success(ok)
            var ns = select_nodes(doc.document_element, "*")
            var names : array<string>
            for (xn in ns) {
                names |> push(xn.node.name)
            }
            t |> equal(length(names), 3)
            t |> equal(names[0], "a")
            t |> equal(names[1], "b")
            t |> equal(names[2], "c")
            unsafe { delete ns; }
        }
    }
    t |> run("empty node set") <| @(t : T?) {
        parse_xml("<root/>") <| $(doc, ok) {
            t |> success(ok)
            var ns = select_nodes(doc.document_element, "missing")
            var count = 0
            for (xn in ns) {
                count++
            }
            t |> equal(count, 0)
            unsafe { delete ns; }
        }
    }
    t |> run("attribute access via each") <| @(t : T?) {
        parse_xml("<root><item key=\"x\"/><item key=\"y\"/><item key=\"z\"/></root>") <| $(doc, ok) {
            t |> success(ok)
            var ns = select_nodes(doc.document_element, "item")
            var keys : array<string>
            for (xn in ns) {
                keys |> push(node_attr(xn.node, "key"))
            }
            t |> equal(length(keys), 3)
            t |> equal(keys[0], "x")
            t |> equal(keys[1], "y")
            t |> equal(keys[2], "z")
            unsafe { delete ns; }
        }
    }
}

[test]
def test_each_child(t : T?) {
    t |> run("all children") <| @(t : T?) {
        parse_xml("<root><a>1</a><b>2</b><c>3</c></root>") <| $(doc, ok) {
            t |> success(ok)
            let root = doc.document_element
            var names : array<string>
            for (ch in each_child(root)) {
                names |> push(ch.name)
            }
            t |> equal(length(names), 3)
            t |> equal(names[0], "a")
            t |> equal(names[1], "b")
            t |> equal(names[2], "c")
        }
    }
    t |> run("empty node") <| @(t : T?) {
        parse_xml("<root/>") <| $(doc, ok) {
            t |> success(ok)
            var count = 0
            for (ch in each_child(doc.document_element)) {
                count++
            }
            t |> equal(count, 0)
        }
    }
}

[test]
def test_each_child_named(t : T?) {
    t |> run("filtered iteration") <| @(t : T?) {
        parse_xml("<root><item>1</item><other/><item>2</item><item>3</item></root>") <| $(doc, ok) {
            t |> success(ok)
            let root = doc.document_element
            var texts : array<string>
            for (ch in each_child(root, "item")) {
                texts |> push(ch.text as string)
            }
            t |> equal(length(texts), 3)
            t |> equal(texts[0], "1")
            t |> equal(texts[1], "2")
            t |> equal(texts[2], "3")
        }
    }
    t |> run("no matching children") <| @(t : T?) {
        parse_xml("<root><a/><b/></root>") <| $(doc, ok) {
            t |> success(ok)
            var count = 0
            for (ch in each_child(doc.document_element, "item")) {
                count++
            }
            t |> equal(count, 0)
        }
    }
}

[test]
def test_each_attribute(t : T?) {
    t |> run("all attributes") <| @(t : T?) {
        parse_xml("<root x=\"1\" y=\"2\" z=\"3\"/>") <| $(doc, ok) {
            t |> success(ok)
            let root = doc.document_element
            var attrs : array<string>
            for (a in each_attribute(root)) {
                attrs |> push(a.name)
            }
            t |> equal(length(attrs), 3)
            t |> equal(attrs[0], "x")
            t |> equal(attrs[1], "y")
            t |> equal(attrs[2], "z")
        }
    }
    t |> run("attribute values via iterator") <| @(t : T?) {
        parse_xml("<point x=\"10\" y=\"20\"/>") <| $(doc, ok) {
            t |> success(ok)
            var total = 0
            for (a in each_attribute(doc.document_element)) {
                total += a as int
            }
            t |> equal(total, 30)
        }
    }
    t |> run("no attributes") <| @(t : T?) {
        parse_xml("<empty/>") <| $(doc, ok) {
            t |> success(ok)
            var count = 0
            for (a in each_attribute(doc.document_element)) {
                count++
            }
            t |> equal(count, 0)
        }
    }
}

[test]
def test_parse_xml(t : T?) {
    parse_xml("<hello>world</hello>") <| $(doc, ok) {
        t |> success(ok)
        let root = doc.document_element
        t |> equal(root.name, "hello")
        t |> equal(root.text as string, "world")
    }
}

[test]
def test_with_doc(t : T?) {
    with_doc() <| $(doc) {
        var root = append_child(doc as xml_node, "test")
        add_child(root, "msg", "hello")
        let txt = node_text(root, "msg")
        t |> equal(txt, "hello")
    }
}

[test]
def test_node_text(t : T?) {
    parse_xml("<root><title>Hello World</title><empty/></root>") <| $(doc, ok) {
        t |> success(ok)
        let root = doc.document_element
        t |> equal(node_text(root, "title"), "Hello World")
        t |> equal(node_text(root, "empty"), "")
        t |> equal(node_text(root, "missing", "fallback"), "fallback")
    }
}

[test]
def test_node_attr(t : T?) {
    parse_xml("<root name=\"test\" count=\"42\" ratio=\"3.14\" flag=\"true\"/>") <| $(doc, ok) {
        t |> success(ok)
        let root = doc.document_element
        t |> equal(node_attr(root, "name"), "test")
        t |> equal(node_attr(root, "missing", "default"), "default")
        t |> equal(node_attr_int(root, "count"), 42)
        t |> equal(node_attr_int(root, "missing", -1), -1)
        t |> success(abs(node_attr_float(root, "ratio") - 3.14) < 0.01)
        t |> equal(node_attr_bool(root, "flag"), true)
    }
}

[test]
def test_add_child(t : T?) {
    with_doc() <| $(doc) {
        var root = append_child(doc as xml_node, "catalog")
        add_child(root, "title", "My Book")
        add_child_ex(root, "item", "id", "42")
        t |> equal(node_text(root, "title"), "My Book")
        let item = child(root, "item")
        t |> equal(node_attr(item, "id"), "42")
    }
}

[test]
def test_add_attr(t : T?) {
    with_doc() <| $(doc) {
        var root = append_child(doc as xml_node, "node")
        add_attr(root, "name", "test")
        add_attr(root, "count", 42)
        add_attr(root, "ratio", 3.14)
        add_attr(root, "flag", true)
        t |> equal(node_attr(root, "name"), "test")
        t |> equal(node_attr_int(root, "count"), 42)
        t |> success(abs(node_attr_float(root, "ratio") - 3.14) < 0.01)
        t |> equal(node_attr_bool(root, "flag"), true)
    }
}

[test]
def test_to_string_doc(t : T?) {
    with_doc() <| $(doc) {
        var root = append_child(doc as xml_node, "root")
        add_child(root, "msg", "hello")
        let s = to_string(doc)
        t |> success(length(s) > 0)
        // Should contain the tag name
        t |> success(find(s, "root") >= 0)
        t |> success(find(s, "hello") >= 0)
    }
}

[test]
def test_to_string_node(t : T?) {
    parse_xml("<root><item>text</item></root>") <| $(doc, ok) {
        t |> success(ok)
        let item = child(doc.document_element, "item")
        let s = to_string(item)
        t |> success(find(s, "item") >= 0)
        t |> success(find(s, "text") >= 0)
    }
}

[test]
def test_with_xpath(t : T?) {
    parse_xml("<root><item>hello</item></root>") <| $(doc, ok) {
        t |> success(ok)
        let root = doc.document_element
        with_xpath("//item") <| $(q) {
            t |> success(q.ok)
            let result = evaluate_node(q, root)
            t |> equal(result.node.name, "item")
        }
    }
}

[test]
def test_select_text(t : T?) {
    parse_xml("<root><a><b>deep</b></a></root>") <| $(doc, ok) {
        t |> success(ok)
        let root = doc.document_element
        t |> equal(select_text(root, "a/b"), "deep")
        t |> equal(select_text(root, "missing", "none"), "none")
    }
}

[test]
def test_select_value_element(t : T?) {
    parse_xml("<root><title>hello</title></root>") <| $(doc, ok) {
        t |> success(ok)
        t |> equal(select_value(doc.document_element, "title"), "hello")
    }
}

[test]
def test_select_value_attribute(t : T?) {
    parse_xml("<root><item key=\"val\"/></root>") <| $(doc, ok) {
        t |> success(ok)
        t |> equal(select_value(doc.document_element, "item/@key"), "val")
    }
}

[test]
def test_for_each_select(t : T?) {
    parse_xml("<root><a>1</a><a>2</a><a>3</a></root>") <| $(doc, ok) {
        t |> success(ok)
        var count = 0
        for_each_select(doc.document_element, "a") <| $(xn) {
            count++
            t |> success(xn.ok)
        }
        t |> equal(count, 3)
    }
}

// ============ operator is/as type tests ============

[test]
def test_attr_as_int(t : T?) {
    parse_xml("<root count=\"42\"/>") <| $(doc, ok) {
        t |> success(ok)
        let a = attribute(doc.document_element, "count")
        t |> success(a is int)
        t |> equal(a as int, 42)
    }
}

[test]
def test_attr_as_float(t : T?) {
    parse_xml("<root ratio=\"3.14\"/>") <| $(doc, ok) {
        t |> success(ok)
        let a = attribute(doc.document_element, "ratio")
        t |> success(a is float)
        t |> success(abs((a as float) - 3.14) < 0.01)
    }
}

[test]
def test_attr_as_bool(t : T?) {
    parse_xml("<root flag=\"true\"/>") <| $(doc, ok) {
        t |> success(ok)
        let a = attribute(doc.document_element, "flag")
        t |> success(a is bool)
        t |> equal(a as bool, true)
    }
}

[test]
def test_attr_as_string(t : T?) {
    parse_xml("<root name=\"hello\"/>") <| $(doc, ok) {
        t |> success(ok)
        let a = attribute(doc.document_element, "name")
        t |> success(a is string)
        t |> equal(a as string, "hello")
    }
}

[test]
def test_attr_as_uint(t : T?) {
    parse_xml("<root val=\"100\"/>") <| $(doc, ok) {
        t |> success(ok)
        let a = attribute(doc.document_element, "val")
        t |> success(a is uint)
        t |> equal(a as uint, 100u)
    }
}

[test]
def test_attr_as_double(t : T?) {
    parse_xml("<root val=\"2.718281828\"/>") <| $(doc, ok) {
        t |> success(ok)
        let a = attribute(doc.document_element, "val")
        t |> success(a is double)
        t |> success(abs((a as double) - 2.718281828lf) < 0.0001lf)
    }
}

[test]
def test_attr_missing_is_not(t : T?) {
    parse_xml("<root/>") <| $(doc, ok) {
        t |> success(ok)
        let a = attribute(doc.document_element, "missing")
        t |> success(!(a is int))
        t |> success(!(a is float))
        t |> success(!(a is bool))
        t |> success(!(a is string))
    }
}

[test]
def test_text_as_int(t : T?) {
    parse_xml("<root><val>99</val></root>") <| $(doc, ok) {
        t |> success(ok)
        let tx = child(doc.document_element, "val").text
        t |> success(tx is int)
        t |> equal(tx as int, 99)
    }
}

[test]
def test_text_as_float(t : T?) {
    parse_xml("<root><val>1.5</val></root>") <| $(doc, ok) {
        t |> success(ok)
        let tx = child(doc.document_element, "val").text
        t |> success(tx is float)
        t |> success(abs((tx as float) - 1.5) < 0.01)
    }
}

[test]
def test_text_as_bool(t : T?) {
    parse_xml("<root><val>true</val></root>") <| $(doc, ok) {
        t |> success(ok)
        let tx = child(doc.document_element, "val").text
        t |> success(tx is bool)
        t |> equal(tx as bool, true)
    }
}

[test]
def test_text_as_string(t : T?) {
    parse_xml("<root><val>hello world</val></root>") <| $(doc, ok) {
        t |> success(ok)
        let tx = child(doc.document_element, "val").text
        t |> success(tx is string)
        t |> equal(tx as string, "hello world")
    }
}

[test]
def test_text_empty_is_not(t : T?) {
    parse_xml("<root><val/></root>") <| $(doc, ok) {
        t |> success(ok)
        let tx = child(doc.document_element, "val").text
        t |> success(!(tx is int))
        t |> success(!(tx is float))
        t |> success(!(tx is string))
    }
}

[test]
def test_node_as_string(t : T?) {
    parse_xml("<root><item>text</item></root>") <| $(doc, ok) {
        t |> success(ok)
        let item = child(doc.document_element, "item")
        let s = item as string
        t |> success(find(s, "item") >= 0)
        t |> success(find(s, "text") >= 0)
    }
}

// ============ operator [] ============

[test]
def test_index_operator(t : T?) {
    t |> run("node[name] returns attribute") <| @(t : T?) {
        parse_xml("<item isbn=\"978\" price=\"29.99\" active=\"true\" count=\"5\"/>") <| $(doc, ok) {
            t |> success(ok)
            let item = doc.document_element
            // basic access
            t |> success(item["isbn"].ok)
            t |> success(!item["missing"].ok)
            // combined with as
            t |> equal(item["isbn"] as string, "978")
            t |> equal(item["count"] as int, 5)
            t |> success(abs(item["price"] as float -29.99) < 0.01)
            t |> equal(item["active"] as bool, true)
        }
    }
    t |> run("node[name] combined with is") <| @(t : T?) {
        parse_xml("<item x=\"1\"/>") <| $(doc, ok) {
            t |> success(ok)
            let item = doc.document_element
            t |> success(item["x"] is int)
            t |> success(!(item["missing"] is int))
        }
    }
}

// ============ tag builder ============

[test]
def test_tag_with_block(t : T?) {
    t |> run("tag with block creates nested structure") <| @(t : T?) {
        with_doc() <| $(doc) {
            doc |> tag("catalog") <| $(var catalog) {
                catalog |> tag("book") <| $(var book) {
                    book |> tag("title", "The C Programming Language")
                    book |> tag("author", "K&R")
                }
            }
            let root = doc.document_element
            t |> equal(root.name, "catalog")
            let book = child(root, "book")
            t |> success(book.ok)
            t |> equal(node_text(book, "title"), "The C Programming Language")
            t |> equal(node_text(book, "author"), "K&R")
        }
    }
}

[test]
def test_tag_leaf(t : T?) {
    t |> run("tag with text creates leaf element") <| @(t : T?) {
        with_doc() <| $(doc) {
            var root = append_child(doc as xml_node, "root")
            root |> tag("name", "Alice")
            root |> tag("age", "30")
            t |> equal(node_text(root, "name"), "Alice")
            t |> equal(node_text(root, "age"), "30")
        }
    }
}

[test]
def test_tag_bare(t : T?) {
    t |> run("tag without text creates empty element") <| @(t : T?) {
        with_doc() <| $(doc) {
            var root = append_child(doc as xml_node, "root")
            var ch = root |> tag("empty")
            t |> success(ch.ok)
            t |> equal(ch.name, "empty")
            t |> success(!ch.first_child.ok)
        }
    }
}

[test]
def test_tag_on_document(t : T?) {
    t |> run("tag overloads work on xml_document?") <| @(t : T?) {
        with_doc() <| $(doc) {
            // block form directly on doc
            doc |> tag("root") <| $(var root) {
                root |> tag("child", "text")
            }
            t |> equal(doc.document_element.name, "root")
            t |> equal(node_text(doc.document_element, "child"), "text")
        }
    }
}

// ============ attr builder ============

[test]
def test_attr_chaining(t : T?) {
    t |> run("attr returns parent for chaining") <| @(t : T?) {
        with_doc() <| $(doc) {
            doc |> tag("item") <| $(var item) {
                item |> attr("isbn", "978-0-13-110362-7") |> attr("price", 29.99) |> attr("edition", 2) |> attr("available", true)
            }
            let item = doc.document_element
            t |> equal(item["isbn"] as string, "978-0-13-110362-7")
            t |> success(abs(item["price"] as float -29.99) < 0.01)
            t |> equal(item["edition"] as int, 2)
            t |> equal(item["available"] as bool, true)
        }
    }
}

// ============ Full builder pattern ============

[test]
def test_full_builder_pattern(t : T?) {
    t |> run("complete XML built with tag + attr") <| @(t : T?) {
        with_doc() <| $(doc) {
            doc |> tag("library") <| $(var library) {
                library |> tag("book") <| $(var book) {
                    book |> attr("isbn", "978-0-13-110362-7") |> attr("lang", "en")
                    book |> tag("title", "The C Programming Language")
                    book |> tag("author", "Kernighan")
                    book |> tag("author", "Ritchie")
                    book |> tag("year", "1988")
                }
                library |> tag("book") <| $(var book) {
                    book |> attr("isbn", "978-0-201-63361-0")
                    book |> tag("title", "Design Patterns")
                    book |> tag("year", "1994")
                }
            }
            let root = doc.document_element
            t |> equal(root.name, "library")
            // First book
            let b1 = root.first_child
            t |> equal(b1["isbn"] as string, "978-0-13-110362-7")
            t |> equal(b1["lang"] as string, "en")
            t |> equal(node_text(b1, "title"), "The C Programming Language")
            t |> equal(node_text(b1, "year"), "1988")
            // Count authors
            var author_count = 0
            for_each_child(b1, "author") <| $(ch) {
                author_count++
            }
            t |> equal(author_count, 2)
            // Second book
            let b2 = b1.next_sibling
            t |> equal(b2["isbn"] as string, "978-0-201-63361-0")
            t |> equal(node_text(b2, "title"), "Design Patterns")
        }
    }
}
