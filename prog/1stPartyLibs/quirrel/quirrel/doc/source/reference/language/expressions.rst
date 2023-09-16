.. _expressions:


=================
Expressions
=================

.. index::
    single: Expressions

----------------
Assignment
----------------

.. index::
    single: assignment(=)

::

    exp := derefexp '=' exp

::

    a = 10

For adding new fields to tables there is the "new slot" operator. (see :ref:`Tables <tables>`)

::

    exp:= derefexp '<-' exp

::

    tbl.a <- 10
    tbl["b"] <- 20

If the slot already exists in the table this behaves like a normal assignment.

----------------
Operators
----------------

.. index::
    single: Operators

^^^^^^^^^^^^^
?: Operator
^^^^^^^^^^^^^

.. index::
    pair: ?: Operator; Operators

::

    exp := exp_cond '?' exp1 ':' exp2

conditionally evaluate an expression depending on the result of an expression.

^^^^^^^^^^^^^^^^^^^^^^^^^^^^
?? Null-coalescing operator
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. index::
    pair: ?? Operator; Operators

::

    exp := exp1 '??' exp2


Conditionally evaluate an expression2 depending on the result of an expression1.
Given code is equivalent to:

::

    exp := (exp1 '!=' null) '?' exp1 ':' exp2


C#-like ``??`` syntax was chosen over Elvis operator ``?:`` which is
common in other languages because it is not equivalent to
visually similar ternary ``? :`` operator (which checks for falsiness,
not null).

It evaluates expressions until the first non-null value
(just like ``||`` operators for the first ``true`` one).

Operator precendence is also follows C# design, so that ``??`` has
lower priority than ``||``


^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
?. and ?[] - Null-propagation operators
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. index::
    pair: ?. and ?[] Operators; Operators

::

    exp := value '?.' key


::

    exp := value '?[' key ']'


If key exists, return result of 'get' operations, else return null.

::

    let tbl = {bar=123}

    tbl.bar // returns 123
    tbl.baz // throws an error
    tbl?.bar // returns 123
    tbl?.baz // returns null
    null.bar // throws an error
    null?.bar // returns null
    tbl?["bar"] // returns 123
    tbl?[4567] // returns null


This works for any type (internally done via SQVM::Get(), like an 'in' operator), including null.
Therefore operator can be chained

::

    let x = tbl?.foo?.bar?.baz?["spam"]

To avoid extra typing, null-propagation operators affect the rest of expression.
Otherwise, an expression like

::

    a?.b.c.d

would make no sense because without automatic propagation a null value's slot could possibly be accessed in runtime.
One would have to type ?. everywhere, writing it as

::

    a?.b?.c?.d

Instead it is done by compiler - once a null-operator is met, it is also assumed for the subsequent ., [] and () operators in an expression.


^^^^^^^^^^^^^
Arithmetic
^^^^^^^^^^^^^

.. index::
    pair: Arithmetic Operators; Operators

::

    exp:= 'exp' op 'exp'

Quirrel supports the standard arithmetic operators ``+, -, *, / and %``.
Other than that is also supports compact operators (``+=,-=,*=,/=,%=``) and
increment and decrement operators(++ and --);::

    a += 2;
    //is the same as writing
    a = a + 2;
    x++
    //is the same as writing
    x = x + 1

All operators work normally with integers and floats; if one operand is an integer and one
is a float the result of the expression will be float.
The ``+`` operator has a special behavior with strings; if one of the operands is a string the
operator ``+`` will try to convert the other operand to string as well and concatenate both
together. For instances and tables, ``_tostring`` is invoked.

^^^^^^^^^^^^^
Relational
^^^^^^^^^^^^^

.. index::
    pair: Relational Operators; Operators

::

    exp:= 'exp' op 'exp'

Relational operators in Quirrel are : ``==, <, <=, <, <=, !=``

These operators return ``true`` if the expression is ``false`` and a value different than ``true`` if the
expression is ``true``. Internally the VM uses the integer ``1`` as ``true`` but this could change in
the future.

^^^^^^^^^^^^^^
3 ways compare
^^^^^^^^^^^^^^

.. index::
    pair: 3 ways compare operator; Operators

::

    exp:= 'exp' <=> 'exp'

the 3 ways compare operator <=> compares 2 values A and B and returns an integer less than 0
if A < B, 0 if A == B and an integer greater than 0 if A > B.

^^^^^^^^^^^^^^
Logical
^^^^^^^^^^^^^^

.. index::
    pair: Logical operators; Operators

::

    exp := exp op exp
    exp := '!' exp

Logical operators in Quirrel are : ``&&, ||, !``

The operator ``&&`` (logical and) returns null if its first argument is null, otherwise returns
its second argument.
The operator ``||`` (logical or) returns its first argument if is different than null, otherwise
returns the second argument.

The '!' operator will return null if the given value to negate was different than null, or a
value different than null if the given value was null.

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
in operator, not in operator
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. index::
    pair: in operator, not in operator; Operators

::

    exp:= keyexp 'in' tableexp
    exp:= keyexp 'not in' tableexp

Tests the existence of a slot in a table.
'in' operator returns true if *keyexp* is a valid key in *tableexp*
'not in' operator returns true if *keyexp* is missing in *tableexp*

::

    let t = {
        foo="I'm foo",
        [123]="I'm not foo"
    }

    if("foo" in t) dostuff("yep");
    if(123 in t) dostuff();
    if(123 not in t) dostuff();

^^^^^^^^^^^^^^^^^^^
instanceof operator
^^^^^^^^^^^^^^^^^^^

.. index::
    pair: instanceof operator; Operators

::

    exp:= instanceexp 'instanceof' classexp

Tests if a class instance is an instance of a certain class.
Returns true if *instanceexp* is an instance of *classexp*.

^^^^^^^^^^^^^^^^^^^
typeof operator
^^^^^^^^^^^^^^^^^^^

.. index::
    pair: typeof operator; Operators

::

    exp:= 'typeof' exp

returns the type name of a value as string.::

    local a={},b="quirrel"
    print(typeof a); //will print "table"
    print(typeof b); //will print "string"

^^^^^^^^^^^^^^^^^^^
Bitwise Operators
^^^^^^^^^^^^^^^^^^^

.. index::
    pair: Bitwise Operators; Operators

::

    exp:= 'exp' op 'exp'
    exp := '~' exp

Quirrel supports the standard C-like bitwise operators ``&, |, ^, ~, <<, >>`` plus the unsigned
right shift operator ``>>>``. The unsigned right shift works exactly like the normal right shift operator(``>>``)
except for treating the left operand as an unsigned integer, so is not affected by the sign. Those operators
only work on integer values; passing of any other operand type to these operators will
cause an exception.

^^^^^^^^^^^^^^^^^^^^^
Operators precedence
^^^^^^^^^^^^^^^^^^^^^

.. index::
    pair: Operators precedence; Operators

+---------------------------------------+-----------+
| ``-, ~, !, typeof , ++, --``          | highest   |
+---------------------------------------+-----------+
| ``/, *, %``                           | ...       |
+---------------------------------------+-----------+
| ``+, -``                              |           |
+---------------------------------------+-----------+
| ``<<, >>, >>>``                       |           |
+---------------------------------------+-----------+
| ``<, <=, >, >=, instanceof``          |           |
+---------------------------------------+-----------+
| ``==, !=, <=>``                       |           |
+---------------------------------------+-----------+
| ``&``                                 |           |
+---------------------------------------+-----------+
| ``^``                                 |           |
+---------------------------------------+-----------+
| ``&&, in``                            |           |
+---------------------------------------+-----------+
| ``||``                                |           |
+---------------------------------------+-----------+
| ``??``                                |           |
+---------------------------------------+-----------+
| ``+=, =, -=, /=, *=, %=``             | ...       |
+---------------------------------------+-----------+

.. _table_constructor:

-----------------
Table Constructor
-----------------

.. index::
    single: Table Contructor

::

    tslots := ( 'id' '=' exp | '[' exp ']' '=' exp  | 'id' ) [',']
    exp := '{' [tslots] '}'

Creates a new table.::

    let a = {} //create an empty table

A table constructor can also contain slots declaration; With the syntax: ::

    let a = {
        slot1 = "I'm the slot value"
    }

An alternative syntax can be::

    '[' exp1 ']' = exp2 [',']

A new slot with exp1 as key and exp2 as value is created::

    let a = {
        [1]="I'm the value"
    }

ES2015-style shorthand table initialization is supported, so the code like below ::

    local x = 123
    local y = 345
    let tbl = {x=x, y=y}

can also be written as ::

    local x = 123
    local y = 345
    let tbl = {x, y}


All syntaxes can be mixed::

    local x = "bar"
    let table=
    {
        a=10,
        b="string",
        x,
        [10]={},
        function bau(a,b)
        {
            return a+b;
        }
    }

The comma between slots is optional.

^^^^^^^^^^^^^^^^^^^^^^
Table with JSON syntax
^^^^^^^^^^^^^^^^^^^^^^

.. index::
    single: Table with JSON syntax

Since Squirrel 3.0 is possible to declare a table using JSON syntax(see http://www.wikipedia.org/wiki/JSON).

the following JSON snippet: ::

    let x = {
      "id": 1,
      "name": "Foo",
      "price": 123,
      "tags": ["Bar","Eek"]
    }

is equivalent to the following quirrel code: ::

    let x = {
      id = 1,
      name = "Foo",
      price = 123,
      tags = ["Bar","Eek"]
    }

-----------------
clone
-----------------

.. index::
    single: clone

::

    exp:= 'clone' exp

Clone performs shallow copy of a table, array or class instance (copies all slots in the new object without
recursion).

After the new object is ready the "_cloned" meta method is called (see :ref:`Metamethods <metamethods>`).

When a class instance is cloned the constructor is not invoked(initializations must rely on ```_cloned``` instead

-----------------
Array contructor
-----------------

.. index::
    single: Array constructor

::

    exp := '[' [explist] ']'

Creates a new array.::

    a <- [] //creates an empty array

Arrays can be initialized with values during the construction::

    a <- [1,"string!",[],{}] //creates an array with 4 elements
