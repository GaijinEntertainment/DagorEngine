
.. _stdlib_sort_boost:

==================================
Boost package for the builtin sort
==================================

.. include:: detail/sort_boost.rst

The sort_boost module implements additional infrastructure for the sorting routines.

All functions and symbols are in "sort_boost" module, use require to get access to it. ::

    require daslib/sort_boost

+++++++++++
Call macros
+++++++++++

.. _call-macro-sort_boost-qsort:

.. das:attribute:: qsort

Implements `qsort` macro. I'ts `qsort(value,block)`.
For the regular array<> or dim it's replaced with `sort(value,block)`.
For the handled types like das`vector its replaced with `sort(temp_array(value),block)`.


