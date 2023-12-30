
.. _stdlib_strings_boost:

=============================================
Boost package for string manipulation library
=============================================

.. include:: detail/strings_boost.rst

The STRINGS boost module implements collection of helper macros and functions to accompany :ref:`STRINGS <stdlib_strings>`.

All functions and symbols are in "strings_boost" module, use require to get access to it. ::

    require daslib/strings_boost


++++++++++++++
Split and join
++++++++++++++

  *  :ref:`split (text:string const implicit;delim:string const implicit) : array\<string\> <function-_at_strings_boost_c__c_split_CIs_CIs>` 
  *  :ref:`split_by_chars (text:string const implicit;delim:string const implicit) : array\<string\> <function-_at_strings_boost_c__c_split_by_chars_CIs_CIs>` 
  *  :ref:`join (it:auto const;separator:string const implicit) : auto <function-_at_strings_boost_c__c_join_C._CIs>` 
  *  :ref:`join (iterable:array\<auto(TT)\> const;separator:string const;blk:block\<(var writer:strings::StringBuilderWriter -const;elem:TT const):void\> const) : string <function-_at_strings_boost_c__c_join_C1_ls_Y_ls_TT_gr_._gr_A_Cs_CN_ls_writer;elem_gr_0_ls_H_ls_strings_c__c_StringBuilderWriter_gr_;CY_ls_TT_gr_L_gr_1_ls_v_gr__builtin_>` 
  *  :ref:`join (iterable:iterator\<auto(TT)\> const;separator:string const;blk:block\<(var writer:strings::StringBuilderWriter -const;elem:TT const):void\> const) : string <function-_at_strings_boost_c__c_join_C1_ls_Y_ls_TT_gr_._gr_G_Cs_CN_ls_writer;elem_gr_0_ls_H_ls_strings_c__c_StringBuilderWriter_gr_;CY_ls_TT_gr_L_gr_1_ls_v_gr__builtin_>` 
  *  :ref:`join (iterable:auto(TT) const[];separator:string const;blk:block\<(var writer:strings::StringBuilderWriter -const;elem:TT const):void\> const) : string <function-_at_strings_boost_c__c_join_C[-1]Y_ls_TT_gr_._Cs_CN_ls_writer;elem_gr_0_ls_H_ls_strings_c__c_StringBuilderWriter_gr_;CY_ls_TT_gr_L_gr_1_ls_v_gr__builtin_>` 
  *  :ref:`split (text:string const implicit;delim:string const implicit;blk:block\<(arg:array\<string\> const#):auto\> const) : auto <function-_at_strings_boost_c__c_split_CIs_CIs_CN_ls_arg_gr_0_ls_C_hh_1_ls_s_gr_A_gr_1_ls_._gr__builtin_>` 
  *  :ref:`split_by_chars (text:string const implicit;delim:string const implicit;blk:block\<(arg:array\<string\> const#):auto\> const) : auto <function-_at_strings_boost_c__c_split_by_chars_CIs_CIs_CN_ls_arg_gr_0_ls_C_hh_1_ls_s_gr_A_gr_1_ls_._gr__builtin_>` 

.. _function-_at_strings_boost_c__c_split_CIs_CIs:

.. das:function:: split(text: string const implicit; delim: string const implicit)

split returns array<string>

+--------+---------------------+
+argument+argument type        +
+========+=====================+
+text    +string const implicit+
+--------+---------------------+
+delim   +string const implicit+
+--------+---------------------+


|function-strings_boost-split|

.. _function-_at_strings_boost_c__c_split_by_chars_CIs_CIs:

.. das:function:: split_by_chars(text: string const implicit; delim: string const implicit)

split_by_chars returns array<string>

+--------+---------------------+
+argument+argument type        +
+========+=====================+
+text    +string const implicit+
+--------+---------------------+
+delim   +string const implicit+
+--------+---------------------+


|function-strings_boost-split_by_chars|

.. _function-_at_strings_boost_c__c_join_C._CIs:

.. das:function:: join(it: auto const; separator: string const implicit)

join returns auto

+---------+---------------------+
+argument +argument type        +
+=========+=====================+
+it       +auto const           +
+---------+---------------------+
+separator+string const implicit+
+---------+---------------------+


|function-strings_boost-join|

.. _function-_at_strings_boost_c__c_join_C1_ls_Y_ls_TT_gr_._gr_A_Cs_CN_ls_writer;elem_gr_0_ls_H_ls_strings_c__c_StringBuilderWriter_gr_;CY_ls_TT_gr_L_gr_1_ls_v_gr__builtin_:

.. das:function:: join(iterable: array<auto(TT)> const; separator: string const; blk: block<(var writer:strings::StringBuilderWriter -const;elem:TT const):void> const)

join returns string

+---------+-------------------------------------------------------------------------------------------------------------------+
+argument +argument type                                                                                                      +
+=========+===================================================================================================================+
+iterable +array<auto(TT)> const                                                                                              +
+---------+-------------------------------------------------------------------------------------------------------------------+
+separator+string const                                                                                                       +
+---------+-------------------------------------------------------------------------------------------------------------------+
+blk      +block<(writer: :ref:`strings::StringBuilderWriter <handle-strings-StringBuilderWriter>` ;elem:TT const):void> const+
+---------+-------------------------------------------------------------------------------------------------------------------+


|function-strings_boost-join|

.. _function-_at_strings_boost_c__c_join_C1_ls_Y_ls_TT_gr_._gr_G_Cs_CN_ls_writer;elem_gr_0_ls_H_ls_strings_c__c_StringBuilderWriter_gr_;CY_ls_TT_gr_L_gr_1_ls_v_gr__builtin_:

.. das:function:: join(iterable: iterator<auto(TT)> const; separator: string const; blk: block<(var writer:strings::StringBuilderWriter -const;elem:TT const):void> const)

join returns string

+---------+-------------------------------------------------------------------------------------------------------------------+
+argument +argument type                                                                                                      +
+=========+===================================================================================================================+
+iterable +iterator<auto(TT)> const                                                                                           +
+---------+-------------------------------------------------------------------------------------------------------------------+
+separator+string const                                                                                                       +
+---------+-------------------------------------------------------------------------------------------------------------------+
+blk      +block<(writer: :ref:`strings::StringBuilderWriter <handle-strings-StringBuilderWriter>` ;elem:TT const):void> const+
+---------+-------------------------------------------------------------------------------------------------------------------+


|function-strings_boost-join|

.. _function-_at_strings_boost_c__c_join_C[-1]Y_ls_TT_gr_._Cs_CN_ls_writer;elem_gr_0_ls_H_ls_strings_c__c_StringBuilderWriter_gr_;CY_ls_TT_gr_L_gr_1_ls_v_gr__builtin_:

.. das:function:: join(iterable: auto(TT) const[]; separator: string const; blk: block<(var writer:strings::StringBuilderWriter -const;elem:TT const):void> const)

join returns string

+---------+-------------------------------------------------------------------------------------------------------------------+
+argument +argument type                                                                                                      +
+=========+===================================================================================================================+
+iterable +auto(TT) const[-1]                                                                                                 +
+---------+-------------------------------------------------------------------------------------------------------------------+
+separator+string const                                                                                                       +
+---------+-------------------------------------------------------------------------------------------------------------------+
+blk      +block<(writer: :ref:`strings::StringBuilderWriter <handle-strings-StringBuilderWriter>` ;elem:TT const):void> const+
+---------+-------------------------------------------------------------------------------------------------------------------+


|function-strings_boost-join|

.. _function-_at_strings_boost_c__c_split_CIs_CIs_CN_ls_arg_gr_0_ls_C_hh_1_ls_s_gr_A_gr_1_ls_._gr__builtin_:

.. das:function:: split(text: string const implicit; delim: string const implicit; blk: block<(arg:array<string> const#):auto> const)

split returns auto

+--------+--------------------------------------------+
+argument+argument type                               +
+========+============================================+
+text    +string const implicit                       +
+--------+--------------------------------------------+
+delim   +string const implicit                       +
+--------+--------------------------------------------+
+blk     +block<(arg:array<string> const#):auto> const+
+--------+--------------------------------------------+


|function-strings_boost-split|

.. _function-_at_strings_boost_c__c_split_by_chars_CIs_CIs_CN_ls_arg_gr_0_ls_C_hh_1_ls_s_gr_A_gr_1_ls_._gr__builtin_:

.. das:function:: split_by_chars(text: string const implicit; delim: string const implicit; blk: block<(arg:array<string> const#):auto> const)

split_by_chars returns auto

+--------+--------------------------------------------+
+argument+argument type                               +
+========+============================================+
+text    +string const implicit                       +
+--------+--------------------------------------------+
+delim   +string const implicit                       +
+--------+--------------------------------------------+
+blk     +block<(arg:array<string> const#):auto> const+
+--------+--------------------------------------------+


|function-strings_boost-split_by_chars|

++++++++++
Formatting
++++++++++

  *  :ref:`wide (text:string const implicit;width:int const) : string <function-_at_strings_boost_c__c_wide_CIs_Ci>` 

.. _function-_at_strings_boost_c__c_wide_CIs_Ci:

.. das:function:: wide(text: string const implicit; width: int const)

wide returns string

+--------+---------------------+
+argument+argument type        +
+========+=====================+
+text    +string const implicit+
+--------+---------------------+
+width   +int const            +
+--------+---------------------+


|function-strings_boost-wide|

+++++++++++++++++++++++
Queries and comparisons
+++++++++++++++++++++++

  *  :ref:`is_character_at (foo:array\<uint8\> const implicit;idx:int const;ch:int const) : auto <function-_at_strings_boost_c__c_is_character_at_CI1_ls_u8_gr_A_Ci_Ci>` 
  *  :ref:`eq (a:string const implicit;b:$::das_string const) : auto <function-_at_strings_boost_c__c_eq_CIs_CH_ls__builtin__c__c_das_string_gr_>` 
  *  :ref:`eq (b:$::das_string const;a:string const implicit) : auto <function-_at_strings_boost_c__c_eq_CH_ls__builtin__c__c_das_string_gr__CIs>` 

.. _function-_at_strings_boost_c__c_is_character_at_CI1_ls_u8_gr_A_Ci_Ci:

.. das:function:: is_character_at(foo: array<uint8> const implicit; idx: int const; ch: int const)

is_character_at returns auto

+--------+---------------------------+
+argument+argument type              +
+========+===========================+
+foo     +array<uint8> const implicit+
+--------+---------------------------+
+idx     +int const                  +
+--------+---------------------------+
+ch      +int const                  +
+--------+---------------------------+


|function-strings_boost-is_character_at|

.. _function-_at_strings_boost_c__c_eq_CIs_CH_ls__builtin__c__c_das_string_gr_:

.. das:function:: eq(a: string const implicit; b: das_string const)

eq returns auto

+--------+--------------------------------------------------------------+
+argument+argument type                                                 +
+========+==============================================================+
+a       +string const implicit                                         +
+--------+--------------------------------------------------------------+
+b       + :ref:`builtin::das_string <handle-builtin-das_string>`  const+
+--------+--------------------------------------------------------------+


|function-strings_boost-eq|

.. _function-_at_strings_boost_c__c_eq_CH_ls__builtin__c__c_das_string_gr__CIs:

.. das:function:: eq(b: das_string const; a: string const implicit)

eq returns auto

+--------+--------------------------------------------------------------+
+argument+argument type                                                 +
+========+==============================================================+
+b       + :ref:`builtin::das_string <handle-builtin-das_string>`  const+
+--------+--------------------------------------------------------------+
+a       +string const implicit                                         +
+--------+--------------------------------------------------------------+


|function-strings_boost-eq|

+++++++
Replace
+++++++

  *  :ref:`replace_multiple (source:string const;replaces:array\<tuple\<text:string;replacement:string\>\> const) : string const <function-_at_strings_boost_c__c_replace_multiple_Cs_C1_ls_N_ls_text;replacement_gr_0_ls_s;s_gr_U_gr_A>` 

.. _function-_at_strings_boost_c__c_replace_multiple_Cs_C1_ls_N_ls_text;replacement_gr_0_ls_s;s_gr_U_gr_A:

.. das:function:: replace_multiple(source: string const; replaces: array<tuple<text:string;replacement:string>> const)

replace_multiple returns string const

+--------+--------------------------------------------------+
+argument+argument type                                     +
+========+==================================================+
+source  +string const                                      +
+--------+--------------------------------------------------+
+replaces+array<tuple<text:string;replacement:string>> const+
+--------+--------------------------------------------------+


|function-strings_boost-replace_multiple|

++++++++++++++++++++
Levenshtein distance
++++++++++++++++++++

  *  :ref:`levenshtein_distance (s:string const implicit;t:string const implicit) : int <function-_at_strings_boost_c__c_levenshtein_distance_CIs_CIs>` 
  *  :ref:`levenshtein_distance_fast (s:string const implicit;t:string const implicit) : int <function-_at_strings_boost_c__c_levenshtein_distance_fast_CIs_CIs>` 

.. _function-_at_strings_boost_c__c_levenshtein_distance_CIs_CIs:

.. das:function:: levenshtein_distance(s: string const implicit; t: string const implicit)

levenshtein_distance returns int

+--------+---------------------+
+argument+argument type        +
+========+=====================+
+s       +string const implicit+
+--------+---------------------+
+t       +string const implicit+
+--------+---------------------+


|function-strings_boost-levenshtein_distance|

.. _function-_at_strings_boost_c__c_levenshtein_distance_fast_CIs_CIs:

.. das:function:: levenshtein_distance_fast(s: string const implicit; t: string const implicit)

levenshtein_distance_fast returns int

+--------+---------------------+
+argument+argument type        +
+========+=====================+
+s       +string const implicit+
+--------+---------------------+
+t       +string const implicit+
+--------+---------------------+


|function-strings_boost-levenshtein_distance_fast|

++++++++++++++++
Character traits
++++++++++++++++

  *  :ref:`is_hex (ch:int const) : bool <function-_at_strings_boost_c__c_is_hex_Ci>` 
  *  :ref:`is_tab_or_space (ch:int const) : bool <function-_at_strings_boost_c__c_is_tab_or_space_Ci>` 

.. _function-_at_strings_boost_c__c_is_hex_Ci:

.. das:function:: is_hex(ch: int const)

is_hex returns bool

+--------+-------------+
+argument+argument type+
+========+=============+
+ch      +int const    +
+--------+-------------+


|function-strings_boost-is_hex|

.. _function-_at_strings_boost_c__c_is_tab_or_space_Ci:

.. das:function:: is_tab_or_space(ch: int const)

is_tab_or_space returns bool

+--------+-------------+
+argument+argument type+
+========+=============+
+ch      +int const    +
+--------+-------------+


|function-strings_boost-is_tab_or_space|


