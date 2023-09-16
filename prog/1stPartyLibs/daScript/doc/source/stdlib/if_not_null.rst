
.. _stdlib_if_not_null:

=================
if_not_null macro
=================

.. include:: detail/if_not_null.rst

The if_not_null module exposes single `if_not_null` pattern.

All functions and symbols are in "if_not_null" module, use require to get access to it. ::

    require daslib/if_not_null



+++++++++++
Call macros
+++++++++++

.. _call-macro-if_not_null-if_not_null:

.. das:attribute:: if_not_null

This macro transforms::

    ptr |> if_not_null <| call(...)

to::

    var _ptr_var = ptr
    if _ptr_var
        call(*_ptr_var,...)



