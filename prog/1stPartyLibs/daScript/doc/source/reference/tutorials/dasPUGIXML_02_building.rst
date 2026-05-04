.. _tutorial_dasPUGIXML_building:

===========================================
PUGIXML-02 — Building XML
===========================================

.. index::
    single: Tutorial; XML Building
    single: Tutorial; dasPUGIXML
    single: Tutorial; XML Builder

This tutorial covers constructing XML documents from scratch using
``pugixml/PUGIXML_boost`` — the ``tag``/``attr`` EDSL, builder helpers,
and serialization to strings.

Creating an empty document
==========================

``with_doc`` allocates a new ``xml_document``, invokes a block, and
frees it automatically:

.. code-block:: das

   with_doc() <| $(doc) {
       var dnode = doc as xml_node
       var root = append_child(dnode, "greeting")
       set(root.text, "Hello!")
       print(to_string(doc))
   }

The ``tag`` builder EDSL
========================

``tag()`` creates child elements using pipe + block syntax for nesting.
Three overloads:

- ``tag(name, block)`` — element with nested content
- ``tag(name, text)`` — leaf element with text
- ``tag(name)`` — bare element

.. code-block:: das

   with_doc() <| $(doc) {
       doc |> tag("library") <| $(var library) {
           library |> tag("book") <| $(var book) {
               book |> tag("title", "The C Programming Language")
               book |> tag("author", "Kernighan")
               book |> tag("year", "1988")
           }
       }
       print(to_string(doc))
   }

Output:

.. code-block:: xml

   <?xml version="1.0"?>
   <library>
     <book>
       <title>The C Programming Language</title>
       <author>Kernighan</author>
       <year>1988</year>
     </book>
   </library>

Attribute chaining
==================

``attr()`` appends an attribute and returns the **parent node**,
enabling fluent chaining:

.. code-block:: das

   var display = config |> tag("display")
   display |> attr("width", 1920) |> attr("height", 1080) |> attr("fullscreen", true)

``attr`` has overloads for ``string``, ``int``, ``float``, and ``bool``.

Builder helpers
===============

Lower-level helpers for when you don't need the pipe+block pattern:

=========================================  ================================================
Function                                   Description
=========================================  ================================================
``add_child(node, name, text)``            Append element with text content
``add_child_ex(node, name, attr, val)``    Append element with one attribute
``add_attr(node, name, value)``            Append typed attribute (string/int/float/bool)
=========================================  ================================================

.. code-block:: das

   add_child(root, "name", "Alice")
   add_child_ex(root, "skill", "name", "daslang")
   add_attr(root, "score", 95)

Serializing to string
=====================

``to_string`` works on both documents and individual nodes:

- ``to_string(doc)`` — full document with ``<?xml version="1.0"?>``
- ``to_string(node)`` — subtree only, no declaration
- ``node as string`` — same as ``to_string(node)``

Dynamic building with loops
===========================

``tag`` and ``attr`` work naturally inside loops:

.. code-block:: das

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
   }

.. seealso::

   Full source: :download:`tutorials/dasPUGIXML/02_building_xml.das <../../../../tutorials/dasPUGIXML/02_building_xml.das>`

   Previous tutorial: :ref:`tutorial_dasPUGIXML_parsing`

   Next tutorial: :ref:`tutorial_dasPUGIXML_xpath`
