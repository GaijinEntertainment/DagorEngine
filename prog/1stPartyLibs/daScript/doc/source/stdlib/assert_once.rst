
.. _stdlib_assert_once:

===========
Assert once
===========

.. include:: detail/assert_once.rst

The assert_once module implements single-time assertion infrastructure.

All functions and symbols are in "assert_once" module, use require to get access to it. ::

    require daslib/assert_once

++++++++++++++++++++
Function annotations
++++++++++++++++++++

.. _handle-assert_once-AssertOnceMacro:

.. das:attribute:: AssertOnceMacro

This macro convert assert_once(expr,message) to the following code::

    var __assert_once_I = true  // this is a global variable
    if __assert_once_I && !expr
        __assert_once_I = false
        assert(false,message)

+++++++++
Assertion
+++++++++

  *  :ref:`assert_once (expr:bool const;message:string const) : void <function-_at_assert_once_c__c_assert_once_Cb_Cs>` 

.. _function-_at_assert_once_c__c_assert_once_Cb_Cs:

.. das:function:: assert_once(expr: bool const; message: string const)

+--------+-------------+
+argument+argument type+
+========+=============+
+expr    +bool const   +
+--------+-------------+
+message +string const +
+--------+-------------+


Same as assert, only the check will be not be repeated after the asseretion failed the first time.


