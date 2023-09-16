
.. _stdlib_enum_trait:

==================
Enumeration traits
==================

.. include:: detail/enum_trait.rst

The enum_trait module implements typeinfo traits for the enumerations.

All functions and symbols are in "enum_trait" module, use require to get access to it. ::

    require daslib/enum_trait

+++++++++++++++
Typeinfo macros
+++++++++++++++

.. _call-macro-enum_trait-enum_length:

.. das:attribute:: enum_length

Implements typeinfo(enum_length EnumOrEnumType) which returns total number of elements in enumeration.

.. _call-macro-enum_trait-enum_names:

.. das:attribute:: enum_names

Implements typeinfo(enum_names EnumOrEnumType) which returns array of strings with enumValue names.


