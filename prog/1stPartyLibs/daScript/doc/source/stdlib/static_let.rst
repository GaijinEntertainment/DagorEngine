
.. _stdlib_static_let:

================
static_let macro
================

.. include:: detail/static_let.rst

The static_let module implements static_let pattern, which allows declaration of private global variables which are local to a scope.

All functions and symbols are in "static_let" module, use require to get access to it. ::

    require daslib/static_let


++++++++++++++++++++
Function annotations
++++++++++++++++++++

.. _handle-static_let-StaticLetMacro:

.. das:attribute:: StaticLetMacro

This macro implements the `static_let` and `static_let_finalize` functions.

++++++++++++++++++++++++++++
Static variable declarations
++++++++++++++++++++++++++++

  *  :ref:`static_let (blk:block\<\> const) : void <function-_at_static_let_c__c_static_let_C_builtin_>` 
  *  :ref:`static_let_finalize (blk:block\<\> const) : void <function-_at_static_let_c__c_static_let_finalize_C_builtin_>` 

.. _function-_at_static_let_c__c_static_let_C_builtin_:

.. das:function:: static_let(blk: block<> const)

+--------+-------------+
+argument+argument type+
+========+=============+
+blk     +block<> const+
+--------+-------------+


Given a scope with the variable declarations, this function will make those variables global.
Variable will be renamed under the hood, and all local access to it will be renamed as well.

.. _function-_at_static_let_c__c_static_let_finalize_C_builtin_:

.. das:function:: static_let_finalize(blk: block<> const)

+--------+-------------+
+argument+argument type+
+========+=============+
+blk     +block<> const+
+--------+-------------+


This is very similar to regular static_let, but additionally the variable will be deleted on the context shutdown.


