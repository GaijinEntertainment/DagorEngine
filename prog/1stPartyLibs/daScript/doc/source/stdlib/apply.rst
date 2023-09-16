
.. _stdlib_apply:

========================
Apply reflection pattern
========================

.. include:: detail/apply.rst

Apply module implements `apply` pattern, i.e. static reflection dispatch for structures and other data types.

All functions and symbols are in "apply" module, use require to get access to it. ::

    require daslib/apply


+++++++++++
Call macros
+++++++++++

.. _call-macro-apply-apply:

.. das:attribute:: apply

This macro implements the apply() pattern. The idea is that for each entry in the structure, variant, or tuple,
the block will be invoked. Both element name, and element value are passed to the block.
For example

    struct Bar
        x, y : float
    apply([[Bar x=1.,y=2.]]) <| $ ( name:string; field )
        print("{name} = {field} ")

Would print x = 1.0 y = 2.0


