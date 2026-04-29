.. _lexical_structure:


=================
Lexical Structure
=================

.. index:: single: lexical structure

-----------
Identifiers
-----------

.. index:: single: identifiers

Identifiers start with an alphabetic character or an underscore (``_``), followed by any number
of alphabetic characters, underscores, digits (``0``–``9``), or backticks (````````).
Daslang is a case-sensitive language, meaning that ``foo``, ``Foo``, and ``fOo`` are
three distinct identifiers.

Backticks are used in system-generated identifiers (such as mangled names) and are
generally not used in user code:

.. code-block:: das

    my_variable     // valid
    _temp           // valid
    player2         // valid

-----------
Keywords
-----------

.. index:: single: keywords

The following words are reserved as keywords and cannot be used as identifiers:

+----------------------------------------------+----------------------------------------------+----------------------------------------------+----------------------------------------------+----------------------------------------------+----------------------------------------------+
| :ref:`struct <structs>`                      | :ref:`class <classes>`                       | :ref:`let <statements>`                      | :ref:`def <functions>`                       | :ref:`while <statements>`                    | :ref:`if <statements>`                       |
+----------------------------------------------+----------------------------------------------+----------------------------------------------+----------------------------------------------+----------------------------------------------+----------------------------------------------+
| :ref:`static_if <static_if>`                 | ``else``                                     | :ref:`for <statements>`                      | ``recover``                                  | ``true``                                     | ``false``                                    |
+----------------------------------------------+----------------------------------------------+----------------------------------------------+----------------------------------------------+----------------------------------------------+----------------------------------------------+
| ``new``                                      | :ref:`typeinfo <generic_programming>`        | ``type``                                     | ``in``                                       | :ref:`is <pattern-matching>`                 | :ref:`as <pattern-matching>`                 |
+----------------------------------------------+----------------------------------------------+----------------------------------------------+----------------------------------------------+----------------------------------------------+----------------------------------------------+
| ``elif``                                     | ``static_elif``                              | :ref:`array <arrays>`                        | :ref:`return <statements>`                   | ``null``                                     | :ref:`break <statements>`                    |
+----------------------------------------------+----------------------------------------------+----------------------------------------------+----------------------------------------------+----------------------------------------------+----------------------------------------------+
| ``try``                                      | :ref:`options <options>`                     | :ref:`table <tables>`                        | ``expect``                                   | ``const``                                    | :ref:`require <modules>`                     |
+----------------------------------------------+----------------------------------------------+----------------------------------------------+----------------------------------------------+----------------------------------------------+----------------------------------------------+
| ``operator``                                 | :ref:`enum <enumerations>`                   | :ref:`finally <statements>`                  | ``delete``                                   | :ref:`deref <expressions>`                   | :ref:`aka <statements>`                      |
+----------------------------------------------+----------------------------------------------+----------------------------------------------+----------------------------------------------+----------------------------------------------+----------------------------------------------+
| :ref:`typedef <aliases>`                     | ``with``                                     | :ref:`cast <expressions>`                    | ``override``                                 | ``abstract``                                 | :ref:`upcast <expressions>`                  |
+----------------------------------------------+----------------------------------------------+----------------------------------------------+----------------------------------------------+----------------------------------------------+----------------------------------------------+
| :ref:`iterator <iterators>`                  | :ref:`var <statements>`                      | :ref:`addr <expressions>`                    | :ref:`continue <statements>`                 | ``where``                                    | :ref:`pass <statements>`                     |
+----------------------------------------------+----------------------------------------------+----------------------------------------------+----------------------------------------------+----------------------------------------------+----------------------------------------------+
| ``reinterpret``                              | :ref:`module <modules>`                      | ``public``                                   | ``label``                                    | ``goto``                                     | ``implicit``                                 |
+----------------------------------------------+----------------------------------------------+----------------------------------------------+----------------------------------------------+----------------------------------------------+----------------------------------------------+
| ``shared``                                   | ``private``                                  | ``smart_ptr``                                | :ref:`generator <generators>`                | :ref:`yield <statements>`                    | :ref:`unsafe <unsafe>`                       |
+----------------------------------------------+----------------------------------------------+----------------------------------------------+----------------------------------------------+----------------------------------------------+----------------------------------------------+
| :ref:`assume <statements>`                   | ``explicit``                                 | ``sealed``                                   | ``static``                                   | :ref:`inscope <statements>`                  | ``fixed_array``                              |
+----------------------------------------------+----------------------------------------------+----------------------------------------------+----------------------------------------------+----------------------------------------------+----------------------------------------------+
| ``typedecl``                                 | ``capture``                                  | ``default``                                  | ``uninitialized``                            |                                              | ``template``                                 |
+----------------------------------------------+----------------------------------------------+----------------------------------------------+----------------------------------------------+----------------------------------------------+----------------------------------------------+

The following words are reserved as built-in type names and cannot be used as identifiers:

+----------------------------------------------+----------------------------------------------+----------------------------------------------+----------------------------------------------+----------------------------------------------+----------------------------------------------+
| ``bool``                                     | ``void``                                     | ``string``                                   | ``auto``                                     | ``int``                                      | ``int2``                                     |
+----------------------------------------------+----------------------------------------------+----------------------------------------------+----------------------------------------------+----------------------------------------------+----------------------------------------------+
| ``int3``                                     | ``int4``                                     | ``uint``                                     | :ref:`bitfield <bitfields>`                  | ``uint2``                                    | ``uint3``                                    |
+----------------------------------------------+----------------------------------------------+----------------------------------------------+----------------------------------------------+----------------------------------------------+----------------------------------------------+
| ``uint4``                                    | ``float``                                    | ``float2``                                   | ``float3``                                   | ``float4``                                   | ``range``                                    |
+----------------------------------------------+----------------------------------------------+----------------------------------------------+----------------------------------------------+----------------------------------------------+----------------------------------------------+
| ``urange``                                   | :ref:`block <blocks>`                        | ``int64``                                    | ``uint64``                                   | ``double``                                   | ``function``                                 |
+----------------------------------------------+----------------------------------------------+----------------------------------------------+----------------------------------------------+----------------------------------------------+----------------------------------------------+
| :ref:`lambda <lambdas>`                      | ``int8``                                     | ``uint8``                                    | ``int16``                                    | ``uint16``                                   | :ref:`tuple <tuples>`                        |
+----------------------------------------------+----------------------------------------------+----------------------------------------------+----------------------------------------------+----------------------------------------------+----------------------------------------------+
| :ref:`variant <variants>`                    | ``range64``                                  | ``urange64``                                 |                                              |                                              |                                              |
+----------------------------------------------+----------------------------------------------+----------------------------------------------+----------------------------------------------+----------------------------------------------+----------------------------------------------+

Keywords and types are covered in detail in subsequent sections of this documentation.
Linked keywords above lead to their relevant documentation pages.

-----------
Operators
-----------

.. index:: single: operators

Daslang recognizes the following operators:

+----------+----------+----------+----------+----------+----------+----------+----------+
| ``+=``   | ``-=``   | ``/=``   | ``*=``   | ``%=``   | ``|=``   | ``^=``   | ``<<``   |
+----------+----------+----------+----------+----------+----------+----------+----------+
| ``>>``   | ``++``   | ``--``   | ``<=``   | ``<<=``  | ``>>=``  | ``>=``   | ``==``   |
+----------+----------+----------+----------+----------+----------+----------+----------+
| ``!=``   | ``->``   | ``<-``   | ``??``   | ``?.``   | ``?[``   | ``<|``   | ``|>``   |
+----------+----------+----------+----------+----------+----------+----------+----------+
| ``:=``   | ``<<<``  | ``>>>``  | ``<<<=`` | ``>>>=`` | ``=>``   | ``+``    | ``@@``   |
+----------+----------+----------+----------+----------+----------+----------+----------+
| ``-``    | ``*``    | ``/``    | ``%``    | ``&``    | ``|``    | ``^``    |  ``>``   |
+----------+----------+----------+----------+----------+----------+----------+----------+
| ``<``    | ``!``    | ``~``    | ``&&``   | ``||``   | ``^^``   | ``&&=``  | ``||=``  |
+----------+----------+----------+----------+----------+----------+----------+----------+
| ``^^=``  | ``..``   |          |          |          |          |          |          |
+----------+----------+----------+----------+----------+----------+----------+----------+

Notable operators unique to Daslang:

* ``<-`` — move assignment
* ``:=`` — clone assignment
* ``??`` — null coalescing
* ``?.`` and ``?[`` — null-safe navigation
* ``<|`` and ``|>`` — pipe operators
* ``@@`` — function pointer / local function
* ``<<<`` and ``>>>`` — bit rotation
* ``^^`` — logical exclusive or
* ``..`` — interval (range creation)
* ``=>`` — tuple construction (``a => b`` creates ``tuple<auto,auto>(a, b)``; also used in table literals)

------------
Other Tokens
------------

.. index::
    single: delimiters
    single: other tokens

Other significant tokens are:

+----------+----------+----------+----------+----------+----------+
| ``{``    | ``}``    | ``[``    | ``]``    | ``.``    | ``:``    |
+----------+----------+----------+----------+----------+----------+
| ``::``   | ``'``    | ``;``    | ``"``    | ``@``    | ``$``    |
+----------+----------+----------+----------+----------+----------+
| ``#``    |          |          |          |          |          |
+----------+----------+----------+----------+----------+----------+

-----------
Literals
-----------

.. index::
    single: literals
    single: string literals
    single: numeric literals

Daslang accepts integer numbers, unsigned integers, floating-point and double-precision numbers,
and string literals.

^^^^^^^^^^^^^^^^
Numeric Literals
^^^^^^^^^^^^^^^^

+-------------------------------+------------------------------------------+
| ``34``                        | Integer (base 10)                        |
+-------------------------------+------------------------------------------+
| ``0xFF00A120``                | Unsigned integer (base 16)               |
+-------------------------------+------------------------------------------+
| ``075``                       | Unsigned integer (base 8)                |
+-------------------------------+------------------------------------------+
| ``13l``                       | 64-bit integer (base 10)                 |
+-------------------------------+------------------------------------------+
| ``0xFF00A120ul``              | 64-bit unsigned integer (base 16)        |
+-------------------------------+------------------------------------------+
| ``32u8``                      | 8-bit unsigned integer                   |
+-------------------------------+------------------------------------------+
| ``'a'``                       | Character literal (integer value)        |
+-------------------------------+------------------------------------------+
| ``1.52``                      | Floating-point number                    |
+-------------------------------+------------------------------------------+
| ``1.0f``                      | Floating-point number (explicit suffix)  |
+-------------------------------+------------------------------------------+
| ``1.e2``                      | Floating-point number (scientific)       |
+-------------------------------+------------------------------------------+
| ``1.e-2``                     | Floating-point number (scientific)       |
+-------------------------------+------------------------------------------+
| ``1.52d``                     | Double-precision number                  |
+-------------------------------+------------------------------------------+
| ``1.e2lf``                    | Double-precision number                  |
+-------------------------------+------------------------------------------+
| ``1.e-2d``                    | Double-precision number                  |
+-------------------------------+------------------------------------------+

Integer suffixes:

* ``u`` or ``U`` — unsigned 32-bit integer
* ``l`` or ``L`` — signed 64-bit integer
* ``ul`` or ``UL`` — unsigned 64-bit integer
* ``u8`` or ``U8`` — unsigned 8-bit integer

Float suffixes:

* ``f`` (or no suffix after a decimal point) — 32-bit float
* ``d``, ``lf`` — 64-bit double

^^^^^^^^^^^^^^^
String Literals
^^^^^^^^^^^^^^^

Strings are delimited by double quotation marks (``"``). They support escape sequences
and string interpolation:

.. code-block:: das

    "I'm a string\n"

**Escape sequences:**

+------------------+-------------------------------+
| ``\t``           | Tab                           |
+------------------+-------------------------------+
| ``\n``           | Newline                       |
+------------------+-------------------------------+
| ``\r``           | Carriage return               |
+------------------+-------------------------------+
| ``\\``           | Backslash                     |
+------------------+-------------------------------+
| ``\"``           | Double quote                  |
+------------------+-------------------------------+
| ``\'``           | Single quote                  |
+------------------+-------------------------------+
| ``\0``           | Null character                |
+------------------+-------------------------------+
| ``\a``           | Bell                          |
+------------------+-------------------------------+
| ``\b``           | Backspace                     |
+------------------+-------------------------------+
| ``\f``           | Form feed                     |
+------------------+-------------------------------+
| ``\v``           | Vertical tab                  |
+------------------+-------------------------------+
| ``\xHH``         | Hexadecimal byte              |
+------------------+-------------------------------+
| ``\uHHHH``       | Unicode code point (16-bit)   |
+------------------+-------------------------------+
| ``\UHHHHHHHH``   | Unicode code point (32-bit)   |
+------------------+-------------------------------+

**Multiline strings:**

Strings can span multiple lines. The content includes all characters between the quotes:

.. code-block:: das

    let msg = "This is
        a multi-line
            string"

**String interpolation:**

Expressions enclosed in curly brackets ``{}`` inside a string are evaluated and their
results are inserted into the string:

.. code-block:: das

    let name = "world"
    let greeting = "Hello, {name}!"         // "Hello, world!"
    let result = "2 + 2 = {2 + 2}"         // "2 + 2 = 4"

To include literal curly brackets in a string, escape them with a backslash:

.. code-block:: das
    :force:

    print("Use \{braces\} for interpolation")   // prints: Use {braces} for interpolation

**Format specifiers:**

String interpolation supports optional format specifiers after a colon:

.. code-block:: das

    let pi = 3.14159
    print("{pi:5.2f}")         // formatted output

(see :ref:`String Builder <string_builder>` for more details on string interpolation).

-----------
Comments
-----------

.. index:: single: comments

A comment is text that the compiler ignores but is useful for programmers.

**Block comments** start with ``/*`` and end with ``*/``. Block comments can span multiple
lines and can be nested:

.. code-block:: das

    /*
    This is a multiline comment.
    /* This is a nested comment. */
    These lines are all ignored by the compiler.
    */

**Line comments** start with ``//`` and extend to the end of the line:

.. code-block:: das

    // This is a single-line comment.
    var x = 42  // This is also a comment.

----------------------------------------------
Automatic Semicolons
----------------------------------------------

.. index:: single: automatic semicolons

Daslang automatically inserts semicolons at the end of lines when the code is enclosed
in curly braces, unless the line is also inside parentheses or brackets:

.. code-block:: das

    def foo {
        var a = 0       // semicolon inserted automatically
        var b = 1;      // explicit semicolon (redundant but valid)
        var c = ( 1     // no automatic semicolon here (inside parentheses)
            + 2 ) * 3   // semicolon inserted after the closing parenthesis
    }

This means that expressions can be split across multiple lines when parentheses or brackets
keep them together:

.. code-block:: das

    let result = (
        some_long_function_name(arg1, arg2)
        + another_value
    )
