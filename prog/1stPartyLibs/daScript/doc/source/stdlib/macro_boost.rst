
.. _stdlib_macro_boost:

======================================================
Boost package for the miscelanious macro manipulations
======================================================

.. include:: detail/macro_boost.rst

Apply module implements miscellaneous infrastructure which simplifies writing of macros.

All functions and symbols are in "macro_boost" module, use require to get access to it. ::

    require daslib/macro_boost



.. _struct-macro_boost-CapturedVariable:

.. das:attribute:: CapturedVariable



CapturedVariable fields are

+----------+---------------------------------------------+
+variable  + :ref:`ast::Variable <handle-ast-Variable>` ?+
+----------+---------------------------------------------+
+expression+ :ref:`ast::ExprVar <handle-ast-ExprVar>` ?  +
+----------+---------------------------------------------+


Stored captured variable together with the `ExprVar` which uses it

++++++++++++++++++++
Function annotations
++++++++++++++++++++

.. _handle-macro_boost-MacroVerifyMacro:

.. das:attribute:: MacroVerifyMacro

This macro implements `macro_verify` macro. It's equivalent to a function call::

    def macro_verify ( expr:bool; prog:ProgramPtr; at:LineInfo; message:string )

However, result will be substituted with::

    if !expr
        macro_error( prog, at, message )
        return [[ExpressionPtr]]
+++++++++++
Call macros
+++++++++++

.. _call-macro-macro_boost-return_skip_lockcheck:

.. das:attribute:: return_skip_lockcheck

this is similar to regular return <-, but it does not check for locks

++++++++++++++++++++++
Implementation details
++++++++++++++++++++++

  *  :ref:`macro_verify (expr:bool const;prog:smart_ptr\<rtti::Program\> const;at:rtti::LineInfo const;message:string const) : void <function-_at_macro_boost_c__c_macro_verify_Cb_CY_ls_ProgramPtr_gr_1_ls_H_ls_rtti_c__c_Program_gr__gr_?M_CH_ls_rtti_c__c_LineInfo_gr__Cs>` 

.. _function-_at_macro_boost_c__c_macro_verify_Cb_CY_ls_ProgramPtr_gr_1_ls_H_ls_rtti_c__c_Program_gr__gr_?M_CH_ls_rtti_c__c_LineInfo_gr__Cs:

.. das:function:: macro_verify(expr: bool const; prog: ProgramPtr; at: LineInfo const; message: string const)

+--------+----------------------------------------------------+
+argument+argument type                                       +
+========+====================================================+
+expr    +bool const                                          +
+--------+----------------------------------------------------+
+prog    + :ref:`ProgramPtr <alias-ProgramPtr>`               +
+--------+----------------------------------------------------+
+at      + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`  const+
+--------+----------------------------------------------------+
+message +string const                                        +
+--------+----------------------------------------------------+


Same as verify, only the check will produce macro error, followed by return [[ExpressionPtr]]

+++++++++++++
Uncategorized
+++++++++++++

.. _function-_at_macro_boost_c__c_capture_block_CY_ls_ExpressionPtr_gr_1_ls_H_ls_ast_c__c_Expression_gr__gr_?M:

.. das:function:: capture_block(expr: ExpressionPtr)

capture_block returns array< :ref:`macro_boost::CapturedVariable <struct-macro_boost-CapturedVariable>` >

+--------+--------------------------------------------+
+argument+argument type                               +
+========+============================================+
+expr    + :ref:`ExpressionPtr <alias-ExpressionPtr>` +
+--------+--------------------------------------------+


Collect all captured variables in the expression.

.. _function-_at_macro_boost_c__c_collect_finally_CY_ls_ExpressionPtr_gr_1_ls_H_ls_ast_c__c_Expression_gr__gr_?M:

.. das:function:: collect_finally(expr: ExpressionPtr)

collect_finally returns array< :ref:`ast::ExprBlock <handle-ast-ExprBlock>` ?>

+--------+--------------------------------------------+
+argument+argument type                               +
+========+============================================+
+expr    + :ref:`ExpressionPtr <alias-ExpressionPtr>` +
+--------+--------------------------------------------+


Collect all finally blocks in the expression.
Returns array of ExprBlock? with all the blocks which have `finally` section
Does not go into 'make_block' expression, such as `lambda`, or 'block' expressions

.. _function-_at_macro_boost_c__c_collect_labels_CY_ls_ExpressionPtr_gr_1_ls_H_ls_ast_c__c_Expression_gr__gr_?M:

.. das:function:: collect_labels(expr: ExpressionPtr)

collect_labels returns array<int>

+--------+--------------------------------------------+
+argument+argument type                               +
+========+============================================+
+expr    + :ref:`ExpressionPtr <alias-ExpressionPtr>` +
+--------+--------------------------------------------+


Collect all labels in the expression. Returns array of integer with label indices
Does not go into 'make_block' expression, such as `lambda`, or 'block' expressions


