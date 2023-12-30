
.. _stdlib_rst:

=======================
Documentation generator
=======================

.. include:: detail/rst.rst

The RST module exposes collection of helper routines to automatically generate Daslang reStructuredText documentation.

All functions and symbols are in "rst" module, use require to get access to it. ::

    require daslib/rst


.. _struct-rst-DocGroup:

.. das:attribute:: DocGroup



DocGroup fields are

+------+----------------------------------------------------+
+name  +string                                              +
+------+----------------------------------------------------+
+func  +array< :ref:`ast::Function <handle-ast-Function>` ?>+
+------+----------------------------------------------------+
+hidden+bool                                                +
+------+----------------------------------------------------+


Group of functions with shared category.

++++++++++++++++
Document writers
++++++++++++++++

  *  :ref:`document (name:string const;mod:rtti::Module? const;fname:string const;substname:string const;groups:array\<rst::DocGroup\> const) : void <function-_at_rst_c__c_document_Cs_C1_ls_H_ls_rtti_c__c_Module_gr__gr_?_Cs_Cs_C1_ls_S_ls_rst_c__c_DocGroup_gr__gr_A>`

.. _function-_at_rst_c__c_document_Cs_C1_ls_H_ls_rtti_c__c_Module_gr__gr_?_Cs_Cs_C1_ls_S_ls_rst_c__c_DocGroup_gr__gr_A:

.. das:function:: document(name: string const; mod: rtti::Module? const; fname: string const; substname: string const; groups: array<rst::DocGroup> const)

+---------+---------------------------------------------------------+
+argument +argument type                                            +
+=========+=========================================================+
+name     +string const                                             +
+---------+---------------------------------------------------------+
+mod      + :ref:`rtti::Module <handle-rtti-Module>` ? const        +
+---------+---------------------------------------------------------+
+fname    +string const                                             +
+---------+---------------------------------------------------------+
+substname+string const                                             +
+---------+---------------------------------------------------------+
+groups   +array< :ref:`rst::DocGroup <struct-rst-DocGroup>` > const+
+---------+---------------------------------------------------------+


Document single module given list of `DocGropus`.
This will generate RST file with documentation for the module.
Functions which do not match any `DocGroup` will be placed in the `Uncategorized` group.

++++++++++++++++
Group operations
++++++++++++++++

  *  :ref:`group_by_regex (name:string const;mod:rtti::Module? const;reg:regex::Regex -const) : rst::DocGroup <function-_at_rst_c__c_group_by_regex_Cs_C1_ls_H_ls_rtti_c__c_Module_gr__gr_?_S_ls_regex_c__c_Regex_gr_>`
  *  :ref:`hide_group (group:rst::DocGroup -const) : rst::DocGroup <function-_at_rst_c__c_hide_group_S_ls_rst_c__c_DocGroup_gr_>`

.. _function-_at_rst_c__c_group_by_regex_Cs_C1_ls_H_ls_rtti_c__c_Module_gr__gr_?_S_ls_regex_c__c_Regex_gr_:

.. das:function:: group_by_regex(name: string const; mod: rtti::Module? const; reg: Regex)

group_by_regex returns  :ref:`rst::DocGroup <struct-rst-DocGroup>`

+--------+-------------------------------------------------+
+argument+argument type                                    +
+========+=================================================+
+name    +string const                                     +
+--------+-------------------------------------------------+
+mod     + :ref:`rtti::Module <handle-rtti-Module>` ? const+
+--------+-------------------------------------------------+
+reg     + :ref:`regex::Regex <struct-regex-Regex>`        +
+--------+-------------------------------------------------+


Creates a group of functions with shared category.
Functions will be added to the group if they match the regular expression.

.. _function-_at_rst_c__c_hide_group_S_ls_rst_c__c_DocGroup_gr_:

.. das:function:: hide_group(group: DocGroup)

hide_group returns  :ref:`rst::DocGroup <struct-rst-DocGroup>`

+--------+--------------------------------------------+
+argument+argument type                               +
+========+============================================+
+group   + :ref:`rst::DocGroup <struct-rst-DocGroup>` +
+--------+--------------------------------------------+


Marks the group as hidden.


