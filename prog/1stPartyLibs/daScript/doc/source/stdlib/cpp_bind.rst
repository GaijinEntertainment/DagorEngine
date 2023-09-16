
.. _stdlib_cpp_bind:

======================
C++ bindings generator
======================

.. include:: detail/cpp_bind.rst

The cpp_bind module implements generation of C++ bindings for the daScript interfaces.

All functions and symbols are in "cpp_bind" module, use require to get access to it. ::

    require daslib/cpp_bind

For example, from tutorial04.das ::

    require fio
    require ast
    require daslib/cpp_bind
    [init]
    def generate_cpp_bindings
        let root = get_das_root() + "/examples/tutorial/"
        fopen(root + "tutorial04_gen.inc","wb") <| $ ( cpp_file )
            log_cpp_class_adapter(cpp_file, "TutorialBaseClass", typeinfo(ast_typedecl type<TutorialBaseClass>))

++++++++++++++++++++++
Generation of bindings
++++++++++++++++++++++

  *  :ref:`log_cpp_class_adapter (cpp_file:fio::FILE const? const;name:string const;cinfo:smart_ptr\<ast::TypeDecl\> const) : void <function-_at_cpp_bind_c__c_log_cpp_class_adapter_CY_ls_file_gr_1_ls_CH_ls_fio_c__c_FILE_gr__gr_?_Cs_CY_ls_TypeDeclPtr_gr_1_ls_H_ls_ast_c__c_TypeDecl_gr__gr_?M>` 

.. _function-_at_cpp_bind_c__c_log_cpp_class_adapter_CY_ls_file_gr_1_ls_CH_ls_fio_c__c_FILE_gr__gr_?_Cs_CY_ls_TypeDeclPtr_gr_1_ls_H_ls_ast_c__c_TypeDecl_gr__gr_?M:

.. das:function:: log_cpp_class_adapter(cpp_file: file; name: string const; cinfo: TypeDeclPtr)

+--------+----------------------------------------+
+argument+argument type                           +
+========+========================================+
+cpp_file+ :ref:`file <alias-file>`               +
+--------+----------------------------------------+
+name    +string const                            +
+--------+----------------------------------------+
+cinfo   + :ref:`TypeDeclPtr <alias-TypeDeclPtr>` +
+--------+----------------------------------------+


Generates C++ class adapter for the daScript class.
Intended use::

    log_cpp_class_adapter(cppFileNameDotInc, "daScriptClassName", typeinfo(ast_typedecl type<daScriptClassName>))


