
.. _stdlib_templates:

===============================================
decltype macro and template function annotation
===============================================

.. include:: detail/templates.rst

The templates exposes collection of template-like routines for Daslang.

All functions and symbols are in "templates" module, use require to get access to it. ::

    require daslib/templates


++++++++++++++++++++
Function annotations
++++++++++++++++++++

.. _handle-templates-template:

.. das:attribute:: template

This macro is used to remove unused (template) arguments from the instantiation of the generic function.
When [template(x)] is specified, the argument x is removed from the function call, but the type of the instance remains.
The call where the function is instanciated is adjusted as well.
For example::

    [template (a), sideeffects]
    def boo ( x : int; a : auto(TT) )   // when boo(1,type<int>)
        return "{x}_{typeinfo(typename type<TT>)}"
    ...
    boo(1,type<int>) // will be replaced with boo(1). instace will print "1_int"


+++++++++++
Call macros
+++++++++++

.. _call-macro-templates-decltype:

.. das:attribute:: decltype

This macro returns ast::TypeDecl for the corresponding expression. For example::

    let x = 1
    let y <- decltype(x) // [[TypeDecl() baseType==Type tInt, flags=TypeDeclFlags constant | TypeDeclFlags ref]]


.. _call-macro-templates-decltype_noref:

.. das:attribute:: decltype_noref

This macro returns TypeDecl for the corresponding expression, minus the ref (&) portion.


