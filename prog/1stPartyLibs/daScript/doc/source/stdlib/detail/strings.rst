.. |function-strings-append| replace:: Appends single character `ch` to das::string `str`.

.. |function-strings-as_string| replace:: Convert array of uint8_t `arr` to string.

.. |function-strings-build_string| replace:: Create StringBuilderWriter and pass it to the block. Upon completion of a block, return whatever was written as string.

.. |function-strings-builtin_strdup| replace:: Creates copy of a string via "C" strdup function. This way string will persist outside of the context lifetime.

.. |function-strings-builtin_string_split| replace:: Split string by the delimiter string.

.. |function-strings-builtin_string_split_by_char| replace:: Split string by any of the delimiter characters.

.. |function-strings-character_at| replace:: Returns character of the string 'str' at index 'idx'.

.. |function-strings-character_uat| replace:: Returns character of the string 'str' at index 'idx'. This function does not check bounds of index.

.. |function-strings-chop| replace:: Return all part of the strings starting at start and ending at start + length.

.. |function-strings-delete_string| replace:: Removes string from the string heap. This is unsafe because it will free the memory and all dangling strings will be broken.

.. |function-strings-empty| replace:: Returns true if string is empty (null or "").

.. |function-strings-ends_with| replace:: returns `true` if the end of the string `str`  matches a the string `cmp` otherwise returns `false`

.. |function-strings-escape| replace:: Escape string so that escape sequences are printable, for example converting "\n" into "\\n".

.. |function-strings-safe_unescape| replace:: Unescape string i.e reverse effects of `escape`. For example "\\n" is converted to "\n".

.. |function-strings-find| replace:: Return index where substr can be found within str (starting from optional 'start' at), or -1 if not found

.. |function-strings-float| replace:: Converts string to float. In case of error panic.

.. |function-strings-double| replace:: Converts string to double. In case of error panic.

.. |function-strings-int| replace:: Converts string to integer. In case of error panic.

.. |function-strings-uint| replace:: Convert string to uint. In case of error panic.

.. |function-strings-int64| replace:: Converts string to int64. In case of error panic.

.. |function-strings-uint64| replace:: Convert string to uint64. In case of error panic.

.. |function-strings-format| replace:: Converts value to string given specified format (that of C printf).

.. |function-strings-is_alpha| replace:: Returns true if character is [A-Za-z].

.. |function-strings-is_char_in_set| replace:: Returns true if character bit is set in the set (of 256 bits in uint32[8]).

.. |function-strings-is_new_line| replace:: Returns true if character is '\n' or '\r'.

.. |function-strings-is_number| replace:: Returns true if character is [0-9].

.. |function-strings-is_white_space| replace:: Returns true if character is [ \t\n\r].

.. |function-strings-length| replace:: Return length of string

.. |function-strings-repeat| replace:: Repeat string specified number of times, and return the result.

.. |function-strings-replace| replace:: Replace all occurances of the stubstring in the string with another substring.

.. |function-strings-resize| replace:: Resize string, i.e make it specified length.

.. |function-strings-reverse| replace:: Return reversed string

.. |function-strings-slice| replace:: Return all part of the strings starting at start and ending by end. Start can be negative (-1 means "1 from the end").

.. |function-strings-starts_with| replace:: returns `true` if the beginning of the string `str` matches the string `cmp`; otherwise returns `false`

.. |function-strings-string| replace:: Return string from the byte array.

.. |function-strings-strip| replace:: Strips white-space-only characters that might appear at the beginning or end of the given string and returns the new stripped string.

.. |function-strings-strip_left| replace:: Strips white-space-only characters that might appear at the beginning of the given string and returns the new stripped string.

.. |function-strings-strip_right| replace:: Strips white-space-only characters that might appear at the end of the given string and returns the new stripped string.

.. |function-strings-to_char| replace:: Convert character to string.

.. |function-strings-to_float| replace:: Convert string to float. In case of error returns 0.0

.. |function-strings-to_double| replace:: Convert string to double. In case of error returns 0.0lf

.. |function-strings-to_int| replace:: Convert string to int. In case of error returns 0

.. |function-strings-to_uint| replace:: Convert string to uint. In case of error returns 0u

.. |function-strings-to_int64| replace:: Convert string to int64. In case of error returns 0l

.. |function-strings-to_uint64| replace:: Convert string to uint64. In case of error returns 0ul

.. |function-strings-to_lower| replace:: Return all lower case string

.. |function-strings-to_lower_in_place| replace:: Modify string in place to be all lower case

.. |function-strings-to_upper| replace:: Return all upper case string

.. |function-strings-to_upper_in_place| replace:: Modify string in place to be all upper case string

.. |function-strings-unescape| replace:: Unescape string i.e reverse effects of `escape`. For example "\\n" is converted to "\n".

.. |function-strings-write| replace:: Returns textual representation of the value.

.. |function-strings-write_char| replace:: Writes character into StringBuilderWriter.

.. |function-strings-write_chars| replace:: Writes multiple characters into StringBuilderWriter.

.. |function-strings-write_escape_string| replace:: Writes escaped string into StringBuilderWriter.

.. |structure_annotation-strings-StringBuilderWriter| replace:: Object representing a string builder. Its significantly faster to write data to the string builder and than convert it to a string, as oppose to using sequences of string concatenations.

.. |function-strings-rtrim| replace:: Removes trailing white space.

.. |function-strings-peek_data| replace:: Passes temporary array which is mapped to the string data to a block as read-only.

.. |function-strings-modify_data| replace:: Passes temporary array which is mapped to the string data to a block for both reading and writing.

.. |function-strings-reserve_string_buffer| replace:: Allocate copy of the string data on the heap.

.. |function-strings-set_total| replace:: Total number of elements in the character set.

.. |function-strings-set_element| replace:: Gen character set element by element index (not character index).






