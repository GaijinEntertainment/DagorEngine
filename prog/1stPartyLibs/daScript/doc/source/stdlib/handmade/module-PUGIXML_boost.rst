The PUGIXML_BOOST module provides high-level daScript helpers on top of the
low-level ``pugixml`` C++ bindings.

Includes RAII document management (``open_xml``, ``parse_xml``, ``with_doc``),
iterator-based traversal (``each``, ``each_child``, ``each_attribute``),
a builder EDSL (``tag``, ``attr``), quick attribute/text accessors, XPath
convenience functions, ``is``/``as`` type-conversion operators, and generic
struct ↔ XML serialization (``to_XML``, ``from_XML``, ``XML``).

All functions and symbols are in "PUGIXML_boost" module, use require to get access to it.

.. code-block:: das

    require pugixml/PUGIXML_boost

See also:

  * :ref:`pugixml <stdlib_pugixml>` — low-level C++ bindings
  * `Tutorial 01 — Parsing and navigation <https://github.com/GaijinEntertainment/daScript/blob/master/tutorials/dasPUGIXML/01_parsing_and_navigation.das>`_
  * `Tutorial 02 — Building XML <https://github.com/GaijinEntertainment/daScript/blob/master/tutorials/dasPUGIXML/02_building_xml.das>`_
  * `Tutorial 03 — XPath <https://github.com/GaijinEntertainment/daScript/blob/master/tutorials/dasPUGIXML/03_xpath.das>`_
  * `Tutorial 04 — Serialization <https://github.com/GaijinEntertainment/daScript/blob/master/tutorials/dasPUGIXML/04_serialization.das>`_
