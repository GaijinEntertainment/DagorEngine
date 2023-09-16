
.. _stdlib_is_local:

========================
is_local_xxx ast helpers
========================

.. include:: detail/is_local.rst

The is_local module exposes collection of helper routines to establish locality of expression.

All functions and symbols are in "is_local" module, use require to get access to it. ::

    require daslib/is_local


++++++++++++
Scope checks
++++++++++++

  *  :ref:`is_local_expr (expr:smart_ptr\<ast::Expression\> const) : bool const <function-_at_is_local_c__c_is_local_expr_CY_ls_ExpressionPtr_gr_1_ls_H_ls_ast_c__c_Expression_gr__gr_?M>` 
  *  :ref:`is_local_or_global_expr (expr:smart_ptr\<ast::Expression\> const) : bool const <function-_at_is_local_c__c_is_local_or_global_expr_CY_ls_ExpressionPtr_gr_1_ls_H_ls_ast_c__c_Expression_gr__gr_?M>` 
  *  :ref:`is_scope_expr (expr:smart_ptr\<ast::Expression\> const) : bool const <function-_at_is_local_c__c_is_scope_expr_CY_ls_ExpressionPtr_gr_1_ls_H_ls_ast_c__c_Expression_gr__gr_?M>` 

.. _function-_at_is_local_c__c_is_local_expr_CY_ls_ExpressionPtr_gr_1_ls_H_ls_ast_c__c_Expression_gr__gr_?M:

.. das:function:: is_local_expr(expr: ExpressionPtr)

is_local_expr returns bool const

+--------+--------------------------------------------+
+argument+argument type                               +
+========+============================================+
+expr    + :ref:`ExpressionPtr <alias-ExpressionPtr>` +
+--------+--------------------------------------------+


Returns true if the expression is local to the current scope.

.. _function-_at_is_local_c__c_is_local_or_global_expr_CY_ls_ExpressionPtr_gr_1_ls_H_ls_ast_c__c_Expression_gr__gr_?M:

.. das:function:: is_local_or_global_expr(expr: ExpressionPtr)

is_local_or_global_expr returns bool const

+--------+--------------------------------------------+
+argument+argument type                               +
+========+============================================+
+expr    + :ref:`ExpressionPtr <alias-ExpressionPtr>` +
+--------+--------------------------------------------+


Returns true if expression is local the current scope or global scope.

.. _function-_at_is_local_c__c_is_scope_expr_CY_ls_ExpressionPtr_gr_1_ls_H_ls_ast_c__c_Expression_gr__gr_?M:

.. das:function:: is_scope_expr(expr: ExpressionPtr)

is_scope_expr returns bool const

+--------+--------------------------------------------+
+argument+argument type                               +
+========+============================================+
+expr    + :ref:`ExpressionPtr <alias-ExpressionPtr>` +
+--------+--------------------------------------------+


Returns true if the expression is a scoped expression, i.e. eventually points to a variable.

+++++++++++++
Uncategorized
+++++++++++++

.. _function-_at_is_local_c__c_is_shared_expr_CY_ls_ExpressionPtr_gr_1_ls_H_ls_ast_c__c_Expression_gr__gr_?M:

.. das:function:: is_shared_expr(expr: ExpressionPtr)

is_shared_expr returns bool const

+--------+--------------------------------------------+
+argument+argument type                               +
+========+============================================+
+expr    + :ref:`ExpressionPtr <alias-ExpressionPtr>` +
+--------+--------------------------------------------+


Returns true if the expression is local to the current scope.


