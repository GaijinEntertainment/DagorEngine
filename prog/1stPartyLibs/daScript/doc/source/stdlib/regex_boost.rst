
.. _stdlib_regex_boost:

=======================
Boost package for REGEX
=======================

.. include:: detail/regex_boost.rst

The REGEX boost module implements collection of helper macros and functions to accompany :ref:`REGEX <stdlib_regex>`.

All functions and symbols are in "regex_boost" module, use require to get access to it. ::

    require daslib/regex_boost


+++++++++++++
Reader macros
+++++++++++++

.. _call-macro-regex_boost-regex:

.. das:attribute:: regex

This macro implements embedding of the REGEX object into the AST::
  var op_regex <- %regex~operator[^a-zA-Z_]%%
Regex is compiled at the time of parsing, and the resulting object is embedded into the AST.


