.. _tutorial_dasPUGIXML_xpath:

===========================================
PUGIXML-03 — XPath Queries
===========================================

.. index::
    single: Tutorial; XPath
    single: Tutorial; dasPUGIXML
    single: Tutorial; XML Queries

This tutorial demonstrates querying XML documents with XPath, using both
convenience wrappers and compiled queries in ``pugixml/PUGIXML_boost``.

The tutorial uses an inline catalog XML for most examples, then queries
``books.xml`` at the end.

``select_text`` — first match text
====================================

``select_text`` runs an XPath query and returns the text content of the
first matching node.  Returns a default string if nothing matches:

.. code-block:: das

   parse_xml(CATALOG_XML) <| $(doc, ok) {
       if (!ok) { return; }
       let root = doc.document_element

       let first_name = select_text(root, "product[1]/name")
       print("first product: {first_name}\n")
       // first product: Wireless Mouse

       let missing = select_text(root, "product/description", "N/A")
       print("description: {missing}\n")
       // description: N/A
   }

``select_value`` — attribute or text
======================================

``select_value`` returns the string value of the first XPath match —
either an attribute's value or an element's text content:

.. code-block:: das

   let id = select_value(root, "product[1]/@id")
   // "A1"

   let cat = select_value(root, "product[@id='B1']/@category")
   // "books"

   let price = select_value(root, "product[@id='A2']/price")
   // "79.99"

   let missing = select_value(root, "product/@color", "none")
   // "none"

``for_each_select`` — iterate matches
========================================

``for_each_select`` invokes a block for every matching ``xpath_node``.
Use ``.node`` or ``.attribute`` to access the underlying XML node or
attribute:

.. code-block:: das

   // Iterate element results
   root |> for_each_select("product/name") <| $(xn) {
       let n = xn.node
       print("  {n.text as string}\n")
   }

   // Iterate attribute results
   root |> for_each_select("product/@id") <| $(xn) {
       let a = xn.attribute
       print("  {a as string}\n")
   }

This is the most common pattern for processing multiple XPath results.

Compiled XPath with ``with_xpath``
====================================

``with_xpath`` pre-compiles an XPath expression once and frees it
when the block ends.  Use ``evaluate_node_set`` to run the compiled
query:

.. code-block:: das

   with_xpath("product/name") <| $(query) {
       var ns = evaluate_node_set(query, root)
       print("Product count: {ns.size}\n")

       ns |> for_each() <| $(xn) {
           let n = xn.node
           print("  {n.text as string}\n")
       }
       unsafe { delete ns; }
   }

Low-level XPath API
===================

``select_node`` returns the first match.  ``select_nodes`` returns all
matches as an ``xpath_node_set``, which is directly iterable in a
``for`` loop (via the ``each`` iterator):

.. code-block:: das

   // First match
   let result = select_node(root, "product[price > 40]/name")
   if (result.ok) {
       print("first expensive: {result.node.text as string}\n")
   }

   // All matches — iterate directly
   var ns = select_nodes(root, "product[price < 40]/@id")
   print("cheap products: {ns.size}\n")
   for (xn in ns) {
       let a = xn.attribute
       print("  {a as string}\n")
   }
   unsafe { delete ns; }

Practical example — querying ``books.xml``
===========================================

The tutorial ends by querying the real ``books.xml`` sample file:

.. code-block:: das

   open_xml("tutorials/dasPUGIXML/books.xml") <| $(doc, ok) {
       if (!ok) { return; }
       let root = doc.document_element

       var en_books = select_nodes(root, "book[@lang='en']")
       print("English books: {en_books.size}\n")
       unsafe { delete en_books; }

       let cheapest = select_text(root, "book[not(price > ../book/price)]/title")
       print("cheapest: {cheapest}\n")

       root |> for_each_select("book/author") <| $(xn) {
           print("  {xn.node.text as string}\n")
       }
   }

.. seealso::

   Full source: :download:`tutorials/dasPUGIXML/03_xpath.das <../../../../tutorials/dasPUGIXML/03_xpath.das>`

   Previous tutorial: :ref:`tutorial_dasPUGIXML_building`

   Next tutorial: :ref:`tutorial_dasPUGIXML_serialization`
