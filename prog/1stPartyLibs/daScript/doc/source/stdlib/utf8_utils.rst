
.. _stdlib_utf8_utils:

===============
UTF-8 utilities
===============

.. include:: detail/utf8_utils.rst

|module-utf8_utils|

+++++++++
Constants
+++++++++

.. _global-utf8_utils-UTF8_ACCEPT:

.. das:attribute:: UTF8_ACCEPT = 0x0

|variable-utf8_utils-UTF8_ACCEPT|

+++++++++++++
Uncategorized
+++++++++++++

.. _function-_at_utf8_utils_c__c_utf16_to_utf32_Cu_Cu:

.. das:function:: utf16_to_utf32(high: uint const; low: uint const)

utf16_to_utf32 returns uint

+--------+-------------+
+argument+argument type+
+========+=============+
+high    +uint const   +
+--------+-------------+
+low     +uint const   +
+--------+-------------+


|function-utf8_utils-utf16_to_utf32|

.. _function-_at_utf8_utils_c__c_utf8_encode_1_ls_u8_gr_A_Cu:

.. das:function:: utf8_encode(dest_array: array<uint8>; ch: uint const)

+----------+-------------+
+argument  +argument type+
+==========+=============+
+dest_array+array<uint8> +
+----------+-------------+
+ch        +uint const   +
+----------+-------------+


Converts UTF-32 string to UTF-8 and returns it as a UTF-8 byte array

.. _function-_at_utf8_utils_c__c_utf8_encode_Cu:

.. das:function:: utf8_encode(ch: uint const)

utf8_encode returns array<uint8>

+--------+-------------+
+argument+argument type+
+========+=============+
+ch      +uint const   +
+--------+-------------+


Converts UTF-32 string to UTF-8 and returns it as a UTF-8 byte array

.. _function-_at_utf8_utils_c__c_utf8_encode_1_ls_u8_gr_A_CI1_ls_u_gr_A:

.. das:function:: utf8_encode(dest_array: array<uint8>; source_utf32_string: array<uint> const implicit)

+-------------------+--------------------------+
+argument           +argument type             +
+===================+==========================+
+dest_array         +array<uint8>              +
+-------------------+--------------------------+
+source_utf32_string+array<uint> const implicit+
+-------------------+--------------------------+


Converts UTF-32 string to UTF-8 and returns it as a UTF-8 byte array

.. _function-_at_utf8_utils_c__c_utf8_encode_CI1_ls_u_gr_A:

.. das:function:: utf8_encode(source_utf32_string: array<uint> const implicit)

utf8_encode returns array<uint8>

+-------------------+--------------------------+
+argument           +argument type             +
+===================+==========================+
+source_utf32_string+array<uint> const implicit+
+-------------------+--------------------------+


Converts UTF-32 string to UTF-8 and returns it as a UTF-8 byte array

.. _function-_at_utf8_utils_c__c_utf8_length_CI1_ls_u8_gr_A:

.. das:function:: utf8_length(utf8_string: array<uint8> const implicit)

utf8_length returns int

+-----------+---------------------------+
+argument   +argument type              +
+===========+===========================+
+utf8_string+array<uint8> const implicit+
+-----------+---------------------------+


Returns the number of characters in the UTF-8 string

.. _function-_at_utf8_utils_c__c_utf8_length_Cs:

.. das:function:: utf8_length(utf8_string: string const)

utf8_length returns int

+-----------+-------------+
+argument   +argument type+
+===========+=============+
+utf8_string+string const +
+-----------+-------------+


Returns the number of characters in the UTF-8 string

.. _function-_at_utf8_utils_c__c_is_first_byte_of_utf8_char_Cu8:

.. das:function:: is_first_byte_of_utf8_char(ch: uint8 const)

is_first_byte_of_utf8_char returns bool

+--------+-------------+
+argument+argument type+
+========+=============+
+ch      +uint8 const  +
+--------+-------------+


|function-utf8_utils-is_first_byte_of_utf8_char|

.. _function-_at_utf8_utils_c__c_contains_utf8_bom_CI1_ls_u8_gr_A:

.. das:function:: contains_utf8_bom(utf8_string: array<uint8> const implicit)

contains_utf8_bom returns bool

+-----------+---------------------------+
+argument   +argument type              +
+===========+===========================+
+utf8_string+array<uint8> const implicit+
+-----------+---------------------------+


|function-utf8_utils-contains_utf8_bom|

.. _function-_at_utf8_utils_c__c_contains_utf8_bom_Cs:

.. das:function:: contains_utf8_bom(utf8_string: string const)

contains_utf8_bom returns bool

+-----------+-------------+
+argument   +argument type+
+===========+=============+
+utf8_string+string const +
+-----------+-------------+


|function-utf8_utils-contains_utf8_bom|

.. _function-_at_utf8_utils_c__c_is_utf8_string_valid_CI1_ls_u8_gr_A:

.. das:function:: is_utf8_string_valid(utf8_string: array<uint8> const implicit)

is_utf8_string_valid returns bool

+-----------+---------------------------+
+argument   +argument type              +
+===========+===========================+
+utf8_string+array<uint8> const implicit+
+-----------+---------------------------+


|function-utf8_utils-is_utf8_string_valid|

.. _function-_at_utf8_utils_c__c_is_utf8_string_valid_Cs:

.. das:function:: is_utf8_string_valid(utf8_string: string const)

is_utf8_string_valid returns bool

+-----------+-------------+
+argument   +argument type+
+===========+=============+
+utf8_string+string const +
+-----------+-------------+


|function-utf8_utils-is_utf8_string_valid|

.. _function-_at_utf8_utils_c__c_utf8_decode_1_ls_u_gr_A_CI1_ls_u8_gr_A:

.. das:function:: utf8_decode(dest_utf32_string: array<uint>; source_utf8_string: array<uint8> const implicit)

+------------------+---------------------------+
+argument          +argument type              +
+==================+===========================+
+dest_utf32_string +array<uint>                +
+------------------+---------------------------+
+source_utf8_string+array<uint8> const implicit+
+------------------+---------------------------+


Converts UTF-8 string to UTF-32 and appends it to the array of codepoints (UTF-32 string)

.. _function-_at_utf8_utils_c__c_utf8_decode_CI1_ls_u8_gr_A:

.. das:function:: utf8_decode(source_utf8_string: array<uint8> const implicit)

utf8_decode returns array<uint>

+------------------+---------------------------+
+argument          +argument type              +
+==================+===========================+
+source_utf8_string+array<uint8> const implicit+
+------------------+---------------------------+


Converts UTF-8 string to UTF-32 and appends it to the array of codepoints (UTF-32 string)

.. _function-_at_utf8_utils_c__c_utf8_decode_Cs:

.. das:function:: utf8_decode(source_utf8_string: string const)

utf8_decode returns array<uint>

+------------------+-------------+
+argument          +argument type+
+==================+=============+
+source_utf8_string+string const +
+------------------+-------------+


Converts UTF-8 string to UTF-32 and appends it to the array of codepoints (UTF-32 string)

.. _function-_at_utf8_utils_c__c_utf8_decode_1_ls_u_gr_A_Cs:

.. das:function:: utf8_decode(dest_utf32_string: array<uint>; source_utf8_string: string const)

+------------------+-------------+
+argument          +argument type+
+==================+=============+
+dest_utf32_string +array<uint>  +
+------------------+-------------+
+source_utf8_string+string const +
+------------------+-------------+


Converts UTF-8 string to UTF-32 and appends it to the array of codepoints (UTF-32 string)

.. _function-_at_utf8_utils_c__c_decode_unicode_escape_Cs:

.. das:function:: decode_unicode_escape(str: string const)

decode_unicode_escape returns string

+--------+-------------+
+argument+argument type+
+========+=============+
+str     +string const +
+--------+-------------+


|function-utf8_utils-decode_unicode_escape|


