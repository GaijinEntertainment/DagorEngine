.. _lexical_structure:


=================
Lexical Structure
=================

.. index:: single: lexical structure

-----------
Identifiers
-----------

.. index:: single: identifiers

Identifiers start with an alphabetic character (and not the symbol '_') followed by any number
of alphabetic characters, '_' or digits ([0-9]). Daslang is a case sensitive language
meaning that the lowercase and uppercase representation of the same alphabetic
character are considered different characters. For instance, "foo", "Foo" and "fOo" are
treated as 3 distinct identifiers.

-----------
Keywords
-----------

.. index:: single: keywords

The following words are reserved as keywords and cannot be used as identifiers:

+------------+------------+-----------+------------+------------+-------------+
| struct     | class      | let       | def        | while      | if          |
+------------+------------+-----------+------------+------------+-------------+
| static_if  | else       | for       | recover    | true       | false       |
+------------+------------+-----------+------------+------------+-------------+
| new        | typeinfo   | type      | in         | is         | as          |
+------------+------------+-----------+------------+------------+-------------+
| elif       | static_elif| array     | return     | null       | break       |
+------------+------------+-----------+------------+------------+-------------+
| try        | options    | table     | expect     | const      | require     |
+------------+------------+-----------+------------+------------+-------------+
| operator   | enum       | finally   | delete     | deref      | aka         |
+------------+------------+-----------+------------+------------+-------------+
| typedef    | with       | cast      | override   | abstract   | upcast      |
+------------+------------+-----------+------------+------------+-------------+
| iterator   | var        | addr      | continue   | where      | pass        |
+------------+------------+-----------+------------+------------+-------------+
| reinterpret| module     | public    | label      | goto       | implicit    |
+------------+------------+-----------+------------+------------+-------------+
| shared     | private    | smart_ptr | generator  | yield      | unsafe      |
+------------+------------+-----------+------------+------------+-------------+
| assume     | explicit   | sealed    | static     | inscope    |             |
+------------+------------+-----------+------------+------------+-------------+

The following words are reserved as type names and cannot be used as identifiers:

+------------+------------+-----------+------------+------------+-------------+
| bool       | void       | string    | auto       | int        | int2        |
+------------+------------+-----------+------------+------------+-------------+
| int3       | int4       | uint      | bitfield   | uint2      | uint3       |
+------------+------------+-----------+------------+------------+-------------+
| uint4      | float      | float2    | float3     | float4     | range       |
+------------+------------+-----------+------------+------------+-------------+
| urange     | block      | int64     | uint64     | double     | function    |
+------------+------------+-----------+------------+------------+-------------+
| lambda     | int8       | uint8     | int16      | uint16     | tuple       |
+------------+------------+-----------+------------+------------+-------------+
| variant    |            |           |            |            |             |
+------------+------------+-----------+------------+------------+-------------+

Keywords and types are covered in detail later in this document.

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
| ``-``    | ``*``    | ``/``    | ``%``    | ``&``    | ``|``    | ``^``    |   ``>``  |
+----------+----------+----------+----------+----------+----------+----------+----------+
| ``<``    | ``!``    | ``~``    | ``&&``   | ``||``   | ``^^``   | ``&&=``  | ``||=``  |
+----------+----------+----------+----------+----------+----------+----------+----------+
| ``^^=``  |          |          |          |          |          |          |          |
+----------+----------+----------+----------+----------+----------+----------+----------+


------------
Other tokens
------------

.. index::
    single: delimiters
    single: other tokens

Other significant tokens are:

+----------+----------+----------+----------+----------+----------+
| ``{``    | ``}``    | ``[``    | ``]``    | ``.``    | ``:``    |
+----------+----------+----------+----------+----------+----------+
| ``::``   | ``'``    | ``;``    | ``"``    | ``]]``   |  ``[[``  |
+----------+----------+----------+----------+----------+----------+
| ``[{``   | ``}]``   | ``{{``   | ``}}``   | ``@``    |  ``$``   |
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

Daslang accepts integer numbers, unsigned integers, floating and double point numbers and string literals.

+-------------------------------+------------------------------------------+
| ``34``                        | Integer number(base 10)                  |
+-------------------------------+------------------------------------------+
| ``0xFF00A120``                | Unsigned Integer number(base 16)         |
+-------------------------------+------------------------------------------+
| ``0753``                      | Integer number(base 8)                   |
+-------------------------------+------------------------------------------+
| ``'a'``                       | Integer number                           |
+-------------------------------+------------------------------------------+
| ``1.52``                      | Floating point number                    |
+-------------------------------+------------------------------------------+
| ``1.e2``                      | Floating point number                    |
+-------------------------------+------------------------------------------+
| ``1.e-2``                     | Floating point number                    |
+-------------------------------+------------------------------------------+
| ``1.52d``                     | Double point number                      |
+-------------------------------+------------------------------------------+
| ``1.e2d``                     | Double point number                      |
+-------------------------------+------------------------------------------+
| ``1.e-2d``                    | Double point number                      |
+-------------------------------+------------------------------------------+
| ``"I'm a string"``            | String                                   |
+-------------------------------+------------------------------------------+
| ``" I'm a``                   |                                          |
| ``multiline verbatim string`` |                                          |
| ``"``                         | String                                   |
+-------------------------------+------------------------------------------+

Pesudo BNF:

.. productionlist::
    IntegerLiteral : [1-9][0-9]* | '0x' [0-9A-Fa-f]+ | ''' [.]+ ''' | 0[0-7]+
    FloatLiteral : [0-9]+ '.' [0-9]+
    FloatLiteral : [0-9]+ '.' 'e'|'E' '+'|'-' [0-9]+
    StringLiteral: '"'[.]* '"'
    VerbatimStringLiteral: '@''"'[.]* '"'

-----------
Comments
-----------

.. index:: single: comments

A comment is text that the compiler ignores, but is useful for programmers.
Comments are normally used to embed annotations in the code. The compiler
treats them as white space.

A comment can be ``/*`` (slash, asterisk) characters, followed by any
sequence of characters (including new lines),
followed by the ``*/`` characters. This syntax is the same as ANSI C::

    /*
    This is
    a multiline comment.
    This lines will be ignored by the compiler.
    */

A comment can also be ``//`` (two slash) characters, followed by any sequence of
characters.  A new line not immediately preceded by a backslash terminates this form of
comment.  It is commonly called a *"single-line comment"*::

    // This is a single line comment. This line will be ignored by the compiler.


------------------
Semantic Indenting
------------------

.. index:: single: indenting

Daslang follows semantic indenting (much like Python).
That means that logical blocks are arranged with the same indenting, and if a control statement requires the nesting of a block (such as the body of a function, block, if, for, etc.), it has to be indented one step more.
The indenting step is part of the options of the program.  It is either 2, 4 or 8, but always the same for whole file.
The default indenting is 4, but can be globally overridden per project.
