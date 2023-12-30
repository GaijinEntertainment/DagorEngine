
.. _stdlib_strings:

===========================
String manipulation library
===========================

.. include:: detail/strings.rst

The string library implements string formatting, conversion, searching, and modification routines.

All functions and symbols are in "strings" module, use require to get access to it. ::

    require strings


++++++++++++++++++
Handled structures
++++++++++++++++++

.. _handle-strings-StringBuilderWriter:

.. das:attribute:: StringBuilderWriter

|structure_annotation-strings-StringBuilderWriter|

+++++++++++++
Character set
+++++++++++++

  *  :ref:`is_char_in_set (Character:int const;Charset:uint const[8] implicit;Context:__context const;At:__lineInfo const) : bool <function-_at_strings_c__c_is_char_in_set_Ci_CI[8]u_C_c_C_l>` 
  *  :ref:`set_total (Charset:uint const[8] implicit) : uint <function-_at_strings_c__c_set_total_CI[8]u>` 
  *  :ref:`set_element (Character:int const;Charset:uint const[8] implicit) : int <function-_at_strings_c__c_set_element_Ci_CI[8]u>` 

.. _function-_at_strings_c__c_is_char_in_set_Ci_CI[8]u_C_c_C_l:

.. das:function:: is_char_in_set(Character: int const; Charset: uint const[8] implicit)

is_char_in_set returns bool

+---------+----------------------+
+argument +argument type         +
+=========+======================+
+Character+int const             +
+---------+----------------------+
+Charset  +uint const[8] implicit+
+---------+----------------------+


|function-strings-is_char_in_set|

.. _function-_at_strings_c__c_set_total_CI[8]u:

.. das:function:: set_total(Charset: uint const[8] implicit)

set_total returns uint

+--------+----------------------+
+argument+argument type         +
+========+======================+
+Charset +uint const[8] implicit+
+--------+----------------------+


|function-strings-set_total|

.. _function-_at_strings_c__c_set_element_Ci_CI[8]u:

.. das:function:: set_element(Character: int const; Charset: uint const[8] implicit)

set_element returns int

+---------+----------------------+
+argument +argument type         +
+=========+======================+
+Character+int const             +
+---------+----------------------+
+Charset  +uint const[8] implicit+
+---------+----------------------+


|function-strings-set_element|

++++++++++++++++
Character groups
++++++++++++++++

  *  :ref:`is_alpha (Character:int const) : bool <function-_at_strings_c__c_is_alpha_Ci>` 
  *  :ref:`is_new_line (Character:int const) : bool <function-_at_strings_c__c_is_new_line_Ci>` 
  *  :ref:`is_white_space (Character:int const) : bool <function-_at_strings_c__c_is_white_space_Ci>` 
  *  :ref:`is_number (Character:int const) : bool <function-_at_strings_c__c_is_number_Ci>` 

.. _function-_at_strings_c__c_is_alpha_Ci:

.. das:function:: is_alpha(Character: int const)

is_alpha returns bool

+---------+-------------+
+argument +argument type+
+=========+=============+
+Character+int const    +
+---------+-------------+


|function-strings-is_alpha|

.. _function-_at_strings_c__c_is_new_line_Ci:

.. das:function:: is_new_line(Character: int const)

is_new_line returns bool

+---------+-------------+
+argument +argument type+
+=========+=============+
+Character+int const    +
+---------+-------------+


|function-strings-is_new_line|

.. _function-_at_strings_c__c_is_white_space_Ci:

.. das:function:: is_white_space(Character: int const)

is_white_space returns bool

+---------+-------------+
+argument +argument type+
+=========+=============+
+Character+int const    +
+---------+-------------+


|function-strings-is_white_space|

.. _function-_at_strings_c__c_is_number_Ci:

.. das:function:: is_number(Character: int const)

is_number returns bool

+---------+-------------+
+argument +argument type+
+=========+=============+
+Character+int const    +
+---------+-------------+


|function-strings-is_number|

++++++++++++++++++
Character by index
++++++++++++++++++

  *  :ref:`character_at (str:string const implicit;idx:int const;context:__context const;at:__lineInfo const) : int <function-_at_strings_c__c_character_at_CIs_Ci_C_c_C_l>` 
  *  :ref:`character_uat (str:string const implicit;idx:int const) : int <function-_at_strings_c__c_character_uat_CIs_Ci>` 

.. _function-_at_strings_c__c_character_at_CIs_Ci_C_c_C_l:

.. das:function:: character_at(str: string const implicit; idx: int const)

character_at returns int

+--------+---------------------+
+argument+argument type        +
+========+=====================+
+str     +string const implicit+
+--------+---------------------+
+idx     +int const            +
+--------+---------------------+


|function-strings-character_at|

.. _function-_at_strings_c__c_character_uat_CIs_Ci:

.. das:function:: character_uat(str: string const implicit; idx: int const)

character_uat returns int

.. warning:: 
  This is unsafe operation.

+--------+---------------------+
+argument+argument type        +
+========+=====================+
+str     +string const implicit+
+--------+---------------------+
+idx     +int const            +
+--------+---------------------+


|function-strings-character_uat|

+++++++++++++++++
String properties
+++++++++++++++++

  *  :ref:`ends_with (str:string const implicit;cmp:string const implicit;context:__context const) : bool <function-_at_strings_c__c_ends_with_CIs_CIs_C_c>` 
  *  :ref:`ends_with (str:$::das_string const implicit;cmp:string const implicit;context:__context const) : bool <function-_at_strings_c__c_ends_with_CIH_ls__builtin__c__c_das_string_gr__CIs_C_c>` 
  *  :ref:`starts_with (str:string const implicit;cmp:string const implicit;context:__context const) : bool <function-_at_strings_c__c_starts_with_CIs_CIs_C_c>` 
  *  :ref:`starts_with (str:string const implicit;cmp:string const implicit;cmpLen:uint const;context:__context const) : bool <function-_at_strings_c__c_starts_with_CIs_CIs_Cu_C_c>` 
  *  :ref:`starts_with (str:string const implicit;offset:int const;cmp:string const implicit;context:__context const) : bool <function-_at_strings_c__c_starts_with_CIs_Ci_CIs_C_c>` 
  *  :ref:`starts_with (str:string const implicit;offset:int const;cmp:string const implicit;cmpLen:uint const;context:__context const) : bool <function-_at_strings_c__c_starts_with_CIs_Ci_CIs_Cu_C_c>` 
  *  :ref:`length (str:string const implicit;context:__context const) : int <function-_at_strings_c__c_length_CIs_C_c>` 
  *  :ref:`length (str:$::das_string const implicit) : int <function-_at_strings_c__c_length_CIH_ls__builtin__c__c_das_string_gr_>` 

.. _function-_at_strings_c__c_ends_with_CIs_CIs_C_c:

.. das:function:: ends_with(str: string const implicit; cmp: string const implicit)

ends_with returns bool

+--------+---------------------+
+argument+argument type        +
+========+=====================+
+str     +string const implicit+
+--------+---------------------+
+cmp     +string const implicit+
+--------+---------------------+


|function-strings-ends_with|

.. _function-_at_strings_c__c_ends_with_CIH_ls__builtin__c__c_das_string_gr__CIs_C_c:

.. das:function:: ends_with(str: das_string const implicit; cmp: string const implicit)

ends_with returns bool

+--------+-----------------------------------------------------------------------+
+argument+argument type                                                          +
+========+=======================================================================+
+str     + :ref:`builtin::das_string <handle-builtin-das_string>`  const implicit+
+--------+-----------------------------------------------------------------------+
+cmp     +string const implicit                                                  +
+--------+-----------------------------------------------------------------------+


|function-strings-ends_with|

.. _function-_at_strings_c__c_starts_with_CIs_CIs_C_c:

.. das:function:: starts_with(str: string const implicit; cmp: string const implicit)

starts_with returns bool

+--------+---------------------+
+argument+argument type        +
+========+=====================+
+str     +string const implicit+
+--------+---------------------+
+cmp     +string const implicit+
+--------+---------------------+


|function-strings-starts_with|

.. _function-_at_strings_c__c_starts_with_CIs_CIs_Cu_C_c:

.. das:function:: starts_with(str: string const implicit; cmp: string const implicit; cmpLen: uint const)

starts_with returns bool

+--------+---------------------+
+argument+argument type        +
+========+=====================+
+str     +string const implicit+
+--------+---------------------+
+cmp     +string const implicit+
+--------+---------------------+
+cmpLen  +uint const           +
+--------+---------------------+


|function-strings-starts_with|

.. _function-_at_strings_c__c_starts_with_CIs_Ci_CIs_C_c:

.. das:function:: starts_with(str: string const implicit; offset: int const; cmp: string const implicit)

starts_with returns bool

+--------+---------------------+
+argument+argument type        +
+========+=====================+
+str     +string const implicit+
+--------+---------------------+
+offset  +int const            +
+--------+---------------------+
+cmp     +string const implicit+
+--------+---------------------+


|function-strings-starts_with|

.. _function-_at_strings_c__c_starts_with_CIs_Ci_CIs_Cu_C_c:

.. das:function:: starts_with(str: string const implicit; offset: int const; cmp: string const implicit; cmpLen: uint const)

starts_with returns bool

+--------+---------------------+
+argument+argument type        +
+========+=====================+
+str     +string const implicit+
+--------+---------------------+
+offset  +int const            +
+--------+---------------------+
+cmp     +string const implicit+
+--------+---------------------+
+cmpLen  +uint const           +
+--------+---------------------+


|function-strings-starts_with|

.. _function-_at_strings_c__c_length_CIs_C_c:

.. das:function:: length(str: string const implicit)

length returns int

+--------+---------------------+
+argument+argument type        +
+========+=====================+
+str     +string const implicit+
+--------+---------------------+


|function-strings-length|

.. _function-_at_strings_c__c_length_CIH_ls__builtin__c__c_das_string_gr_:

.. das:function:: length(str: das_string const implicit)

length returns int

+--------+-----------------------------------------------------------------------+
+argument+argument type                                                          +
+========+=======================================================================+
+str     + :ref:`builtin::das_string <handle-builtin-das_string>`  const implicit+
+--------+-----------------------------------------------------------------------+


|function-strings-length|

++++++++++++++
String builder
++++++++++++++

  *  :ref:`build_string (block:block\<(var arg0:strings::StringBuilderWriter):void\> const implicit;context:__context const;lineinfo:__lineInfo const) : string <function-_at_strings_c__c_build_string_CI0_ls_H_ls_strings_c__c_StringBuilderWriter_gr__gr_1_ls_v_gr__builtin__C_c_C_l>` 
  *  :ref:`write (writer:strings::StringBuilderWriter;anything:any) : strings::StringBuilderWriter& <function-_at_strings_c__c_write_H_ls_strings_c__c_StringBuilderWriter_gr__*>` 
  *  :ref:`write_char (writer:strings::StringBuilderWriter implicit;ch:int const) : strings::StringBuilderWriter& <function-_at_strings_c__c_write_char_IH_ls_strings_c__c_StringBuilderWriter_gr__Ci>` 
  *  :ref:`write_chars (writer:strings::StringBuilderWriter implicit;ch:int const;count:int const) : strings::StringBuilderWriter& <function-_at_strings_c__c_write_chars_IH_ls_strings_c__c_StringBuilderWriter_gr__Ci_Ci>` 
  *  :ref:`write_escape_string (writer:strings::StringBuilderWriter implicit;str:string const implicit) : strings::StringBuilderWriter& <function-_at_strings_c__c_write_escape_string_IH_ls_strings_c__c_StringBuilderWriter_gr__CIs>` 
  *  :ref:`format (writer:strings::StringBuilderWriter implicit;format:string const implicit;value:int const) : strings::StringBuilderWriter& <function-_at_strings_c__c_format_IH_ls_strings_c__c_StringBuilderWriter_gr__CIs_Ci>` 
  *  :ref:`format (writer:strings::StringBuilderWriter implicit;format:string const implicit;value:uint const) : strings::StringBuilderWriter& <function-_at_strings_c__c_format_IH_ls_strings_c__c_StringBuilderWriter_gr__CIs_Cu>` 
  *  :ref:`format (writer:strings::StringBuilderWriter implicit;format:string const implicit;value:int64 const) : strings::StringBuilderWriter& <function-_at_strings_c__c_format_IH_ls_strings_c__c_StringBuilderWriter_gr__CIs_Ci64>` 
  *  :ref:`format (writer:strings::StringBuilderWriter implicit;format:string const implicit;value:uint64 const) : strings::StringBuilderWriter& <function-_at_strings_c__c_format_IH_ls_strings_c__c_StringBuilderWriter_gr__CIs_Cu64>` 
  *  :ref:`format (writer:strings::StringBuilderWriter implicit;format:string const implicit;value:float const) : strings::StringBuilderWriter& <function-_at_strings_c__c_format_IH_ls_strings_c__c_StringBuilderWriter_gr__CIs_Cf>` 
  *  :ref:`format (writer:strings::StringBuilderWriter implicit;format:string const implicit;value:double const) : strings::StringBuilderWriter& <function-_at_strings_c__c_format_IH_ls_strings_c__c_StringBuilderWriter_gr__CIs_Cd>` 
  *  :ref:`format (format:string const implicit;value:int const;context:__context const) : string <function-_at_strings_c__c_format_CIs_Ci_C_c>` 
  *  :ref:`format (format:string const implicit;value:uint const;context:__context const) : string <function-_at_strings_c__c_format_CIs_Cu_C_c>` 
  *  :ref:`format (format:string const implicit;value:int64 const;context:__context const) : string <function-_at_strings_c__c_format_CIs_Ci64_C_c>` 
  *  :ref:`format (format:string const implicit;value:uint64 const;context:__context const) : string <function-_at_strings_c__c_format_CIs_Cu64_C_c>` 
  *  :ref:`format (format:string const implicit;value:float const;context:__context const) : string <function-_at_strings_c__c_format_CIs_Cf_C_c>` 
  *  :ref:`format (format:string const implicit;value:double const;context:__context const) : string <function-_at_strings_c__c_format_CIs_Cd_C_c>` 

.. _function-_at_strings_c__c_build_string_CI0_ls_H_ls_strings_c__c_StringBuilderWriter_gr__gr_1_ls_v_gr__builtin__C_c_C_l:

.. das:function:: build_string(block: block<(var arg0:strings::StringBuilderWriter):void> const implicit)

build_string returns string

+--------+-------------------------------------------------------------------------------------------------------+
+argument+argument type                                                                                          +
+========+=======================================================================================================+
+block   +block<( :ref:`strings::StringBuilderWriter <handle-strings-StringBuilderWriter>` ):void> const implicit+
+--------+-------------------------------------------------------------------------------------------------------+


|function-strings-build_string|

.. _function-_at_strings_c__c_write_H_ls_strings_c__c_StringBuilderWriter_gr__*:

.. das:function:: write(writer: StringBuilderWriter; anything: any)

write returns  :ref:`strings::StringBuilderWriter <handle-strings-StringBuilderWriter>` &

+--------+--------------------------------------------------------------------------+
+argument+argument type                                                             +
+========+==========================================================================+
+writer  + :ref:`strings::StringBuilderWriter <handle-strings-StringBuilderWriter>` +
+--------+--------------------------------------------------------------------------+
+anything+any                                                                       +
+--------+--------------------------------------------------------------------------+


|function-strings-write|

.. _function-_at_strings_c__c_write_char_IH_ls_strings_c__c_StringBuilderWriter_gr__Ci:

.. das:function:: write_char(writer: StringBuilderWriter implicit; ch: int const)

write_char returns  :ref:`strings::StringBuilderWriter <handle-strings-StringBuilderWriter>` &

+--------+-----------------------------------------------------------------------------------+
+argument+argument type                                                                      +
+========+===================================================================================+
+writer  + :ref:`strings::StringBuilderWriter <handle-strings-StringBuilderWriter>`  implicit+
+--------+-----------------------------------------------------------------------------------+
+ch      +int const                                                                          +
+--------+-----------------------------------------------------------------------------------+


|function-strings-write_char|

.. _function-_at_strings_c__c_write_chars_IH_ls_strings_c__c_StringBuilderWriter_gr__Ci_Ci:

.. das:function:: write_chars(writer: StringBuilderWriter implicit; ch: int const; count: int const)

write_chars returns  :ref:`strings::StringBuilderWriter <handle-strings-StringBuilderWriter>` &

+--------+-----------------------------------------------------------------------------------+
+argument+argument type                                                                      +
+========+===================================================================================+
+writer  + :ref:`strings::StringBuilderWriter <handle-strings-StringBuilderWriter>`  implicit+
+--------+-----------------------------------------------------------------------------------+
+ch      +int const                                                                          +
+--------+-----------------------------------------------------------------------------------+
+count   +int const                                                                          +
+--------+-----------------------------------------------------------------------------------+


|function-strings-write_chars|

.. _function-_at_strings_c__c_write_escape_string_IH_ls_strings_c__c_StringBuilderWriter_gr__CIs:

.. das:function:: write_escape_string(writer: StringBuilderWriter implicit; str: string const implicit)

write_escape_string returns  :ref:`strings::StringBuilderWriter <handle-strings-StringBuilderWriter>` &

+--------+-----------------------------------------------------------------------------------+
+argument+argument type                                                                      +
+========+===================================================================================+
+writer  + :ref:`strings::StringBuilderWriter <handle-strings-StringBuilderWriter>`  implicit+
+--------+-----------------------------------------------------------------------------------+
+str     +string const implicit                                                              +
+--------+-----------------------------------------------------------------------------------+


|function-strings-write_escape_string|

.. _function-_at_strings_c__c_format_IH_ls_strings_c__c_StringBuilderWriter_gr__CIs_Ci:

.. das:function:: format(writer: StringBuilderWriter implicit; format: string const implicit; value: int const)

format returns  :ref:`strings::StringBuilderWriter <handle-strings-StringBuilderWriter>` &

+--------+-----------------------------------------------------------------------------------+
+argument+argument type                                                                      +
+========+===================================================================================+
+writer  + :ref:`strings::StringBuilderWriter <handle-strings-StringBuilderWriter>`  implicit+
+--------+-----------------------------------------------------------------------------------+
+format  +string const implicit                                                              +
+--------+-----------------------------------------------------------------------------------+
+value   +int const                                                                          +
+--------+-----------------------------------------------------------------------------------+


|function-strings-format|

.. _function-_at_strings_c__c_format_IH_ls_strings_c__c_StringBuilderWriter_gr__CIs_Cu:

.. das:function:: format(writer: StringBuilderWriter implicit; format: string const implicit; value: uint const)

format returns  :ref:`strings::StringBuilderWriter <handle-strings-StringBuilderWriter>` &

+--------+-----------------------------------------------------------------------------------+
+argument+argument type                                                                      +
+========+===================================================================================+
+writer  + :ref:`strings::StringBuilderWriter <handle-strings-StringBuilderWriter>`  implicit+
+--------+-----------------------------------------------------------------------------------+
+format  +string const implicit                                                              +
+--------+-----------------------------------------------------------------------------------+
+value   +uint const                                                                         +
+--------+-----------------------------------------------------------------------------------+


|function-strings-format|

.. _function-_at_strings_c__c_format_IH_ls_strings_c__c_StringBuilderWriter_gr__CIs_Ci64:

.. das:function:: format(writer: StringBuilderWriter implicit; format: string const implicit; value: int64 const)

format returns  :ref:`strings::StringBuilderWriter <handle-strings-StringBuilderWriter>` &

+--------+-----------------------------------------------------------------------------------+
+argument+argument type                                                                      +
+========+===================================================================================+
+writer  + :ref:`strings::StringBuilderWriter <handle-strings-StringBuilderWriter>`  implicit+
+--------+-----------------------------------------------------------------------------------+
+format  +string const implicit                                                              +
+--------+-----------------------------------------------------------------------------------+
+value   +int64 const                                                                        +
+--------+-----------------------------------------------------------------------------------+


|function-strings-format|

.. _function-_at_strings_c__c_format_IH_ls_strings_c__c_StringBuilderWriter_gr__CIs_Cu64:

.. das:function:: format(writer: StringBuilderWriter implicit; format: string const implicit; value: uint64 const)

format returns  :ref:`strings::StringBuilderWriter <handle-strings-StringBuilderWriter>` &

+--------+-----------------------------------------------------------------------------------+
+argument+argument type                                                                      +
+========+===================================================================================+
+writer  + :ref:`strings::StringBuilderWriter <handle-strings-StringBuilderWriter>`  implicit+
+--------+-----------------------------------------------------------------------------------+
+format  +string const implicit                                                              +
+--------+-----------------------------------------------------------------------------------+
+value   +uint64 const                                                                       +
+--------+-----------------------------------------------------------------------------------+


|function-strings-format|

.. _function-_at_strings_c__c_format_IH_ls_strings_c__c_StringBuilderWriter_gr__CIs_Cf:

.. das:function:: format(writer: StringBuilderWriter implicit; format: string const implicit; value: float const)

format returns  :ref:`strings::StringBuilderWriter <handle-strings-StringBuilderWriter>` &

+--------+-----------------------------------------------------------------------------------+
+argument+argument type                                                                      +
+========+===================================================================================+
+writer  + :ref:`strings::StringBuilderWriter <handle-strings-StringBuilderWriter>`  implicit+
+--------+-----------------------------------------------------------------------------------+
+format  +string const implicit                                                              +
+--------+-----------------------------------------------------------------------------------+
+value   +float const                                                                        +
+--------+-----------------------------------------------------------------------------------+


|function-strings-format|

.. _function-_at_strings_c__c_format_IH_ls_strings_c__c_StringBuilderWriter_gr__CIs_Cd:

.. das:function:: format(writer: StringBuilderWriter implicit; format: string const implicit; value: double const)

format returns  :ref:`strings::StringBuilderWriter <handle-strings-StringBuilderWriter>` &

+--------+-----------------------------------------------------------------------------------+
+argument+argument type                                                                      +
+========+===================================================================================+
+writer  + :ref:`strings::StringBuilderWriter <handle-strings-StringBuilderWriter>`  implicit+
+--------+-----------------------------------------------------------------------------------+
+format  +string const implicit                                                              +
+--------+-----------------------------------------------------------------------------------+
+value   +double const                                                                       +
+--------+-----------------------------------------------------------------------------------+


|function-strings-format|

.. _function-_at_strings_c__c_format_CIs_Ci_C_c:

.. das:function:: format(format: string const implicit; value: int const)

format returns string

+--------+---------------------+
+argument+argument type        +
+========+=====================+
+format  +string const implicit+
+--------+---------------------+
+value   +int const            +
+--------+---------------------+


|function-strings-format|

.. _function-_at_strings_c__c_format_CIs_Cu_C_c:

.. das:function:: format(format: string const implicit; value: uint const)

format returns string

+--------+---------------------+
+argument+argument type        +
+========+=====================+
+format  +string const implicit+
+--------+---------------------+
+value   +uint const           +
+--------+---------------------+


|function-strings-format|

.. _function-_at_strings_c__c_format_CIs_Ci64_C_c:

.. das:function:: format(format: string const implicit; value: int64 const)

format returns string

+--------+---------------------+
+argument+argument type        +
+========+=====================+
+format  +string const implicit+
+--------+---------------------+
+value   +int64 const          +
+--------+---------------------+


|function-strings-format|

.. _function-_at_strings_c__c_format_CIs_Cu64_C_c:

.. das:function:: format(format: string const implicit; value: uint64 const)

format returns string

+--------+---------------------+
+argument+argument type        +
+========+=====================+
+format  +string const implicit+
+--------+---------------------+
+value   +uint64 const         +
+--------+---------------------+


|function-strings-format|

.. _function-_at_strings_c__c_format_CIs_Cf_C_c:

.. das:function:: format(format: string const implicit; value: float const)

format returns string

+--------+---------------------+
+argument+argument type        +
+========+=====================+
+format  +string const implicit+
+--------+---------------------+
+value   +float const          +
+--------+---------------------+


|function-strings-format|

.. _function-_at_strings_c__c_format_CIs_Cd_C_c:

.. das:function:: format(format: string const implicit; value: double const)

format returns string

+--------+---------------------+
+argument+argument type        +
+========+=====================+
+format  +string const implicit+
+--------+---------------------+
+value   +double const         +
+--------+---------------------+


|function-strings-format|

++++++++++++++++++++++++
das::string manipulation
++++++++++++++++++++++++

  *  :ref:`append (str:$::das_string implicit;ch:int const) : void <function-_at_strings_c__c_append_IH_ls__builtin__c__c_das_string_gr__Ci>` 
  *  :ref:`resize (str:$::das_string implicit;new_length:int const) : void <function-_at_strings_c__c_resize_IH_ls__builtin__c__c_das_string_gr__Ci>` 

.. _function-_at_strings_c__c_append_IH_ls__builtin__c__c_das_string_gr__Ci:

.. das:function:: append(str: das_string implicit; ch: int const)

+--------+-----------------------------------------------------------------+
+argument+argument type                                                    +
+========+=================================================================+
+str     + :ref:`builtin::das_string <handle-builtin-das_string>`  implicit+
+--------+-----------------------------------------------------------------+
+ch      +int const                                                        +
+--------+-----------------------------------------------------------------+


|function-strings-append|

.. _function-_at_strings_c__c_resize_IH_ls__builtin__c__c_das_string_gr__Ci:

.. das:function:: resize(str: das_string implicit; new_length: int const)

+----------+-----------------------------------------------------------------+
+argument  +argument type                                                    +
+==========+=================================================================+
+str       + :ref:`builtin::das_string <handle-builtin-das_string>`  implicit+
+----------+-----------------------------------------------------------------+
+new_length+int const                                                        +
+----------+-----------------------------------------------------------------+


|function-strings-resize|

++++++++++++++++++++
String modifications
++++++++++++++++++++

  *  :ref:`repeat (str:string const implicit;count:int const;context:__context const) : string <function-_at_strings_c__c_repeat_CIs_Ci_C_c>` 
  *  :ref:`strip (str:string const implicit;context:__context const) : string <function-_at_strings_c__c_strip_CIs_C_c>` 
  *  :ref:`strip_right (str:string const implicit;context:__context const) : string <function-_at_strings_c__c_strip_right_CIs_C_c>` 
  *  :ref:`strip_left (str:string const implicit;context:__context const) : string <function-_at_strings_c__c_strip_left_CIs_C_c>` 
  *  :ref:`chop (str:string const implicit;start:int const;length:int const;context:__context const) : string <function-_at_strings_c__c_chop_CIs_Ci_Ci_C_c>` 
  *  :ref:`slice (str:string const implicit;start:int const;end:int const;context:__context const) : string <function-_at_strings_c__c_slice_CIs_Ci_Ci_C_c>` 
  *  :ref:`slice (str:string const implicit;start:int const;context:__context const) : string <function-_at_strings_c__c_slice_CIs_Ci_C_c>` 
  *  :ref:`reverse (str:string const implicit;context:__context const) : string <function-_at_strings_c__c_reverse_CIs_C_c>` 
  *  :ref:`to_upper (str:string const implicit;context:__context const) : string <function-_at_strings_c__c_to_upper_CIs_C_c>` 
  *  :ref:`to_lower (str:string const implicit;context:__context const) : string <function-_at_strings_c__c_to_lower_CIs_C_c>` 
  *  :ref:`to_lower_in_place (str:string const implicit) : string <function-_at_strings_c__c_to_lower_in_place_CIs>` 
  *  :ref:`to_upper_in_place (str:string const implicit) : string <function-_at_strings_c__c_to_upper_in_place_CIs>` 
  *  :ref:`escape (str:string const implicit;context:__context const) : string <function-_at_strings_c__c_escape_CIs_C_c>` 
  *  :ref:`unescape (str:string const implicit;context:__context const;at:__lineInfo const) : string <function-_at_strings_c__c_unescape_CIs_C_c_C_l>` 
  *  :ref:`safe_unescape (str:string const implicit;context:__context const) : string <function-_at_strings_c__c_safe_unescape_CIs_C_c>` 
  *  :ref:`replace (str:string const implicit;toSearch:string const implicit;replace:string const implicit;context:__context const) : string <function-_at_strings_c__c_replace_CIs_CIs_CIs_C_c>` 
  *  :ref:`rtrim (str:string const implicit;context:__context const) : string <function-_at_strings_c__c_rtrim_CIs_C_c>` 
  *  :ref:`rtrim (str:string const implicit;chars:string const implicit;context:__context const) : string <function-_at_strings_c__c_rtrim_CIs_CIs_C_c>` 

.. _function-_at_strings_c__c_repeat_CIs_Ci_C_c:

.. das:function:: repeat(str: string const implicit; count: int const)

repeat returns string

+--------+---------------------+
+argument+argument type        +
+========+=====================+
+str     +string const implicit+
+--------+---------------------+
+count   +int const            +
+--------+---------------------+


|function-strings-repeat|

.. _function-_at_strings_c__c_strip_CIs_C_c:

.. das:function:: strip(str: string const implicit)

strip returns string

+--------+---------------------+
+argument+argument type        +
+========+=====================+
+str     +string const implicit+
+--------+---------------------+


|function-strings-strip|

.. _function-_at_strings_c__c_strip_right_CIs_C_c:

.. das:function:: strip_right(str: string const implicit)

strip_right returns string

+--------+---------------------+
+argument+argument type        +
+========+=====================+
+str     +string const implicit+
+--------+---------------------+


|function-strings-strip_right|

.. _function-_at_strings_c__c_strip_left_CIs_C_c:

.. das:function:: strip_left(str: string const implicit)

strip_left returns string

+--------+---------------------+
+argument+argument type        +
+========+=====================+
+str     +string const implicit+
+--------+---------------------+


|function-strings-strip_left|

.. _function-_at_strings_c__c_chop_CIs_Ci_Ci_C_c:

.. das:function:: chop(str: string const implicit; start: int const; length: int const)

chop returns string

+--------+---------------------+
+argument+argument type        +
+========+=====================+
+str     +string const implicit+
+--------+---------------------+
+start   +int const            +
+--------+---------------------+
+length  +int const            +
+--------+---------------------+


|function-strings-chop|

.. _function-_at_strings_c__c_slice_CIs_Ci_Ci_C_c:

.. das:function:: slice(str: string const implicit; start: int const; end: int const)

slice returns string

+--------+---------------------+
+argument+argument type        +
+========+=====================+
+str     +string const implicit+
+--------+---------------------+
+start   +int const            +
+--------+---------------------+
+end     +int const            +
+--------+---------------------+


|function-strings-slice|

.. _function-_at_strings_c__c_slice_CIs_Ci_C_c:

.. das:function:: slice(str: string const implicit; start: int const)

slice returns string

+--------+---------------------+
+argument+argument type        +
+========+=====================+
+str     +string const implicit+
+--------+---------------------+
+start   +int const            +
+--------+---------------------+


|function-strings-slice|

.. _function-_at_strings_c__c_reverse_CIs_C_c:

.. das:function:: reverse(str: string const implicit)

reverse returns string

+--------+---------------------+
+argument+argument type        +
+========+=====================+
+str     +string const implicit+
+--------+---------------------+


|function-strings-reverse|

.. _function-_at_strings_c__c_to_upper_CIs_C_c:

.. das:function:: to_upper(str: string const implicit)

to_upper returns string

+--------+---------------------+
+argument+argument type        +
+========+=====================+
+str     +string const implicit+
+--------+---------------------+


|function-strings-to_upper|

.. _function-_at_strings_c__c_to_lower_CIs_C_c:

.. das:function:: to_lower(str: string const implicit)

to_lower returns string

+--------+---------------------+
+argument+argument type        +
+========+=====================+
+str     +string const implicit+
+--------+---------------------+


|function-strings-to_lower|

.. _function-_at_strings_c__c_to_lower_in_place_CIs:

.. das:function:: to_lower_in_place(str: string const implicit)

to_lower_in_place returns string

.. warning:: 
  This is unsafe operation.

+--------+---------------------+
+argument+argument type        +
+========+=====================+
+str     +string const implicit+
+--------+---------------------+


|function-strings-to_lower_in_place|

.. _function-_at_strings_c__c_to_upper_in_place_CIs:

.. das:function:: to_upper_in_place(str: string const implicit)

to_upper_in_place returns string

.. warning:: 
  This is unsafe operation.

+--------+---------------------+
+argument+argument type        +
+========+=====================+
+str     +string const implicit+
+--------+---------------------+


|function-strings-to_upper_in_place|

.. _function-_at_strings_c__c_escape_CIs_C_c:

.. das:function:: escape(str: string const implicit)

escape returns string

+--------+---------------------+
+argument+argument type        +
+========+=====================+
+str     +string const implicit+
+--------+---------------------+


|function-strings-escape|

.. _function-_at_strings_c__c_unescape_CIs_C_c_C_l:

.. das:function:: unescape(str: string const implicit)

unescape returns string

+--------+---------------------+
+argument+argument type        +
+========+=====================+
+str     +string const implicit+
+--------+---------------------+


|function-strings-unescape|

.. _function-_at_strings_c__c_safe_unescape_CIs_C_c:

.. das:function:: safe_unescape(str: string const implicit)

safe_unescape returns string

+--------+---------------------+
+argument+argument type        +
+========+=====================+
+str     +string const implicit+
+--------+---------------------+


|function-strings-safe_unescape|

.. _function-_at_strings_c__c_replace_CIs_CIs_CIs_C_c:

.. das:function:: replace(str: string const implicit; toSearch: string const implicit; replace: string const implicit)

replace returns string

+--------+---------------------+
+argument+argument type        +
+========+=====================+
+str     +string const implicit+
+--------+---------------------+
+toSearch+string const implicit+
+--------+---------------------+
+replace +string const implicit+
+--------+---------------------+


|function-strings-replace|

.. _function-_at_strings_c__c_rtrim_CIs_C_c:

.. das:function:: rtrim(str: string const implicit)

rtrim returns string

+--------+---------------------+
+argument+argument type        +
+========+=====================+
+str     +string const implicit+
+--------+---------------------+


|function-strings-rtrim|

.. _function-_at_strings_c__c_rtrim_CIs_CIs_C_c:

.. das:function:: rtrim(str: string const implicit; chars: string const implicit)

rtrim returns string

+--------+---------------------+
+argument+argument type        +
+========+=====================+
+str     +string const implicit+
+--------+---------------------+
+chars   +string const implicit+
+--------+---------------------+


|function-strings-rtrim|

+++++++++++++++++
Search substrings
+++++++++++++++++

  *  :ref:`find (str:string const implicit;substr:string const implicit;start:int const;context:__context const) : int <function-_at_strings_c__c_find_CIs_CIs_Ci_C_c>` 
  *  :ref:`find (str:string const implicit;substr:string const implicit) : int <function-_at_strings_c__c_find_CIs_CIs>` 
  *  :ref:`find (str:string const implicit;substr:int const;context:__context const) : int <function-_at_strings_c__c_find_CIs_Ci_C_c>` 
  *  :ref:`find (str:string const implicit;substr:int const;start:int const;context:__context const) : int <function-_at_strings_c__c_find_CIs_Ci_Ci_C_c>` 

.. _function-_at_strings_c__c_find_CIs_CIs_Ci_C_c:

.. das:function:: find(str: string const implicit; substr: string const implicit; start: int const)

find returns int

+--------+---------------------+
+argument+argument type        +
+========+=====================+
+str     +string const implicit+
+--------+---------------------+
+substr  +string const implicit+
+--------+---------------------+
+start   +int const            +
+--------+---------------------+


|function-strings-find|

.. _function-_at_strings_c__c_find_CIs_CIs:

.. das:function:: find(str: string const implicit; substr: string const implicit)

find returns int

+--------+---------------------+
+argument+argument type        +
+========+=====================+
+str     +string const implicit+
+--------+---------------------+
+substr  +string const implicit+
+--------+---------------------+


|function-strings-find|

.. _function-_at_strings_c__c_find_CIs_Ci_C_c:

.. das:function:: find(str: string const implicit; substr: int const)

find returns int

+--------+---------------------+
+argument+argument type        +
+========+=====================+
+str     +string const implicit+
+--------+---------------------+
+substr  +int const            +
+--------+---------------------+


|function-strings-find|

.. _function-_at_strings_c__c_find_CIs_Ci_Ci_C_c:

.. das:function:: find(str: string const implicit; substr: int const; start: int const)

find returns int

+--------+---------------------+
+argument+argument type        +
+========+=====================+
+str     +string const implicit+
+--------+---------------------+
+substr  +int const            +
+--------+---------------------+
+start   +int const            +
+--------+---------------------+


|function-strings-find|

++++++++++++++++++++++++++
String conversion routines
++++++++++++++++++++++++++

  *  :ref:`string (bytes:array\<uint8\> const implicit;context:__context const) : string <function-_at_strings_c__c_string_CI1_ls_u8_gr_A_C_c>` 
  *  :ref:`to_char (char:int const;context:__context const) : string <function-_at_strings_c__c_to_char_Ci_C_c>` 
  *  :ref:`int (str:string const implicit;context:__context const;at:__lineInfo const) : int <function-_at_strings_c__c_int_CIs_C_c_C_l>` 
  *  :ref:`uint (str:string const implicit;context:__context const;at:__lineInfo const) : uint <function-_at_strings_c__c_uint_CIs_C_c_C_l>` 
  *  :ref:`int64 (str:string const implicit;context:__context const;at:__lineInfo const) : int64 <function-_at_strings_c__c_int64_CIs_C_c_C_l>` 
  *  :ref:`uint64 (str:string const implicit;context:__context const;at:__lineInfo const) : uint64 <function-_at_strings_c__c_uint64_CIs_C_c_C_l>` 
  *  :ref:`float (str:string const implicit;context:__context const;at:__lineInfo const) : float <function-_at_strings_c__c_float_CIs_C_c_C_l>` 
  *  :ref:`double (str:string const implicit;context:__context const;at:__lineInfo const) : double <function-_at_strings_c__c_double_CIs_C_c_C_l>` 
  *  :ref:`to_int (value:string const implicit;hex:bool const) : int <function-_at_strings_c__c_to_int_CIs_Cb>` 
  *  :ref:`to_uint (value:string const implicit;hex:bool const) : uint <function-_at_strings_c__c_to_uint_CIs_Cb>` 
  *  :ref:`to_int64 (value:string const implicit;hex:bool const) : int64 <function-_at_strings_c__c_to_int64_CIs_Cb>` 
  *  :ref:`to_uint64 (value:string const implicit;hex:bool const) : uint64 <function-_at_strings_c__c_to_uint64_CIs_Cb>` 
  *  :ref:`to_float (value:string const implicit) : float <function-_at_strings_c__c_to_float_CIs>` 
  *  :ref:`to_double (value:string const implicit) : double <function-_at_strings_c__c_to_double_CIs>` 

.. _function-_at_strings_c__c_string_CI1_ls_u8_gr_A_C_c:

.. das:function:: string(bytes: array<uint8> const implicit)

string returns string

+--------+---------------------------+
+argument+argument type              +
+========+===========================+
+bytes   +array<uint8> const implicit+
+--------+---------------------------+


|function-strings-string|

.. _function-_at_strings_c__c_to_char_Ci_C_c:

.. das:function:: to_char(char: int const)

to_char returns string

+--------+-------------+
+argument+argument type+
+========+=============+
+char    +int const    +
+--------+-------------+


|function-strings-to_char|

.. _function-_at_strings_c__c_int_CIs_C_c_C_l:

.. das:function:: int(str: string const implicit)

int returns int

+--------+---------------------+
+argument+argument type        +
+========+=====================+
+str     +string const implicit+
+--------+---------------------+


|function-strings-int|

.. _function-_at_strings_c__c_uint_CIs_C_c_C_l:

.. das:function:: uint(str: string const implicit)

uint returns uint

+--------+---------------------+
+argument+argument type        +
+========+=====================+
+str     +string const implicit+
+--------+---------------------+


|function-strings-uint|

.. _function-_at_strings_c__c_int64_CIs_C_c_C_l:

.. das:function:: int64(str: string const implicit)

int64 returns int64

+--------+---------------------+
+argument+argument type        +
+========+=====================+
+str     +string const implicit+
+--------+---------------------+


|function-strings-int64|

.. _function-_at_strings_c__c_uint64_CIs_C_c_C_l:

.. das:function:: uint64(str: string const implicit)

uint64 returns uint64

+--------+---------------------+
+argument+argument type        +
+========+=====================+
+str     +string const implicit+
+--------+---------------------+


|function-strings-uint64|

.. _function-_at_strings_c__c_float_CIs_C_c_C_l:

.. das:function:: float(str: string const implicit)

float returns float

+--------+---------------------+
+argument+argument type        +
+========+=====================+
+str     +string const implicit+
+--------+---------------------+


|function-strings-float|

.. _function-_at_strings_c__c_double_CIs_C_c_C_l:

.. das:function:: double(str: string const implicit)

double returns double

+--------+---------------------+
+argument+argument type        +
+========+=====================+
+str     +string const implicit+
+--------+---------------------+


|function-strings-double|

.. _function-_at_strings_c__c_to_int_CIs_Cb:

.. das:function:: to_int(value: string const implicit; hex: bool const)

to_int returns int

+--------+---------------------+
+argument+argument type        +
+========+=====================+
+value   +string const implicit+
+--------+---------------------+
+hex     +bool const           +
+--------+---------------------+


|function-strings-to_int|

.. _function-_at_strings_c__c_to_uint_CIs_Cb:

.. das:function:: to_uint(value: string const implicit; hex: bool const)

to_uint returns uint

+--------+---------------------+
+argument+argument type        +
+========+=====================+
+value   +string const implicit+
+--------+---------------------+
+hex     +bool const           +
+--------+---------------------+


|function-strings-to_uint|

.. _function-_at_strings_c__c_to_int64_CIs_Cb:

.. das:function:: to_int64(value: string const implicit; hex: bool const)

to_int64 returns int64

+--------+---------------------+
+argument+argument type        +
+========+=====================+
+value   +string const implicit+
+--------+---------------------+
+hex     +bool const           +
+--------+---------------------+


|function-strings-to_int64|

.. _function-_at_strings_c__c_to_uint64_CIs_Cb:

.. das:function:: to_uint64(value: string const implicit; hex: bool const)

to_uint64 returns uint64

+--------+---------------------+
+argument+argument type        +
+========+=====================+
+value   +string const implicit+
+--------+---------------------+
+hex     +bool const           +
+--------+---------------------+


|function-strings-to_uint64|

.. _function-_at_strings_c__c_to_float_CIs:

.. das:function:: to_float(value: string const implicit)

to_float returns float

+--------+---------------------+
+argument+argument type        +
+========+=====================+
+value   +string const implicit+
+--------+---------------------+


|function-strings-to_float|

.. _function-_at_strings_c__c_to_double_CIs:

.. das:function:: to_double(value: string const implicit)

to_double returns double

+--------+---------------------+
+argument+argument type        +
+========+=====================+
+value   +string const implicit+
+--------+---------------------+


|function-strings-to_double|

+++++++++++++++
String as array
+++++++++++++++

  *  :ref:`peek_data (str:string const implicit;block:block\<(arg0:array\<uint8\> const#):void\> const implicit;context:__context const;lineinfo:__lineInfo const) : void <function-_at_strings_c__c_peek_data_CIs_CI0_ls_C_hh_1_ls_u8_gr_A_gr_1_ls_v_gr__builtin__C_c_C_l>` 
  *  :ref:`modify_data (str:string const implicit;block:block\<(var arg0:array\<uint8\>#):void\> const implicit;context:__context const;lineinfo:__lineInfo const) : string <function-_at_strings_c__c_modify_data_CIs_CI0_ls__hh_1_ls_u8_gr_A_gr_1_ls_v_gr__builtin__C_c_C_l>` 

.. _function-_at_strings_c__c_peek_data_CIs_CI0_ls_C_hh_1_ls_u8_gr_A_gr_1_ls_v_gr__builtin__C_c_C_l:

.. das:function:: peek_data(str: string const implicit; block: block<(arg0:array<uint8> const#):void> const implicit)

+--------+------------------------------------------------+
+argument+argument type                                   +
+========+================================================+
+str     +string const implicit                           +
+--------+------------------------------------------------+
+block   +block<(array<uint8> const#):void> const implicit+
+--------+------------------------------------------------+


|function-strings-peek_data|

.. _function-_at_strings_c__c_modify_data_CIs_CI0_ls__hh_1_ls_u8_gr_A_gr_1_ls_v_gr__builtin__C_c_C_l:

.. das:function:: modify_data(str: string const implicit; block: block<(var arg0:array<uint8>#):void> const implicit)

modify_data returns string

+--------+------------------------------------------+
+argument+argument type                             +
+========+==========================================+
+str     +string const implicit                     +
+--------+------------------------------------------+
+block   +block<(array<uint8>#):void> const implicit+
+--------+------------------------------------------+


|function-strings-modify_data|

+++++++++++++++++++++++++++
Low level memory allocation
+++++++++++++++++++++++++++

  *  :ref:`delete_string (str:string& implicit;context:__context const) : void <function-_at_strings_c__c_delete_string_&Is_C_c>` 
  *  :ref:`reserve_string_buffer (str:string const implicit;length:int const;context:__context const) : string <function-_at_strings_c__c_reserve_string_buffer_CIs_Ci_C_c>` 

.. _function-_at_strings_c__c_delete_string_&Is_C_c:

.. das:function:: delete_string(str: string& implicit)

.. warning:: 
  This is unsafe operation.

+--------+----------------+
+argument+argument type   +
+========+================+
+str     +string& implicit+
+--------+----------------+


|function-strings-delete_string|

.. _function-_at_strings_c__c_reserve_string_buffer_CIs_Ci_C_c:

.. das:function:: reserve_string_buffer(str: string const implicit; length: int const)

reserve_string_buffer returns string

+--------+---------------------+
+argument+argument type        +
+========+=====================+
+str     +string const implicit+
+--------+---------------------+
+length  +int const            +
+--------+---------------------+


|function-strings-reserve_string_buffer|


