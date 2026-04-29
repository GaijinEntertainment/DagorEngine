options gen2
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options indenting = 4
options strict_smart_pointers = true

require pugixml/PUGIXML_boost
require math
require strings
require dastest/testing_boost public

// ============ Node navigation ============

[test]
def test_node_last_child(t : T?) {
    t |> run("last_child returns final child element") <| @(t : T?) {
        parse_xml("<root><a/><b/><c/></root>") <| $(doc, ok) {
            t |> success(ok)
            let root = doc.document_element
            t |> equal(root.last_child.name, "c")
        }
    }
}

[test]
def test_node_previous_sibling(t : T?) {
    t |> run("previous_sibling navigates backward") <| @(t : T?) {
        parse_xml("<root><a/><b/><c/></root>") <| $(doc, ok) {
            t |> success(ok)
            let root = doc.document_element
            let c_node = root.last_child
            t |> equal(c_node.previous_sibling.name, "b")
            t |> equal(c_node.previous_sibling.previous_sibling.name, "a")
        }
    }
}

[test]
def test_node_parent(t : T?) {
    t |> run("parent returns containing element") <| @(t : T?) {
        parse_xml("<root><child><nested/></child></root>") <| $(doc, ok) {
            t |> success(ok)
            let root = doc.document_element
            let nested = child(child(root, "child"), "nested")
            t |> equal(nested.parent.name, "child")
            t |> equal(nested.parent.parent.name, "root")
        }
    }
}

[test]
def test_node_root(t : T?) {
    t |> run("root returns document node from any depth") <| @(t : T?) {
        parse_xml("<root><a><b><c/></b></a></root>") <| $(doc, ok) {
            t |> success(ok)
            let deep = child(child(child(doc.document_element, "a"), "b"), "c")
            let r = deep.root
            t |> success(r.ok)
            // root is the document node (not the document element)
            t |> equal(int(r._type), int(xml_node_type.node_document))
        }
    }
}

[test]
def test_node_last_attribute(t : T?) {
    t |> run("last_attribute returns final attribute") <| @(t : T?) {
        parse_xml("<root x=\"1\" y=\"2\" z=\"3\"/>") <| $(doc, ok) {
            t |> success(ok)
            let root = doc.document_element
            t |> equal(root.last_attribute.name, "z")
        }
    }
}

[test]
def test_node_child_value(t : T?) {
    t |> run("child_value returns PCDATA text content") <| @(t : T?) {
        parse_xml("<root>hello world</root>") <| $(doc, ok) {
            t |> success(ok)
            let root = doc.document_element
            t |> equal(root.child_value, "hello world")
        }
    }
}

[test]
def test_node_child_value_by_name(t : T?) {
    t |> run("child_value(name) returns named child text") <| @(t : T?) {
        parse_xml("<root><title>My Title</title><body>Content</body></root>") <| $(doc, ok) {
            t |> success(ok)
            let root = doc.document_element
            t |> equal(child_value(root, "title"), "My Title")
            t |> equal(child_value(root, "body"), "Content")
            t |> equal(child_value(root, "missing"), "")
        }
    }
}

[test]
def test_find_child_by_attribute(t : T?) {
    t |> run("find_child_by_attribute locates element") <| @(t : T?) {
        parse_xml("<root><item id=\"a\"/><item id=\"b\"/><item id=\"c\"/></root>") <| $(doc, ok) {
            t |> success(ok)
            let root = doc.document_element
            let found = find_child_by_attribute(root, "item", "id", "b")
            t |> success(found.ok)
            t |> equal(node_attr(found, "id"), "b")
        }
    }
}

[test]
def test_find_child_by_attribute_any_name(t : T?) {
    t |> run("find_child_by_attribute without element name") <| @(t : T?) {
        parse_xml("<root><foo key=\"x\"/><bar key=\"y\"/></root>") <| $(doc, ok) {
            t |> success(ok)
            let root = doc.document_element
            let found = find_child_by_attribute(root, "key", "y")
            t |> success(found.ok)
            t |> equal(found.name, "bar")
        }
    }
}

[test]
def test_first_element_by_path(t : T?) {
    t |> run("first_element_by_path traverses path") <| @(t : T?) {
        parse_xml("<root><a><b><c>deep</c></b></a></root>") <| $(doc, ok) {
            t |> success(ok)
            let root = doc.document_element
            let found = first_element_by_path(root, "a/b/c")
            t |> success(found.ok)
            t |> equal(found.name, "c")
            t |> equal(found.text as string, "deep")
        }
    }
}

[test]
def test_node_path(t : T?) {
    t |> run("path returns slash-separated path") <| @(t : T?) {
        parse_xml("<root><a><b/></a></root>") <| $(doc, ok) {
            t |> success(ok)
            let b_node = child(child(doc.document_element, "a"), "b")
            let p = path(b_node, "/")
            t |> success(find(p, "root") >= 0)
            t |> success(find(p, "a") >= 0)
            t |> success(find(p, "b") >= 0)
        }
    }
}

[test]
def test_node_hash_value(t : T?) {
    t |> run("hash_value is consistent for same node") <| @(t : T?) {
        parse_xml("<root><a/><b/></root>") <| $(doc, ok) {
            t |> success(ok)
            let root = doc.document_element
            let a_node = root.first_child
            let b_node = a_node.next_sibling
            // Same node should have same hash
            t |> equal(a_node.hash_value, a_node.hash_value)
            // Different nodes should (usually) have different hashes
            t |> success(a_node.hash_value != b_node.hash_value)
        }
    }
}

[test]
def test_node_comparison(t : T?) {
    t |> run("node == and != operators") <| @(t : T?) {
        parse_xml("<root><a/><b/></root>") <| $(doc, ok) {
            t |> success(ok)
            let root = doc.document_element
            let a1 = child(root, "a")
            let a2 = child(root, "a")
            let b_node = child(root, "b")
            t |> success(a1 == a2)
            t |> success(a1 != b_node)
        }
    }
}

[test]
def test_node_empty(t : T?) {
    t |> run("node empty and ok properties") <| @(t : T?) {
        parse_xml("<root/>") <| $(doc, ok) {
            t |> success(ok)
            let root = doc.document_element
            t |> success(root.ok)
            t |> success(!root.empty)
            let missing = child(root, "nope")
            t |> success(!missing.ok)
            t |> success(missing.empty)
        }
    }
}

[test]
def test_node_type(t : T?) {
    t |> run("node type returns correct xml_node_type") <| @(t : T?) {
        parse_xml("<root>text<!-- comment --></root>") <| $(doc, ok) {
            t |> success(ok)
            let root = doc.document_element
            t |> equal(int(root._type), int(xml_node_type.node_element))
        }
    }
}

// ============ Attribute navigation & properties ============

[test]
def test_attribute_previous(t : T?) {
    t |> run("previous_attribute navigates backward") <| @(t : T?) {
        parse_xml("<root a=\"1\" b=\"2\" c=\"3\"/>") <| $(doc, ok) {
            t |> success(ok)
            let root = doc.document_element
            let last = root.last_attribute
            t |> equal(last.name, "c")
            t |> equal(last.previous_attribute.name, "b")
            t |> equal(last.previous_attribute.previous_attribute.name, "a")
        }
    }
}

[test]
def test_attribute_hash_value(t : T?) {
    t |> run("attribute hash_value is consistent") <| @(t : T?) {
        parse_xml("<root x=\"1\" y=\"2\"/>") <| $(doc, ok) {
            t |> success(ok)
            let root = doc.document_element
            let x_attr = attribute(root, "x")
            let y_attr = attribute(root, "y")
            t |> equal(x_attr.hash_value, x_attr.hash_value)
            t |> success(x_attr.hash_value != y_attr.hash_value)
        }
    }
}

[test]
def test_attribute_comparison(t : T?) {
    t |> run("attribute == and != operators") <| @(t : T?) {
        parse_xml("<root x=\"1\" y=\"2\"/>") <| $(doc, ok) {
            t |> success(ok)
            let root = doc.document_element
            let x1 = attribute(root, "x")
            let x2 = attribute(root, "x")
            let y_attr = attribute(root, "y")
            t |> success(x1 == x2)
            t |> success(x1 != y_attr)
        }
    }
}

[test]
def test_attribute_empty(t : T?) {
    t |> run("missing attribute is empty/not ok") <| @(t : T?) {
        parse_xml("<root x=\"1\"/>") <| $(doc, ok) {
            t |> success(ok)
            let root = doc.document_element
            let x_attr = attribute(root, "x")
            t |> success(x_attr.ok)
            t |> success(!x_attr.empty)
            let missing = attribute(root, "nope")
            t |> success(!missing.ok)
            t |> success(missing.empty)
        }
    }
}

[test]
def test_attribute_name_value(t : T?) {
    t |> run("attribute name and value properties") <| @(t : T?) {
        parse_xml("<root color=\"red\"/>") <| $(doc, ok) {
            t |> success(ok)
            let a = attribute(doc.document_element, "color")
            t |> equal(a.name, "color")
            t |> equal(a.value, "red")
        }
    }
}

// ============ Text properties ============

[test]
def test_text_get(t : T?) {
    t |> run("text get property returns raw text") <| @(t : T?) {
        parse_xml("<root>hello</root>") <| $(doc, ok) {
            t |> success(ok)
            let tx = doc.document_element.text
            t |> success(tx.ok)
            t |> success(!tx.empty)
            t |> equal(tx.get, "hello")
        }
    }
}

[test]
def test_text_data(t : T?) {
    t |> run("text data returns PCDATA node") <| @(t : T?) {
        parse_xml("<root>hello</root>") <| $(doc, ok) {
            t |> success(ok)
            let tx = doc.document_element.text
            let data_node = tx.data
            t |> success(data_node.ok)
            // data returns the PCDATA child node; its parent is the element
            t |> equal(int(data_node._type), int(xml_node_type.node_pcdata))
            t |> equal(data_node.parent.name, "root")
        }
    }
}

// ============ Parse result ============

[test]
def test_parse_result_success(t : T?) {
    t |> run("parse_result reports success for valid XML") <| @(t : T?) {
        var doc = new xml_document()
        var result : xml_parse_result
        let ok = load_string(doc, "<root/>", result, parse_default)
        t |> success(ok)
        t |> equal(int(result.status), int(xml_parse_status.status_ok))
        t |> success(length(result.description) > 0)
        unsafe { delete doc; }
    }
}

[test]
def test_parse_result_failure(t : T?) {
    t |> run("parse_result reports failure for invalid XML") <| @(t : T?) {
        var doc = new xml_document()
        var result : xml_parse_result
        let ok = load_string(doc, "<root><unclosed>", result, parse_default)
        t |> success(!ok)
        t |> success(int(result.status) != int(xml_parse_status.status_ok))
        unsafe { delete doc; }
    }
}

// ============ Document operations ============

[test]
def test_document_element(t : T?) {
    t |> run("document_element returns root element") <| @(t : T?) {
        parse_xml("<myroot/>") <| $(doc, ok) {
            t |> success(ok)
            t |> equal(doc.document_element.name, "myroot")
        }
    }
}

[test]
def test_document_reset(t : T?) {
    t |> run("reset clears the document") <| @(t : T?) {
        var doc = new xml_document()
        var result : xml_parse_result
        load_string(doc, "<root><a/></root>", result, parse_default)
        t |> success(doc.document_element.ok)
        reset(doc)
        t |> success(!doc.document_element.ok)
        unsafe { delete doc; }
    }
}

[test]
def test_save_string(t : T?) {
    t |> run("save_string serializes document") <| @(t : T?) {
        with_doc() <| $(doc) {
            var root = append_child(doc as xml_node, "test")
            add_child(root, "item", "value")
            let s = save_string(doc, "\t", format_default, xml_encoding.encoding_auto)
            t |> success(length(s) > 0)
            t |> success(find(s, "test") >= 0)
            t |> success(find(s, "value") >= 0)
        }
    }
}

// ============ XPath advanced ============

[test]
def test_xpath_evaluate_boolean(t : T?) {
    t |> run("evaluate_boolean returns bool result") <| @(t : T?) {
        parse_xml("<root><item/></root>") <| $(doc, ok) {
            t |> success(ok)
            with_xpath("boolean(//item)") <| $(q) {
                t |> success(evaluate_boolean(q, doc.document_element))
            }
            with_xpath("boolean(//missing)") <| $(q) {
                t |> success(!evaluate_boolean(q, doc.document_element))
            }
        }
    }
}

[test]
def test_xpath_evaluate_number(t : T?) {
    t |> run("evaluate_number returns numeric result") <| @(t : T?) {
        parse_xml("<root><a/><a/><a/></root>") <| $(doc, ok) {
            t |> success(ok)
            with_xpath("count(//a)") <| $(q) {
                t |> success(abs(evaluate_number(q, doc.document_element) - 3.0lf) < 0.001lf)
            }
        }
    }
}

[test]
def test_xpath_evaluate_string(t : T?) {
    t |> run("evaluate_string returns string result") <| @(t : T?) {
        parse_xml("<root><title>Hello</title></root>") <| $(doc, ok) {
            t |> success(ok)
            with_xpath("string(//title)") <| $(q) {
                t |> equal(evaluate_string(q, doc.document_element), "Hello")
            }
        }
    }
}

[test]
def test_xpath_evaluate_node_set(t : T?) {
    t |> run("evaluate_node_set returns node set") <| @(t : T?) {
        parse_xml("<root><a/><b/><c/></root>") <| $(doc, ok) {
            t |> success(ok)
            with_xpath("/root/*") <| $(q) {
                var ns = evaluate_node_set(q, doc.document_element)
                t |> equal(ns.size, 3)
                t |> success(!ns.empty)
                t |> equal(ns.first.node.name, "a")
                t |> equal(at(ns, 2).node.name, "c")
                unsafe { delete ns; }
            }
        }
    }
}

[test]
def test_xpath_query_properties(t : T?) {
    t |> run("xpath_query exposes return_type and result") <| @(t : T?) {
        with_xpath("count(//a)") <| $(q) {
            t |> success(q.ok)
            t |> equal(int(q.return_type), int(xpath_value_type.xpath_type_number))
        }
        with_xpath("boolean(//a)") <| $(q) {
            t |> equal(int(q.return_type), int(xpath_value_type.xpath_type_boolean))
        }
    }
}

[test]
def test_xpath_node_set_sort(t : T?) {
    t |> run("xpath_node_set sort reverses order") <| @(t : T?) {
        parse_xml("<root><a/><b/><c/></root>") <| $(doc, ok) {
            t |> success(ok)
            var ns = select_nodes(doc.document_element, "*")
            t |> equal(at(ns, 0).node.name, "a")
            sort(ns, true)
            t |> equal(at(ns, 0).node.name, "c")
            unsafe { delete ns; }
        }
    }
}

[test]
def test_xpath_node_accessors(t : T?) {
    t |> run("xpath_node node/attribute/parent accessors") <| @(t : T?) {
        parse_xml("<root><item key=\"val\"/></root>") <| $(doc, ok) {
            t |> success(ok)
            // Node result
            let nr = select_node(doc.document_element, "item")
            t |> success(nr.ok)
            t |> equal(nr.node.name, "item")
            t |> success(nr.parent.ok)
            t |> equal(nr.parent.name, "root")
            // Attribute result
            let ar = select_node(doc.document_element, "item/@key")
            t |> success(ar.ok)
            t |> equal(ar.attribute.name, "key")
            t |> equal(ar.attribute.value, "val")
        }
    }
}

[test]
def test_xpath_variables(t : T?) {
    t |> run("xpath with variable substitution") <| @(t : T?) {
        parse_xml("<root><item n=\"1\"/><item n=\"2\"/><item n=\"3\"/></root>") <| $(doc, ok) {
            t |> success(ok)
            var vars = new xpath_variable_set()
            set(vars, "target", 2.0lf)
            var q = xpath_compile("item[@n=$target]", vars)
            t |> success(q.ok)
            let result = evaluate_node(q, doc.document_element)
            t |> success(result.ok)
            t |> equal(node_attr(result.node, "n"), "2")
            unsafe { delete q; }
            unsafe { delete vars; }
        }
    }
}
