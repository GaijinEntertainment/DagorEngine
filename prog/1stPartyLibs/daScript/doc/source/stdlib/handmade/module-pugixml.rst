The PUGIXML module provides XML parsing, navigation, manipulation, and XPath query
support built on top of the `pugixml <https://pugixml.org/>`_ C++ library. It exposes
document loading/saving, DOM-style node and attribute access, text content helpers,
and a full XPath 1.0 evaluation engine.

Use ``PUGIXML_boost`` for high-level helpers such as RAII document handling,
iterator-based traversal, builder EDSL, and struct ↔ XML serialization.
The low-level C++ bindings live in this module.

All functions and symbols are in "pugixml" module, use require to get access to it.

.. code-block:: das

    require pugixml

See also:

  * :ref:`PUGIXML_boost <stdlib_PUGIXML_boost>` — high-level helpers and serialization
  * `Tutorial 01 — Parsing and navigation <https://github.com/GaijinEntertainment/daScript/blob/master/tutorials/dasPUGIXML/01_parsing_and_navigation.das>`_
  * `Tutorial 02 — Building XML <https://github.com/GaijinEntertainment/daScript/blob/master/tutorials/dasPUGIXML/02_building_xml.das>`_
  * `Tutorial 03 — XPath <https://github.com/GaijinEntertainment/daScript/blob/master/tutorials/dasPUGIXML/03_xpath.das>`_
  * `Tutorial 04 — Serialization <https://github.com/GaijinEntertainment/daScript/blob/master/tutorials/dasPUGIXML/04_serialization.das>`_
