options gen2
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options indenting = 4
options strict_smart_pointers = true

require pugixml/PUGIXML_boost
require math
require strings
require dastest/testing_boost public

// ============ Node mutation ============

[test]
def test_set_name(t : T?) {
    t |> run("set_name renames a node") <| @(t : T?) {
        with_doc() <| $(doc) {
            var root = append_child(doc as xml_node, "old")
            t |> equal(root.name, "old")
            set_name(root, "new_name")
            t |> equal(root.name, "new_name")
        }
    }
}

[test]
def test_set_value(t : T?) {
    t |> run("set_value changes node value") <| @(t : T?) {
        with_doc() <| $(doc) {
            var dnode = doc as xml_node
            var root = append_child(dnode, "root")
            var pi = append_child(root, xml_node_type.node_pi)
            set_name(pi, "target")
            set_value(pi, "data")
            t |> equal(pi.value, "data")
        }
    }
}

[test]
def test_prepend_child(t : T?) {
    t |> run("prepend_child inserts at beginning") <| @(t : T?) {
        with_doc() <| $(doc) {
            var root = append_child(doc as xml_node, "root")
            append_child(root, "second")
            prepend_child(root, "first")
            t |> equal(root.first_child.name, "first")
            t |> equal(root.last_child.name, "second")
        }
    }
}

[test]
def test_insert_child_after(t : T?) {
    t |> run("insert_child_after places node correctly") <| @(t : T?) {
        with_doc() <| $(doc) {
            var root = append_child(doc as xml_node, "root")
            var a_node = append_child(root, "a")
            append_child(root, "c")
            insert_child_after(root, "b", a_node)
            // Order should be: a, b, c
            t |> equal(root.first_child.name, "a")
            t |> equal(root.first_child.next_sibling.name, "b")
            t |> equal(root.last_child.name, "c")
        }
    }
}

[test]
def test_insert_child_before(t : T?) {
    t |> run("insert_child_before places node correctly") <| @(t : T?) {
        with_doc() <| $(doc) {
            var root = append_child(doc as xml_node, "root")
            append_child(root, "a")
            var c_node = append_child(root, "c")
            insert_child_before(root, "b", c_node)
            // Order should be: a, b, c
            t |> equal(root.first_child.next_sibling.name, "b")
        }
    }
}

[test]
def test_remove_child(t : T?) {
    t |> run("remove_child removes a node object") <| @(t : T?) {
        with_doc() <| $(doc) {
            var root = append_child(doc as xml_node, "root")
            append_child(root, "keep")
            var rm = append_child(root, "remove_me")
            append_child(root, "also_keep")
            remove_child(root, rm)
            var count = 0
            for_each_child(root) <| $(ch) {
                count++
                t |> success(ch.name != "remove_me")
            }
            t |> equal(count, 2)
        }
    }
}

[test]
def test_remove_child_by_name(t : T?) {
    t |> run("remove_child by name removes first match") <| @(t : T?) {
        with_doc() <| $(doc) {
            var root = append_child(doc as xml_node, "root")
            append_child(root, "a")
            append_child(root, "b")
            append_child(root, "c")
            remove_child(root, "b")
            var names : array<string>
            for_each_child(root) <| $(ch) {
                names |> push(ch.name)
            }
            t |> equal(length(names), 2)
            t |> equal(names[0], "a")
            t |> equal(names[1], "c")
        }
    }
}

[test]
def test_remove_children(t : T?) {
    t |> run("remove_children clears all children") <| @(t : T?) {
        with_doc() <| $(doc) {
            var root = append_child(doc as xml_node, "root")
            append_child(root, "a")
            append_child(root, "b")
            append_child(root, "c")
            remove_children(root)
            t |> success(!root.first_child.ok)
        }
    }
}

// ============ Attribute mutation ============

[test]
def test_prepend_attribute(t : T?) {
    t |> run("prepend_attribute inserts at beginning") <| @(t : T?) {
        with_doc() <| $(doc) {
            var root = append_child(doc as xml_node, "root")
            var a2 = append_attribute(root, "second")
            set_value(a2, "2")
            var a1 = prepend_attribute(root, "first")
            set_value(a1, "1")
            t |> equal(root.first_attribute.name, "first")
            t |> equal(root.last_attribute.name, "second")
        }
    }
}

[test]
def test_insert_attribute_after(t : T?) {
    t |> run("insert_attribute_after places attribute correctly") <| @(t : T?) {
        with_doc() <| $(doc) {
            var root = append_child(doc as xml_node, "root")
            var a = append_attribute(root, "a")
            set_value(a, "1")
            var c = append_attribute(root, "c")
            set_value(c, "3")
            var b = insert_attribute_after(root, "b", a)
            set_value(b, "2")
            t |> equal(root.first_attribute.next_attribute.name, "b")
        }
    }
}

[test]
def test_insert_attribute_before(t : T?) {
    t |> run("insert_attribute_before places attribute correctly") <| @(t : T?) {
        with_doc() <| $(doc) {
            var root = append_child(doc as xml_node, "root")
            var a = append_attribute(root, "a")
            set_value(a, "1")
            var c = append_attribute(root, "c")
            set_value(c, "3")
            var b = insert_attribute_before(root, "b", c)
            set_value(b, "2")
            t |> equal(root.first_attribute.next_attribute.name, "b")
        }
    }
}

[test]
def test_remove_attribute(t : T?) {
    t |> run("remove_attribute removes attribute object") <| @(t : T?) {
        with_doc() <| $(doc) {
            var root = append_child(doc as xml_node, "root")
            add_attr(root, "keep", "yes")
            var rm = append_attribute(root, "remove_me")
            set_value(rm, "no")
            remove_attribute(root, rm)
            t |> success(!attribute(root, "remove_me").ok)
            t |> success(attribute(root, "keep").ok)
        }
    }
}

[test]
def test_remove_attribute_by_name(t : T?) {
    t |> run("remove_attribute by name") <| @(t : T?) {
        with_doc() <| $(doc) {
            var root = append_child(doc as xml_node, "root")
            add_attr(root, "a", "1")
            add_attr(root, "b", "2")
            add_attr(root, "c", "3")
            remove_attribute(root, "b")
            t |> success(!attribute(root, "b").ok)
            t |> success(attribute(root, "a").ok)
            t |> success(attribute(root, "c").ok)
        }
    }
}

[test]
def test_remove_attributes(t : T?) {
    t |> run("remove_attributes clears all attributes") <| @(t : T?) {
        with_doc() <| $(doc) {
            var root = append_child(doc as xml_node, "root")
            add_attr(root, "a", "1")
            add_attr(root, "b", "2")
            remove_attributes(root)
            t |> success(!root.first_attribute.ok)
        }
    }
}

[test]
def test_attribute_set_name(t : T?) {
    t |> run("set_name renames an attribute") <| @(t : T?) {
        with_doc() <| $(doc) {
            var root = append_child(doc as xml_node, "root")
            var a = append_attribute(root, "old_name")
            set_value(a, "val")
            set_name(a, "new_name")
            t |> equal(a.name, "new_name")
            t |> success(attribute(root, "new_name").ok)
        }
    }
}

[test]
def test_attribute_set_value_types(t : T?) {
    t |> run("set_value works for all types") <| @(t : T?) {
        with_doc() <| $(doc) {
            var root = append_child(doc as xml_node, "root")
            var ai = append_attribute(root, "i")
            set_value(ai, 42)
            t |> equal(ai as int, 42)
            var au = append_attribute(root, "u")
            set_value(au, 100u)
            t |> equal(au as uint, 100u)
            var af = append_attribute(root, "f")
            set_value(af, 3.14)
            t |> success(abs(af as float -3.14) < 0.01)
            var ad = append_attribute(root, "d")
            set_value(ad, 2.718lf)
            t |> success(abs(ad as double - 2.718lf) < 0.01lf)
            var ab = append_attribute(root, "b")
            set_value(ab, true)
            t |> equal(ab as bool, true)
        }
    }
}

// ============ Text mutation ============

[test]
def test_text_set_string(t : T?) {
    t |> run("text set(string) changes text content") <| @(t : T?) {
        with_doc() <| $(doc) {
            var root = append_child(doc as xml_node, "root")
            set(root.text, "hello")
            t |> equal(root.text as string, "hello")
            set(root.text, "world")
            t |> equal(root.text as string, "world")
        }
    }
}

[test]
def test_text_set_int(t : T?) {
    t |> run("text set(int) stores numeric text") <| @(t : T?) {
        with_doc() <| $(doc) {
            var root = append_child(doc as xml_node, "root")
            set(root.text, 42)
            t |> equal(root.text as int, 42)
        }
    }
}

[test]
def test_text_set_float(t : T?) {
    t |> run("text set(float) stores float text") <| @(t : T?) {
        with_doc() <| $(doc) {
            var root = append_child(doc as xml_node, "root")
            set(root.text, 3.14)
            t |> success(abs(root.text as float -3.14) < 0.01)
        }
    }
}

[test]
def test_text_set_double(t : T?) {
    t |> run("text set(double) stores double text") <| @(t : T?) {
        with_doc() <| $(doc) {
            var root = append_child(doc as xml_node, "root")
            set(root.text, 2.718281828lf)
            t |> success(abs(root.text as double - 2.718281828lf) < 0.0001lf)
        }
    }
}

[test]
def test_text_set_uint(t : T?) {
    t |> run("text set(uint) stores unsigned text") <| @(t : T?) {
        with_doc() <| $(doc) {
            var root = append_child(doc as xml_node, "root")
            set(root.text, 999u)
            t |> equal(root.text as uint, 999u)
        }
    }
}

[test]
def test_text_set_bool(t : T?) {
    t |> run("text set(bool) stores boolean text") <| @(t : T?) {
        with_doc() <| $(doc) {
            var root = append_child(doc as xml_node, "root")
            set(root.text, true)
            t |> equal(root.text as bool, true)
            set(root.text, false)
            t |> equal(root.text as bool, false)
        }
    }
}

// ============ Copy / Move operations ============

[test]
def test_append_copy_node(t : T?) {
    t |> run("append_copy duplicates a node") <| @(t : T?) {
        with_doc() <| $(doc) {
            var dnode = doc as xml_node
            var root = append_child(dnode, "root")
            var src = append_child(root, "source")
            add_attr(src, "id", "1")
            add_child(src, "inner", "data")
            var root2 = append_child(dnode, "root2")
            var copy = append_copy(root2, src)
            t |> equal(copy.name, "source")
            t |> equal(node_attr(copy, "id"), "1")
            t |> equal(node_text(copy, "inner"), "data")
            // Original still exists
            t |> success(child(root, "source").ok)
        }
    }
}

[test]
def test_prepend_copy_node(t : T?) {
    t |> run("prepend_copy duplicates at beginning") <| @(t : T?) {
        with_doc() <| $(doc) {
            var root = append_child(doc as xml_node, "root")
            append_child(root, "existing")
            var src = append_child(root, "source")
            var target = append_child(doc as xml_node, "target")
            append_child(target, "before")
            prepend_copy(target, src)
            t |> equal(target.first_child.name, "source")
        }
    }
}

[test]
def test_append_copy_attr(t : T?) {
    t |> run("append_copy duplicates an attribute") <| @(t : T?) {
        with_doc() <| $(doc) {
            var root = append_child(doc as xml_node, "root")
            add_attr(root, "src_attr", "value")
            var target = append_child(root, "target")
            let src_a = attribute(root, "src_attr")
            append_copy(target, src_a)
            t |> equal(node_attr(target, "src_attr"), "value")
        }
    }
}

[test]
def test_append_move_node(t : T?) {
    t |> run("append_move moves node to new parent") <| @(t : T?) {
        with_doc() <| $(doc) {
            var root = append_child(doc as xml_node, "root")
            var src = append_child(root, "moveable")
            add_child(src, "data", "test")
            var target = append_child(root, "target")
            append_move(target, src)
            // Should be under target now
            t |> success(child(target, "moveable").ok)
            // Should no longer be a direct child of root (but root still has "target")
            var direct_children : array<string>
            for_each_child(root) <| $(ch) {
                direct_children |> push(ch.name)
            }
            t |> success(!direct_children |> has("moveable"))
        }
    }
}

// ============ Append child by type ============

[test]
def test_append_child_by_type(t : T?) {
    t |> run("append_child with node type creates typed node") <| @(t : T?) {
        with_doc() <| $(doc) {
            var root = append_child(doc as xml_node, "root")
            var comment = append_child(root, xml_node_type.node_comment)
            set_value(comment, "this is a comment")
            t |> equal(int(comment._type), int(xml_node_type.node_comment))
            t |> equal(comment.value, "this is a comment")
        }
    }
}

[test]
def test_prepend_child_by_type(t : T?) {
    t |> run("prepend_child with node type") <| @(t : T?) {
        with_doc() <| $(doc) {
            var root = append_child(doc as xml_node, "root")
            append_child(root, "element")
            var cdata = prepend_child(root, xml_node_type.node_cdata)
            set_value(cdata, "raw data")
            t |> equal(int(root.first_child._type), int(xml_node_type.node_cdata))
        }
    }
}

// ============ has() helper for array<string> ============

def has(arr : array<string>; val : string) : bool {
    for (s in arr) {
        if (s == val) {
            return true
        }
    }
    return false
}
