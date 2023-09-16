
.. _stdlib_constant_expression:

============================================
Constant expression checker and substitution
============================================

.. include:: detail/constexpr.rst

The constant_expression module implements `constant expression` function argument check, as well as argument substitution.

All functions and symbols are in "constexpr" module, use require to get access to it. ::

    require daslib/constant_expression

++++++++++++++++++++
Function annotations
++++++++++++++++++++

.. _handle-constant_expression-constexpr:

.. das:attribute:: constexpr

This macro implements a constexpr function argument checker. Given list of arguments to verify, it will fail for every one where non-constant expression is passed. For example::

    [constexpr (a)]
    def foo ( t:string; a : int )
        print("{t} = {a}\n")
    var BOO = 13
    [export]
    def main
        foo("blah", 1)
        foo("ouch", BOO)    // comilation error: `a is not a constexpr, BOO`

.. _handle-constant_expression-constant_expression:

.. das:attribute:: constant_expression

This function annotation implments constant expression folding for the given arguments.
When argument is specified in the annotation, and is passed as a contstant expression,
custom version of the function is generated, and an argument is substituted with a constant value.
This allows using of static_if expression on the said arguments, as well as other optimizations.
For example::

    [constant_expression(constString)]
    def take_const_arg(constString:string)
        print("constant string is = {constString}\n")   // note - constString here is not an argument


+++++++++++++
Macro helpers
+++++++++++++

  *  :ref:`isConstantExpression (expr:smart_ptr\<ast::Expression\> const) : bool <function-_at_constant_expression_c__c_isConstantExpression_CY_ls_ExpressionPtr_gr_1_ls_H_ls_ast_c__c_Expression_gr__gr_?M>` 

.. _function-_at_constant_expression_c__c_isConstantExpression_CY_ls_ExpressionPtr_gr_1_ls_H_ls_ast_c__c_Expression_gr__gr_?M:

.. das:function:: isConstantExpression(expr: ExpressionPtr)

isConstantExpression returns bool

+--------+--------------------------------------------+
+argument+argument type                               +
+========+============================================+
+expr    + :ref:`ExpressionPtr <alias-ExpressionPtr>` +
+--------+--------------------------------------------+


This macro function retrusn true if the expression is a constant expression


