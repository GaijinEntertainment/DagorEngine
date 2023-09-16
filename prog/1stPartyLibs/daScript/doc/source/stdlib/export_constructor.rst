
.. _stdlib_export_constructor:

==================
Export constructor
==================

.. include:: detail/export_constructor.rst

The export_constructor module simplifies creation of daScript structure and classes from C++ side.

In the following example::

    [export_constructor]
    class Foo {}

Function make`Foo is generated with an export flag; it returns new Foo() object.

All functions and symbols are in "export_constructor" module, use require to get access to it. ::

    require daslib/export_constructor



++++++++++++++++
Structure macros
++++++++++++++++

.. _handle-export_constructor-export_constructor:

.. das:attribute:: export_constructor

implements 'export_constructor' macro, adds function make`{StructureName} which makes a new instance of a class or structure


