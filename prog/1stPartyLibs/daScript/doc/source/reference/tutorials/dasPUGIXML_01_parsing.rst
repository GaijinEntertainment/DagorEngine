.. _tutorial_dasPUGIXML_parsing:

===========================================
PUGIXML-01 — Parsing & Navigation
===========================================

.. index::
    single: Tutorial; XML
    single: Tutorial; XML Parsing
    single: Tutorial; dasPUGIXML
    single: Tutorial; PUGIXML_boost
    single: Tutorial; pugixml

This tutorial covers parsing XML documents and navigating their node trees
using ``pugixml/PUGIXML_boost``.

Setup
=====

Import the module:

.. code-block:: das

   require pugixml/PUGIXML_boost

``PUGIXML_boost`` re-exports the core ``pugixml`` module, so a single
``require`` gives you everything.

Parsing from a string
=====================

``parse_xml`` parses an XML string inside a block.  The document is freed
automatically when the block returns — no manual ``delete`` needed:

.. code-block:: das

   let xml = "<greeting lang=\"en\">Hello!</greeting>"

   parse_xml(xml) <| $(doc, ok) {
       if (!ok) { return; }
       let root = doc.document_element
       print("{root.name}: {root.text as string}\n")
   }
   // greeting: Hello!

Loading from a file
===================

``open_xml`` loads a file with the same RAII pattern:

.. code-block:: das

   open_xml("tutorials/dasPUGIXML/books.xml") <| $(doc, ok) {
       if (!ok) { return; }
       let root = doc.document_element
       print("root: {root.name}\n")
   }

Iterating children
==================

``for_each_child`` iterates over all child elements of a node using a
block callback.  Pass an optional name to filter by tag:

.. code-block:: das

   // all children
   root |> for_each_child() <| $(ch) {
       print("<{ch.name}>\n")
   }

   // only <setting> children
   root |> for_each_child("setting") <| $(ch) { ... }

``each_child`` returns a lazy iterator for use in ``for`` loops — same
traversal, different syntax:

.. code-block:: das

   for (ch in each_child(root)) {
       print("<{ch.name}>\n")
   }

   for (ch in each_child(root, "setting")) { ... }

``for_each_attribute`` iterates attributes with a block callback:

.. code-block:: das

   node |> for_each_attribute() <| $(a) {
       print("{a.name} = {a as string}\n")
   }

``each_attribute`` is the iterator alternative:

.. code-block:: das

   for (a in each_attribute(node)) {
       print("{a.name} = {a as string}\n")
   }

Quick accessors
===============

Helper functions read child text and attribute values with defaults
for missing data:

================================  =======================================
Function                          Returns
================================  =======================================
``node_text(n, "tag")``           Text of child ``<tag>`` as ``string``
``node_attr(n, "key")``           Attribute value as ``string``
``node_attr_int(n, "key")``       Attribute as ``int``
``node_attr_float(n, "key")``     Attribute as ``float``
``node_attr_bool(n, "key")``      Attribute as ``bool``
================================  =======================================

All accept an optional default value:

.. code-block:: das

   let name  = node_text(node, "name", "unknown")
   let price = node_attr_float(node, "price", 0.0)

Typed access with ``is``/``as``
===============================

``operator []`` on ``xml_node`` returns an ``xml_attribute``.
The ``is`` and ``as`` operators convert attributes and text values
to typed values:

.. code-block:: das

   // Attribute access
   let x_attr = node["x"]           // xml_attribute
   let x_val  = node["x"] as int    // 10
   let label  = node["label"] as string

   // Text access
   let count = child_node.text as int
   let ratio = child_node.text as float

``is`` returns ``true`` when the attribute or text node exists (non-empty).

Practical example
=================

Combining these tools to read a book catalog:

.. code-block:: das

   open_xml("books.xml") <| $(doc, ok) {
       if (!ok) { return; }
       doc.document_element |> for_each_child("book") <| $(book) {
           let title = node_text(book, "title")
           let isbn  = book["isbn"] as string
           let price = child(book, "price").text as float
           print("{title} — {isbn}, ${price}\n")
       }
   }

.. seealso::

   Full source: :download:`tutorials/dasPUGIXML/01_parsing_and_navigation.das <../../../../tutorials/dasPUGIXML/01_parsing_and_navigation.das>`

   Next tutorial: :ref:`tutorial_dasPUGIXML_building`
