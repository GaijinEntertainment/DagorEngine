.. _pattern-matching:

================
Pattern matching
================

Pattern matching allows you to compare a value against a set of structural patterns, extracting specific
fields when a pattern matches.
In Daslang, pattern matching is implemented via macros in the ``daslib/match`` module.

Enumeration Matching
--------------------

You can match on enumeration values using the ``match`` keyword. Each ``if`` clause represents a pattern to test.
The ``_`` pattern is a catch-all that matches anything not covered by previous cases:

.. code-block:: das

    enum Color {
        Black
        Red
        Green
        Blue
    }

    def enum_match (color:Color) {
        match ( color ) {
            if ( Color Black ) {
                return 0
            }
            if ( Color Red ) {
                return 1
            }
            if ( _ ) {
                return -1
            }
        }
    }

Matching Variants
-----------------

Variants can be matched using the ``as`` keyword to test and bind to a specific case:

.. code-block:: das

    variant IF {
        i : int
        f : float
    }

    def variant_as_match (v:IF) {
        match ( v ) {
            if ( _ as i ) {
                return "int"
            }
            if ( _ as f ) {
                return "float"
            }
            if ( _ ) {
                return "anything"
            }
        }
    }

Variants can also be matched using constructor syntax:

.. code-block:: das

    def variant_match (v : IF) {
        match ( v ) {
            if ( IF(i=$v(i)) ) {
                return 1
            }
            if ( IF(f=$v(f)) ) {
                return 2
            }
            if ( _ ) {
                return 0
            }
        }
    }

Here ``$v(i)`` declares a variable ``i`` that is bound to the matched variant field.

Declaring Variables in Patterns
-------------------------------

The ``$v(name)`` syntax declares a new variable and binds it to the matched value.
This works in any pattern, not just variant matching:

.. code-block:: das

    variant IF {
        i : int
        f : float
    }

    def variant_as_match (v:IF) {
        match ( v ) {
            if ( $v(as_int) as i ) {
                return as_int
            }
            if ( $v(as_float) as f ) {
                return as_float
            }
            if ( _ ) {
                return None
            }
        }
    }

Matching Structs
----------------

Structs can be matched by specifying field values or binding fields to variables:

.. code-block:: das

    struct Foo {
        a : int
    }

    def struct_match (f:Foo) {
        match ( f ) {
            if ( Foo(a=13) ) {
                return 0
            }
            if ( Foo(a=$v(anyA)) ) {
                return anyA
            }
        }
    }

The first case matches only when ``a`` is 13. The second case matches any ``Foo`` and binds ``a`` to the variable ``anyA``.

Using Guards
------------

Guards are additional conditions that must be satisfied for a match to succeed.
They are specified with ``&&`` after the pattern:

.. code-block:: das

    struct AB {
        a, b : int
    }

    def guards_match (ab:AB) {
        match ( ab ) {
            if ( AB(a=$v(a), b=$v(b)) && (b > a) ) {
                return "{b} > {a}"
            }
            if ( AB(a=$v(a), b=$v(b)) ) {
                return "{b} <= {a}"
            }
        }
    }

Tuple Matching
--------------

Tuples are matched using value or wildcard patterns. The ``...`` pattern matches any number of elements:

.. code-block:: das

    def tuple_match ( A : tuple<int;float;string> ) {
        match ( A ) {
            if (1,_,"3") {
                return 1
            }
            if (13,...) {      // starts with 13
                return 2
            }
            if (...,"13") {    // ends with "13"
                return 3
            }
            if (2,...,"2") {   // starts with 2, ends with "2"
                return 4
            }
            if ( _ ) {
                return 0
            }
        }
    }

The ``_`` matches any single element; ``...`` matches zero or more elements.

Matching Static Arrays
----------------------

Static arrays use the ``fixed_array`` keyword and support the same wildcard and guard patterns:

.. code-block:: das

    def static_array_match ( A : int[3] ) {
        match ( A ) {
            if ( fixed_array($v(a),$v(b),$v(c)) && (a+b+c)==6 ) { // total of 3 elements, sum is 6
                return 1
            }
            if ( fixed_array(0,...) ) {    // starts with 0
                return 0
            }
            if ( fixed_array(..,13) ) {   // ends with 13
                return 2
            }
            if ( fixed_array(12,...,12) ) {    // starts and ends with 12
                return 3
            }
            if ( _ ) {
                return -1
            }
        }
    }

Dynamic Array Matching
----------------------

Dynamic arrays use bracket syntax and support the same patterns as tuples and static arrays.
The number of explicit elements in the pattern is checked against the array length:

.. code-block:: das

    def dynamic_array_match ( A : array<int> ) {
        match ( A ) {
            if ( [$v(a),$v(b),$v(c)] && (a+b+c)==6 ) { // total of 3 elements, sum is 6
                return 1
            }
            if ( [0,0,0,...] ) {    // first 3 are 0
                return 0
            }
            if ( [...,1,2] ) {      // ends with 1,2
                return 2
            }
            if ( [0,1;...,2,3] ) {    // starts with 0,1, ends with 2,3
                return 3
            }
            if ( _ ) {
                return -1
            }
        }
    }

Match Expressions
-----------------

The ``match_expr`` pattern matches when an expression involving previously declared variables equals the value.
This is useful for expressing relationships between elements:

.. code-block:: das

    def ascending_array_match ( A : int[3] ) {
        match ( A ) {
            if ( fixed_array($v(x),match_expr(x+1),match_expr(x+2)) ) {
                return true
            }
            if ( _ ) {
                return false
            }
        }
    }

Here the first element is bound to ``x``, and the second and third elements must equal ``x+1`` and ``x+2`` respectively.

Matching with ``||``
--------------------

The ``||`` operator matches either of the provided patterns. Both sides must declare the same variables:

.. code-block:: das

    struct Bar {
        a : int
        b : float
    }

    def or_match ( B:Bar ) {
        match ( B ) {
            if ( Bar(a=1,b=$v(b)) || Bar(a=2,b=$v(b)) ) {
                return b
            }
            if ( _ ) {
                return 0.0
            }
        }
    }

[match_as_is] Structure Annotation
-----------------------------------

The ``[match_as_is]`` annotation enables pattern matching for structures of different types,
provided the necessary ``is`` and ``as`` operators have been implemented:

.. code-block:: das

    [match_as_is]
    struct CmdMove : Cmd {
        override rtti = "CmdMove"
        x : float
        y : float
    }

The required ``is`` and ``as`` operators:

.. code-block:: das

    def operator is CmdMove ( cmd:Cmd ) {
        return cmd.rtti=="CmdMove"
    }

    def operator is CmdMove ( anything ) {
        return false
    }

    def operator as CmdMove ( cmd:Cmd ==const ) : CmdMove const& {
        assert(cmd.rtti=="CmdMove")
        unsafe {
            return reinterpret<CmdMove const&> cmd
        }
    }

    def operator as CmdMove ( var cmd:Cmd ==const ) : CmdMove& {
        assert(cmd.rtti=="CmdMove")
        unsafe {
            return reinterpret<CmdMove&> cmd
        }
    }

    def operator as CmdMove ( anything ) {
        panic("Cannot cast to CmdMove")
        return default<CmdMove>
    }

With these operators in place, you can match against ``CmdMove`` in a ``match`` expression:

.. code-block:: das

    def matching_as_and_is (cmd:Cmd) {
        match ( cmd ) {
            if ( CmdMove(x=$v(x), y=$v(y)) ) {
                return x + y
            }
            if ( _ ) {
                return 0.
            }
        }
    }

[match_copy] Structure Annotation
----------------------------------

The ``[match_copy]`` annotation provides an alternative to ``[match_as_is]`` by using a ``match_copy`` function
instead of ``is``/``as`` operators:

.. code-block:: das

    [match_copy]
    struct CmdLocate : Cmd {
        override rtti = "CmdLocate"
        x : float
        y : float
        z : float
    }

The ``match_copy`` function attempts to copy the source into the target type, returning ``true`` on success:

.. code-block:: das

    def match_copy ( var cmdm:CmdLocate; cmd:Cmd ) {
        if ( cmd.rtti != "CmdLocate" ) {
            return false
        }
        unsafe {
            cmdm = reinterpret<CmdLocate const&> cmd
        }
        return true
    }

Usage is identical to regular struct matching:

.. code-block:: das

    def matching_copy ( cmd:Cmd ) {
        match ( cmd ) {
            if ( CmdLocate(x=$v(x), y=$v(y), z=$v(z)) ) {
                return x + y + z
            }
            if ( _ ) {
                return 0.
            }
        }
    }

Static Matching
---------------

``static_match`` works like ``match``, but ignores patterns with type mismatches at compile time instead
of reporting errors. This makes it suitable for generic functions:

.. code-block:: das

    static_match ( match_expression ) {
        if ( pattern_1 ) {
            return result_1
        }
        if ( pattern_2 ) {
            return result_2
        }
        ...
        if ( _ ) {
            return result_default
        }
    }

Example:

.. code-block:: das

    enum Color {
        red
        green
        blue
    }

    def enum_static_match ( color, blah ) {
        static_match ( color ) {
            if ( Color red ) {
                return 0
            }
            if ( match_expr(blah) ) {
                return 1
            }
            if ( _ ) {
                return -1
            }
        }
    }

If ``color`` is not ``Color``, the first case is silently skipped. If ``blah`` is not ``Color``, the second case is skipped.
The function always compiles regardless of the argument types.

match_type
----------

The ``match_type`` subexpression matches based on the type of an expression:

.. code-block:: das

    if ( match_type(type<Type>, expr) ) {
        // code to run if match is successful
    }

Example:

.. code-block:: das

    def static_match_by_type (what) {
        static_match ( what ) {
            if ( match_type(type<int>,$v(expr)) ) {
                return expr
            }
            if ( _ ) {
                return -1
            }
        }
    }

If ``what`` is of type ``int``, it is bound to ``expr`` and returned. Otherwise the catch-all returns ``-1``.

Multi-Match
-----------

``multi_match`` evaluates all matching cases instead of stopping at the first match:

.. code-block:: das

    def multi_match_test ( a:int ) {
        var text = "{a}"
        multi_match ( a ) {
            if ( 0 ) {
                text += " zero"
            }
            if ( 1 ) {
                text += " one"
            }
            if ( 2 ) {
                text += " two"
            }
            if ( $v(a) && (a % 2 == 0) && (a!=0) ) {
                text += " even"
            }
            if ( $v(a) && (a % 2 == 1) ) {
                text += " odd"
            }
        }
        return text
    }

Unlike ``match``, which stops at the first successful pattern, ``multi_match`` continues through all cases.
The equivalent code using regular ``match`` would require a separate ``match`` block for each case.

``static_multi_match`` is a variant of ``multi_match`` that works with ``static_match``.

.. seealso::

    :ref:`Variants <variants>` for the variant type used in ``as`` matching,
    :ref:`Enumerations <enumerations>` for enumeration types used in enum matching,
    :ref:`Tuples <tuples>` for tuple types matched by index,
    :ref:`Structs <structs>` for struct types matched by field,
    :ref:`Arrays <arrays>` for dynamic array matching,
    :ref:`Classes <classes>` for the ``[match_as_is]`` annotation,
    :ref:`Expressions <expressions>` for ``is``, ``as``, and ``?as`` operators.
