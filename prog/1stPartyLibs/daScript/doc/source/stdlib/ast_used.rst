
.. _stdlib_ast_used:

==========================
AST type ussage collection
==========================

.. include:: detail/ast_used.rst

The ast_used module implements type collecting infrasturcture. It allows to determine, if enumeration and structure types are used in the code.

All functions and symbols are in "ast_used" module, use require to get access to it. ::

    require daslib/ast_used

.. _struct-ast_used-OnlyUsedTypes:

.. das:attribute:: OnlyUsedTypes



OnlyUsedTypes fields are

+--+---------------------------------------------------------------+
+st+table< :ref:`ast::Structure <handle-ast-Structure>` ?;bool>    +
+--+---------------------------------------------------------------+
+en+table< :ref:`ast::Enumeration <handle-ast-Enumeration>` ?;bool>+
+--+---------------------------------------------------------------+


Collection of all structure and enumeration types that are used in the AST.

+++++++++++++++++++++++++++
Collecting type information
+++++++++++++++++++++++++++

  *  :ref:`collect_used_types (vfun:array\<ast::Function?\> const;vvar:array\<ast::Variable?\> const;blk:block\<(usedTypes:ast_used::OnlyUsedTypes const):void\> const) : void <function-_at_ast_used_c__c_collect_used_types_C1_ls_1_ls_H_ls_ast_c__c_Function_gr__gr_?_gr_A_C1_ls_1_ls_H_ls_ast_c__c_Variable_gr__gr_?_gr_A_CN_ls_usedTypes_gr_0_ls_CS_ls_ast_used_c__c_OnlyUsedTypes_gr__gr_1_ls_v_gr__builtin_>` 

.. _function-_at_ast_used_c__c_collect_used_types_C1_ls_1_ls_H_ls_ast_c__c_Function_gr__gr_?_gr_A_C1_ls_1_ls_H_ls_ast_c__c_Variable_gr__gr_?_gr_A_CN_ls_usedTypes_gr_0_ls_CS_ls_ast_used_c__c_OnlyUsedTypes_gr__gr_1_ls_v_gr__builtin_:

.. das:function:: collect_used_types(vfun: array<ast::Function?> const; vvar: array<ast::Variable?> const; blk: block<(usedTypes:ast_used::OnlyUsedTypes const):void> const)

+--------+----------------------------------------------------------------------------------------------------+
+argument+argument type                                                                                       +
+========+====================================================================================================+
+vfun    +array< :ref:`ast::Function <handle-ast-Function>` ?> const                                          +
+--------+----------------------------------------------------------------------------------------------------+
+vvar    +array< :ref:`ast::Variable <handle-ast-Variable>` ?> const                                          +
+--------+----------------------------------------------------------------------------------------------------+
+blk     +block<(usedTypes: :ref:`ast_used::OnlyUsedTypes <struct-ast_used-OnlyUsedTypes>`  const):void> const+
+--------+----------------------------------------------------------------------------------------------------+


Goes through list of functions `vfun` and variables `vvar` and collects list of which enumeration and structure types are used in them.
Calls `blk` with said list.


