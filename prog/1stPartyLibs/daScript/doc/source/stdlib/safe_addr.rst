
.. _stdlib_safe_addr:

===============
safe_addr macro
===============

.. include:: detail/safe_addr.rst

The safe_addr module implements safe_addr pattern, which returns temporary address of local expression.

All functions and symbols are in "safe_addr" module, use require to get access to it. ::

    require daslib/safe_addr


++++++++++++++++++++
Function annotations
++++++++++++++++++++

.. _handle-safe_addr-SafeAddrMacro:

.. das:attribute:: SafeAddrMacro

This macro reports an error if safe_addr is attempted on the object, which is not local to the scope.
I.e. if the object can `expire` while in scope, with delete, garbage collection, or on the C++ side.

.. _handle-safe_addr-SharedAddrMacro:

.. das:attribute:: SharedAddrMacro

|function_annotation-safe_addr-SharedAddrMacro|

++++++++++++++++++++++
Safe temporary address
++++++++++++++++++++++

  *  :ref:`safe_addr (x:auto(T)& ==const -const) : T -&?# <function-_at_safe_addr_c__c_safe_addr_&_eq_Y_ls_T_gr_.>` 
  *  :ref:`safe_addr (x:auto(T) const& ==const) : T -&? const# <function-_at_safe_addr_c__c_safe_addr_C&_eq_Y_ls_T_gr_.>` 
  *  :ref:`shared_addr (tab:table\<auto(KEY);auto(VAL)\> const;k:KEY const) : auto <function-_at_safe_addr_c__c_shared_addr_C1_ls_Y_ls_KEY_gr_._gr_2_ls_Y_ls_VAL_gr_._gr_T_CY_ls_KEY_gr_L>` 
  *  :ref:`shared_addr (val:auto(VALUE) const&) : auto <function-_at_safe_addr_c__c_shared_addr_C&Y_ls_VALUE_gr_.>` 

.. _function-_at_safe_addr_c__c_safe_addr_&_eq_Y_ls_T_gr_.:

.. das:function:: safe_addr(x: auto(T)& ==const)

safe_addr returns T?#

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +auto(T)&!    +
+--------+-------------+


returns temporary pointer to the given expression

.. _function-_at_safe_addr_c__c_safe_addr_C&_eq_Y_ls_T_gr_.:

.. das:function:: safe_addr(x: auto(T) const& ==const)

safe_addr returns T? const#

+--------+---------------+
+argument+argument type  +
+========+===============+
+x       +auto(T) const&!+
+--------+---------------+


returns temporary pointer to the given expression

.. _function-_at_safe_addr_c__c_shared_addr_C1_ls_Y_ls_KEY_gr_._gr_2_ls_Y_ls_VAL_gr_._gr_T_CY_ls_KEY_gr_L:

.. das:function:: shared_addr(tab: table<auto(KEY);auto(VAL)> const; k: KEY const)

shared_addr returns auto

+--------+--------------------------------+
+argument+argument type                   +
+========+================================+
+tab     +table<auto(KEY);auto(VAL)> const+
+--------+--------------------------------+
+k       +KEY const                       +
+--------+--------------------------------+


returns address of the given shared variable. it's safe because shared variables never go out of scope

.. _function-_at_safe_addr_c__c_shared_addr_C&Y_ls_VALUE_gr_.:

.. das:function:: shared_addr(val: auto(VALUE) const&)

shared_addr returns auto

+--------+------------------+
+argument+argument type     +
+========+==================+
+val     +auto(VALUE) const&+
+--------+------------------+


returns address of the given shared variable. it's safe because shared variables never go out of scope

++++++++++++++++++
Temporary pointers
++++++++++++++++++

  *  :ref:`temp_ptr (x:auto(T)? const implicit ==const) : T? const# <function-_at_safe_addr_c__c_temp_ptr_CI_eq_1_ls_Y_ls_T_gr_._gr_?>` 
  *  :ref:`temp_ptr (x:auto(T)? implicit ==const -const) : T?# <function-_at_safe_addr_c__c_temp_ptr_I_eq_1_ls_Y_ls_T_gr_._gr_?>` 

.. _function-_at_safe_addr_c__c_temp_ptr_CI_eq_1_ls_Y_ls_T_gr_._gr_?:

.. das:function:: temp_ptr(x: auto(T)? const implicit ==const)

temp_ptr returns T? const#

+--------+------------------------+
+argument+argument type           +
+========+========================+
+x       +auto(T)? const implicit!+
+--------+------------------------+


returns temporary pointer from a given pointer

.. _function-_at_safe_addr_c__c_temp_ptr_I_eq_1_ls_Y_ls_T_gr_._gr_?:

.. das:function:: temp_ptr(x: auto(T)? implicit ==const)

temp_ptr returns T?#

+--------+------------------+
+argument+argument type     +
+========+==================+
+x       +auto(T)? implicit!+
+--------+------------------+


returns temporary pointer from a given pointer


