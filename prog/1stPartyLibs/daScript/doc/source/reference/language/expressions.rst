.. _expressions:


===========
Expressions
===========

.. index::
    single: Expressions

----------------
Assignment
----------------

.. index::
    single: assignment(=)
    single: move(<-)
    single: clone(:=)

::

    exp := exp '=' exp
    exp := exp '<-' exp
    exp := exp ':=' exp

daScript implements 3 kind of assignment: copy assignment(=)::

    a = 10

Copy assignment is equivalent of C++ memcpy operation::

    template <typename TT, typename QQ>
    __forceinline void das_copy ( TT & a, const QQ b ) {
        static_assert(sizeof(TT)<=sizeof(QQ),"can't copy from smaller type");
        memcpy(&a, &b, sizeof(TT));
    }

"Move" assignment::

    var b = new Foo
    var a: Foo?
    a <- b

Move assignment nullifies source (b).
It's main purpose is to correctly move ownership, and optimize copying if you don't need source for heavy types (such as arrays, tables).
Some external handled types can be non assignable, but still moveable.

Move assignment is equivalent of C++ memcpy + memset operations::

    template <typename TT, typename QQ>
    __forceinline void das_move ( TT & a, const QQ & b ) {
        static_assert(sizeof(TT)<=sizeof(QQ),"can't move from smaller type");
        memcpy(&a, &b, sizeof(TT));
        memset((TT *)&b, 0, sizeof(TT));
    }

"Clone" assignment::

    a := b

Clone assignment is syntactic sugar for calling clone(var a: auto&; b: auto&) if it exists or basic assignment for POD types.
It is also implemented for das_string, array and table types, and creates a 'deep' copy.

Some external handled types can be non assignable, but still cloneable (see :ref:`Clone <clone>`).

----------------
Operators
----------------

.. index::
    single: Operators

^^^^^^^^^^^^^^^^^^^
\.\. Operator
^^^^^^^^^^^^^^^^^^^

.. index::
    single: \.\. Operator; Operators

::

    expr1 \.\. expr2

This is equivalent to `inverval(expr1,expr2)`. By default `interval(a,b:int)` is implemented as `range(a,b)`,
and `interval(a,b:uint)` is implemented as `urange(a,b)`. Users can define their own interval functions or generics.

^^^^^^^^^^^^^
?: Operator
^^^^^^^^^^^^^

.. index::
    pair: ?: Operator; Operators

::

    exp := exp_cond '?' exp1 ':' exp2

This conditionally evaluate an expression depending on the result of an expression.
If expr_cond is true, only exp1 will be evaluated. Similarly, if false, only exp2.

^^^^^^^^^^^^^^^^^^^^^^^^^^^^
?? Null-coalescing operator
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. index::
    pair: ?? Operator; Operators

::

    exp := exp1 '??' exp2


Conditionally evaluate exp2 depending on the result of exp1.
The given code is equivalent to:

::

    exp := (exp1 '!=' null) '?' *exp1 ':' exp2


It evaluates expressions until the first non-null value
(just like | operators for the first 'true' one).

Operator precedence is also follows C# design, so that ?? has lower priority than |

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
?. and ?[ - Null-propagation operator
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. index::
    pair: ?. Operator; Operators

::

    exp := value '?.' key


If the value is not null, then dereferences the field 'key' for struct, otherwise returns null.

::

    struct TestObjectFooNative
        fooData : int

    struct TestObjectBarNative
        fooPtr: TestObjectFooNative?
        barData: float

    def test
        var a: TestObjectFooNative?
        var b: TestObjectBarNative?
        var idummy: int
        var fdummy: float
        a?.fooData ?? idummy = 1 // will return reference to idummy, since a is null
        assert(idummy == 1)

        a = new TestObjectFooNative
        a?.fooData ?? idummy = 2 // will return reference to a.fooData, since a is now not null
        assert(a.fooData == 2 & idummy == 1)

        b = new TestObjectBarNative
        b?.fooPtr?.fooData ?? idummy = 3 // will return reference to idummy, since while b is not null, but b.?barData is still null
        assert(idummy == 3)

        b.fooPtr <- a
        b?.fooPtr?.fooData ?? idummy = 4 // will return reference to b.fooPtr.fooData
        assert(b.fooPtr.fooData == 4 & idummy == 3)

Additionally, null propagation of index ?[ can be used with tables::

	var tab <- {{ "one"=>1; "two"=> 2 }}
	let i = tab?["three"] ?? 3
	print("i = {i}\n")

It checks both the container pointer and the availability of the key.

^^^^^^^^^^^^^
Arithmetic
^^^^^^^^^^^^^

.. index::
    pair: Arithmetic Operators; Operators

::

    exp:= 'exp' op 'exp'

daScript supports the standard arithmetic operators ``+, -, *, / and %``.
It also supports compact operators ``+=, -=, *=, /=, %=`` and
increment and decrement operators ``++ and --``::

    a += 2
    // is the same as writing
    a = a + 2
    x++
    // is the same as writing
    x = x + 1

All operators are defined for numeric and vector types, i.e (u)int* and float* and double.

^^^^^^^^^^^^^
Relational
^^^^^^^^^^^^^

.. index::
    pair: Relational Operators; Operators

::

    exp:= 'exp' op 'exp'

Relational operators in daScript are : ``==, <, <=, >, >=, !=``.

These operators return true if the expression is false and a value different than true if the
expression is true.

^^^^^^^^^^^^^^
Logical
^^^^^^^^^^^^^^

.. index::
    pair: Logical operators; Operators

::

    exp := exp op exp
    exp := '!' exp

Logical operators in daScript are : ``&&, ||, ^^, !, &&=, ||=, ^^=``.

The operator ``&&`` (logical and) returns false if its first argument is false, or otherwise returns
its second argument.
The operator ``||`` (logical or) returns its first argument if is different than false, or otherwise
returns the second argument.

The operator ``^^`` (logical exclusive or) returns true if arguments are different, and false otherwise.

It is important to understand, that && and || will not necessarily 'evaluate' all their arguments.
Unlike their C++ equivalents, &&= and ||= will also cancel evaluation of the right side.

The '!' (negation) operator will return false if the given value was true, or false otherwise.

^^^^^^^^^^^^^^^^^^^
Bitwise Operators
^^^^^^^^^^^^^^^^^^^

.. index::
    pair: Bitwise Operators; Operators

::

    exp:= 'exp' op 'exp'
    exp := '~' exp

daScript supports the standard C-like bitwise operators ``&, |, ^, ~, <<, >>, <<<, >>>``.
Those operators only work on integer values.

^^^^^^^^^^^^^^^^^^^
Pipe Operators
^^^^^^^^^^^^^^^^^^^

.. index::
    pair: Pipe Operators; Operators

::

    exp:= 'exp' |> 'exp'
    exp:= 'exp' <| 'exp'

daScript supports pipe operators. Pipe operators are similar to 'call' expressions where the other expression is first argument.

::

    def addX(a, b)
        assert(b == 2 || b == 3)
        return a + b
    def test
        let t = 12 |> addX(2) |> addX(3)
        assert(t == 17)
        return true

::

    def addOne(a)
        return a + 1

    def test
        let t =  addOne() <| 2
        assert(t == 3)

The ``lpipe`` macro allows piping to the previous line::

    require daslib/lpipe

    def main
        print()
        lpipe() <| "this is string constant"

In the example above, the string constant will be piped to the print expression on the previous line.
This allows piping of multiple blocks while still using significant whitespace syntax.

^^^^^^^^^^^^^^^^^^^^^
Operators precedence
^^^^^^^^^^^^^^^^^^^^^

.. index::
    pair: Operators Precedence; Operators

+--------------------------------------------------------------------------+-----------+
| ``post++  post--  .   ->  ?. ?[ *(deref)``                               | highest   |
+--------------------------------------------------------------------------+-----------+
| ``|>  <|``                                                               |           |
+--------------------------------------------------------------------------+-----------+
| ``is  as``                                                               |           |
+--------------------------------------------------------------------------+-----------+
| ``-  +  ~  !   ++  --``                                                  |           |
+--------------------------------------------------------------------------+-----------+
| ``??``                                                                   |           |
+--------------------------------------------------------------------------+-----------+
| ``/  *  %``                                                              |           |
+--------------------------------------------------------------------------+-----------+
| ``+  -``                                                                 |           |
+--------------------------------------------------------------------------+-----------+
| ``<<  >> <<< >>>``                                                       |           |
+--------------------------------------------------------------------------+-----------+
| ``<  <=  >  >=``                                                         |           |
+--------------------------------------------------------------------------+-----------+
| ``==  !=``                                                               |           |
+--------------------------------------------------------------------------+-----------+
| ``&``                                                                    |           |
+--------------------------------------------------------------------------+-----------+
| ``^``                                                                    |           |
+--------------------------------------------------------------------------+-----------+
| ``|``                                                                    |           |
+--------------------------------------------------------------------------+-----------+
| ``&&``                                                                   |           |
+--------------------------------------------------------------------------+-----------+
| ``^^``                                                                   |           |
+--------------------------------------------------------------------------+-----------+
| ``||``                                                                   |           |
+--------------------------------------------------------------------------+-----------+
| ``?  :``                                                                 |           |
+--------------------------------------------------------------------------+-----------+
| ``+=  =  -=  /=  *=  %=  &=  |=  ^=  <<=  >>=  <- <<<= >>>= &&= ||= ^^=``| ...       |
+--------------------------------------------------------------------------+-----------+
| ``=>``                                                                   |           |
+--------------------------------------------------------------------------+-----------+
| ``',' comma``                                                            | lowest    |
+--------------------------------------------------------------------------+-----------+

.. _array_contructor:

-----------------
Array Initializer
-----------------

.. index::
    single: Array Initializer

::

    exp := '[['type[] [explist] ']]'

Creates a new fixed size array::

    let a = [[int[] 1; 2]]     // creates array of two elements
    let a = [[int[2] 1; 2]]    // creates array of two elements
    var a = [[auto 1; 2]]      // creates which fully infers its own type
    let a = [[int[2] 1; 2; 3]] // error, too many initializers
    var a = [[auto 1]]         // int
    var a = [[auto[] 1]]       // int[1]

Arrays can be also created with array comprehensions::

    let q <- [[ for x in range(0, 10); x * x ]]

Similar syntax can be used to initialize dynamic arrays::

    let a <- [{int[3] 1;2;3 }]                      // creates and initializes array<int>
    let q <- [{ for x in range(0, 10); x * x }]     // comprehension which initializes array<int>

Only dynamic multi-dimensional arrays can be initialized (for now)::

    var a <- [[auto [{int 1;2;3}]; [{int 4;5}]]]    // array<int>[2]
    var a <- [{auto [{int 1;2;3}]; [{int 4;5}]}]    // array<array<int>>

(see :ref:`Arrays <arrays>`, :ref:`Comprehensions <comprehensions>`).

.. _struct_contructor:

-------------------------------------------
Struct, Class, and Handled Type Initializer
-------------------------------------------

.. index::
    single: Struct, Class, and Handled type Initializer

::

    struct Foo
      x: int = 1
      y: int = 2

    let fExplicit = [[Foo x = 13, y = 11]]              // x = 13, y = 11
    let fPartial  = [[Foo x = 13]]                      // x = 13, y = 0
    let fComplete = [[Foo() x = 13]]                    // x = 13, y = 2 with 'construct' syntax
    let aArray    = [[Foo() x=11,y=22; x=33; y=44]]     // array of Foo with 'construct' syntax

Initialization also supports optional inline block::

    var c = [[ Foo x=1, y=2 where $ ( var foo ) { print("{foo}"); } ]]

Classes and handled (external) types can also be initialized using structure initialization syntax. Classes and handled types always require constructor syntax, i.e. ().

(see :ref:`Structs <structs>`, :ref:`Classes <classes>`, :ref:`Handles <handles>` ).

.. _tuple_contructor:

-----------------
Tuple Initializer
-----------------

.. index::
    single: Tuple Initializer

Create new tuple::

    let a = [[tuple<int;float;string> 1, 2.0, "3"]]     // creates typed tuple
    let b = [[auto 1, 2.0, "3"]]                        // infers tuple type
    let c = [[auto 1, 2.0, "3"; 2, 3.0, "4"]]           // creates array of tuples

(see :ref:`Tuples <tuples>`).

-------------------
Variant Initializer
-------------------

.. index::
    single: Variant Initializer

Variants are created with a syntax similar to that of a structure::

    variant Foo
        i : int
        f : float

    let x = [[Foo i = 3]]
    let y = [[Foo f = 4.0]]
    let a = [[Foo[2] i=3; f=4.0]]   // array of variants
    let z = [[Foo i = 3, f = 4.0]]  // syntax error, only one initializer

(see :ref:`Variants <variants>`).

-----------------
Table Initializer
-----------------

.. index::
    single: Table Initializer

Tables are created by specifying key => value pairs separated by semicolon::

    var a <- {{ 1=>"one"; 2=>"two" }}
    var a <- {{ 1=>"one"; 2=>2 }}       // error, type mismatch

(see :ref:`Tables <tables>`).
