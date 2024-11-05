
.. _stdlib_json:

=========================
JSON manipulation library
=========================

.. include:: detail/json.rst

The JSON module implements JSON parser and serialization routines.
See `JHSON <www.json.org>` for details.

All functions and symbols are in "json" module, use require to get access to it. ::

    require daslib/json


++++++++++++
Type aliases
++++++++++++

.. _alias-JsValue:

.. das:attribute:: JsValue is a variant type

+-------+------------------------+
+_object+table<string;JsonValue?>+
+-------+------------------------+
+_array +array<JsonValue?>       +
+-------+------------------------+
+_string+string                  +
+-------+------------------------+
+_number+double                  +
+-------+------------------------+
+_bool  +bool                    +
+-------+------------------------+
+_null  +void?                   +
+-------+------------------------+


Single JSON element.

.. _alias-Token:

.. das:attribute:: Token is a variant type

+-------+------+
+_string+string+
+-------+------+
+_number+double+
+-------+------+
+_bool  +bool  +
+-------+------+
+_null  +void? +
+-------+------+
+_symbol+int   +
+-------+------+
+_error +string+
+-------+------+


JSON input stream token.

.. _struct-json-JsonValue:

.. das:attribute:: JsonValue



JsonValue fields are

+-----+--------------------------------+
+value+ :ref:`JsValue <alias-JsValue>` +
+-----+--------------------------------+


JSON value, wraps any JSON element.

.. _struct-json-TokenAt:

.. das:attribute:: TokenAt



TokenAt fields are

+-----+----------------------------+
+value+ :ref:`Token <alias-Token>` +
+-----+----------------------------+
+line +int                         +
+-----+----------------------------+
+row  +int                         +
+-----+----------------------------+


JSON parsing token. Contains token and its position.

++++++++++++++++
Value conversion
++++++++++++++++

  *  :ref:`JV (v:string const) : json::JsonValue? <function-_at_json_c__c_JV_Cs>` 
  *  :ref:`JV (v:double const) : json::JsonValue? <function-_at_json_c__c_JV_Cd>` 
  *  :ref:`JV (v:bool const) : json::JsonValue? <function-_at_json_c__c_JV_Cb>` 
  *  :ref:`JVNull () : json::JsonValue? <function-_at_json_c__c_JVNull>` 
  *  :ref:`JV (v:table\<string;json::JsonValue?\> -const) : json::JsonValue? <function-_at_json_c__c_JV_1_ls_s_gr_2_ls_1_ls_S_ls_json_c__c_JsonValue_gr__gr__qm__gr_T>` 
  *  :ref:`JV (v:array\<json::JsonValue?\> -const) : json::JsonValue? <function-_at_json_c__c_JV_1_ls_1_ls_S_ls_json_c__c_JsonValue_gr__gr__qm__gr_A>` 

.. _function-_at_json_c__c_JV_Cs:

.. das:function:: JV(v: string const)

JV returns  :ref:`json::JsonValue <struct-json-JsonValue>` ?

+--------+-------------+
+argument+argument type+
+========+=============+
+v       +string const +
+--------+-------------+


Creates `JsonValue` out of value.

.. _function-_at_json_c__c_JV_Cd:

.. das:function:: JV(v: double const)

JV returns  :ref:`json::JsonValue <struct-json-JsonValue>` ?

+--------+-------------+
+argument+argument type+
+========+=============+
+v       +double const +
+--------+-------------+


Creates `JsonValue` out of value.

.. _function-_at_json_c__c_JV_Cb:

.. das:function:: JV(v: bool const)

JV returns  :ref:`json::JsonValue <struct-json-JsonValue>` ?

+--------+-------------+
+argument+argument type+
+========+=============+
+v       +bool const   +
+--------+-------------+


Creates `JsonValue` out of value.

.. _function-_at_json_c__c_JVNull:

.. das:function:: JVNull()

JVNull returns  :ref:`json::JsonValue <struct-json-JsonValue>` ?

Creates `JsonValue` representing `null`.

.. _function-_at_json_c__c_JV_1_ls_s_gr_2_ls_1_ls_S_ls_json_c__c_JsonValue_gr__gr__qm__gr_T:

.. das:function:: JV(v: table<string;JsonValue?>)

JV returns  :ref:`json::JsonValue <struct-json-JsonValue>` ?

+--------+---------------------------------------------------------------+
+argument+argument type                                                  +
+========+===============================================================+
+v       +table<string; :ref:`json::JsonValue <struct-json-JsonValue>` ?>+
+--------+---------------------------------------------------------------+


Creates `JsonValue` out of value.

.. _function-_at_json_c__c_JV_1_ls_1_ls_S_ls_json_c__c_JsonValue_gr__gr__qm__gr_A:

.. das:function:: JV(v: array<JsonValue?>)

JV returns  :ref:`json::JsonValue <struct-json-JsonValue>` ?

+--------+--------------------------------------------------------+
+argument+argument type                                           +
+========+========================================================+
+v       +array< :ref:`json::JsonValue <struct-json-JsonValue>` ?>+
+--------+--------------------------------------------------------+


Creates `JsonValue` out of value.

++++++++++++++
Read and write
++++++++++++++

  *  :ref:`read_json (text:string const implicit;error:string& -const) : json::JsonValue? <function-_at_json_c__c_read_json_CIs_&s>` 
  *  :ref:`read_json (text:array\<uint8\> const;error:string& -const) : json::JsonValue? <function-_at_json_c__c_read_json_C1_ls_u8_gr_A_&s>` 
  *  :ref:`write_json (val:json::JsonValue? const) : string <function-_at_json_c__c_write_json_C1_ls_S_ls_json_c__c_JsonValue_gr__gr__qm_>` 
  *  :ref:`write_json (val:json::JsonValue? const#) : string <function-_at_json_c__c_write_json_C_hh_1_ls_S_ls_json_c__c_JsonValue_gr__gr__qm_>` 

.. _function-_at_json_c__c_read_json_CIs_&s:

.. das:function:: read_json(text: string const implicit; error: string&)

read_json returns  :ref:`json::JsonValue <struct-json-JsonValue>` ?

+--------+---------------------+
+argument+argument type        +
+========+=====================+
+text    +string const implicit+
+--------+---------------------+
+error   +string&              +
+--------+---------------------+


reads JSON from the `text` string.
if `error` is not empty, it contains the parsing error message.

.. _function-_at_json_c__c_read_json_C1_ls_u8_gr_A_&s:

.. das:function:: read_json(text: array<uint8> const; error: string&)

read_json returns  :ref:`json::JsonValue <struct-json-JsonValue>` ?

+--------+------------------+
+argument+argument type     +
+========+==================+
+text    +array<uint8> const+
+--------+------------------+
+error   +string&           +
+--------+------------------+


reads JSON from the `text` string.
if `error` is not empty, it contains the parsing error message.

.. _function-_at_json_c__c_write_json_C1_ls_S_ls_json_c__c_JsonValue_gr__gr__qm_:

.. das:function:: write_json(val: JsonValue? const)

write_json returns string

+--------+-------------------------------------------------------+
+argument+argument type                                          +
+========+=======================================================+
+val     + :ref:`json::JsonValue <struct-json-JsonValue>` ? const+
+--------+-------------------------------------------------------+


Overload accepting temporary type
Fine, as json doesn't escape the function

.. _function-_at_json_c__c_write_json_C_hh_1_ls_S_ls_json_c__c_JsonValue_gr__gr__qm_:

.. das:function:: write_json(val: JsonValue? const#)

write_json returns string

+--------+--------------------------------------------------------+
+argument+argument type                                           +
+========+========================================================+
+val     + :ref:`json::JsonValue <struct-json-JsonValue>` ? const#+
+--------+--------------------------------------------------------+


Overload accepting temporary type
Fine, as json doesn't escape the function

+++++++++++++++
JSON properties
+++++++++++++++

  *  :ref:`set_no_trailing_zeros (value:bool const) : bool const <function-_at_json_c__c_set_no_trailing_zeros_Cb>` 
  *  :ref:`set_no_empty_arrays (value:bool const) : bool const <function-_at_json_c__c_set_no_empty_arrays_Cb>` 
  *  :ref:`set_allow_duplicate_keys (value:bool const) : bool const <function-_at_json_c__c_set_allow_duplicate_keys_Cb>` 

.. _function-_at_json_c__c_set_no_trailing_zeros_Cb:

.. das:function:: set_no_trailing_zeros(value: bool const)

set_no_trailing_zeros returns bool const

+--------+-------------+
+argument+argument type+
+========+=============+
+value   +bool const   +
+--------+-------------+


if `value` is true, then numbers are written without trailing zeros.

.. _function-_at_json_c__c_set_no_empty_arrays_Cb:

.. das:function:: set_no_empty_arrays(value: bool const)

set_no_empty_arrays returns bool const

+--------+-------------+
+argument+argument type+
+========+=============+
+value   +bool const   +
+--------+-------------+


if `value` is true, then empty arrays are not written at all

.. _function-_at_json_c__c_set_allow_duplicate_keys_Cb:

.. das:function:: set_allow_duplicate_keys(value: bool const)

set_allow_duplicate_keys returns bool const

+--------+-------------+
+argument+argument type+
+========+=============+
+value   +bool const   +
+--------+-------------+


if `value` is true, then duplicate keys are allowed in objects. the later key overwrites the earlier one.
note - we use StringBuilderWriter for performance reasons here

+++++++++++
Broken JSON
+++++++++++

  *  :ref:`try_fixing_broken_json (bad:string -const) : string <function-_at_json_c__c_try_fixing_broken_json_s>` 

.. _function-_at_json_c__c_try_fixing_broken_json_s:

.. das:function:: try_fixing_broken_json(bad: string)

try_fixing_broken_json returns string

+--------+-------------+
+argument+argument type+
+========+=============+
+bad     +string       +
+--------+-------------+


fixes broken json. so far supported
1. "string" + "string" string concatination
2. "text "nested text" text" nested quotes
3. extra , at the end of object or array
4. /uXXXXXX sequences in the middle of white space
write until beginning of string or end
if eof we done
write the first quote
write until end of string or end of file
if eof we done, we fix close quote and we are done
skipping whitespace
if eof we done, we fix close quote and we are done
valid JSON things after string are :, }, ], or , if its one of those, we close the string and we are done
now, if its a + - it could be a string concatenation
if it is indeed new string, we go back to string writing, and add a separator
ok, its not a string concatination, or a valid character
if it was a whitespace after the quote, we assume its missing ','
. we assume its nested quotes and we replace with \"
if eof we done, we fix nested quote and and closing quote and we are done
write the second replaced quote


