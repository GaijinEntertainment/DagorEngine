.. _statements:


=================
Statements
=================

.. index::
    single: statements

A daScript program is a simple sequence of statements::

    stats ::= stat [';'|'\n'] stats

Statements in daScript are comparable to those in C-family languages (C/C++, Java, C#,
etc.): there are assignments, function calls, program flow control structures, etc.  There are also some
custom statements like blocks, structs, and initializers (which will be covered in detail
later in this document).
Statements can be separated with a new line or ';'.

----------------
Visibility Block
----------------

.. index::
    pair: block; statement

::

    visibility_block ::= indent (stat)* unindent
    visibility_block ::= '{' (stat)* '}'

A sequence of statements delimited by indenting or curly brackets ({ }) is called a visibility_block.

-----------------------
Control Flow Statements
-----------------------

.. index::
    single: control flow statements

daScript implements the most common control flow statements: ``if, while, for``

^^^^^^^^^^^^^^
true and false
^^^^^^^^^^^^^^

.. index::
    single: true and false
    single: true
    single: false

daScript has a strong boolean type (bool). Only expressions with a boolean type can be part of the condition in control statements.

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
if/elif/else statement
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. index::
    pair: if/else; statement
    pair: if; statement
    pair: else; statement

::

    stat ::= 'if' exp '\n' visibility_block (['elif' exp '\n' visibility_block])*  ['else' '\n' visibility_block]

Conditionally executes a statement depending on the result of an expression::

    if a > b
        a = b
    elif a < b
        b = a
    else
        print("equal")

^^^^^^^^^^^^^^^^^
while statement
^^^^^^^^^^^^^^^^^

.. index::
    pair: while; statement

::

    stat ::= 'while' exp '\n' indent stat

Executes a statement while the condition is true::

      while true
          if a<0
              break

------------
Ranged Loops
------------

.. index::
    single: Loops

^^^^^^^^
for
^^^^^^^^

.. index::
    pair: for; statement

::

    stat ::= 'for' iterator 'in' [rangeexp] '\n' visibility_block

Executes a loop body statement for every element/iterator in expression, in sequenced order::

    for i in range(0, 10)
        print("{i}")       // will print numbers from 0 to 9

    // or

    let arr: array<int>
    resize(arr, 4)
    for i in arr
        print("{i}")       // will print content of array from first element to last

    // or

    var a: array<int>
    var b: int[10]
    resize(a, 4)
    for l, r in a, b
        print("{l}=={r}")  // will print content of a array and first 4 elements of array b

    // or

    var tab: table<string; int>
    for k, v in keys(tab), values(tab)
        print("{k}:{v}")   // will print content of table, in form key:value

Iterable types are implemented via iterators (see :ref:`Iterators <iterators>`).

-------
break
-------

.. index::
    pair: break; statement

::

    stat ::= 'break'

The break statement terminates the execution of a loop (``for`` or ``while``).

---------
continue
---------

.. index::
    pair: continue; statement

::

    stat ::= 'continue'

The continue operator jumps to the next iteration of the loop, skipping the execution of
the rest of the statements.

---------
return
---------

.. index::
    pair: return; statement

::

    stat ::= return [exp]
    stat ::= return <- exp

The return statement terminates the execution of the current function, block, or lambda, and
optionally returns the result of an expression. If the expression is omitted, the function
will return nothing, and the return type is assumed to be void.
Returning mismatching types from same function is an error (i.e., all returns should return a value of the same type).
If the function's return type is explicit, the return expression should return the same type.

Example::

    def foo(a: bool)
        if a
          return 1
        else
          return 0.f  // error, different return type

    def bar(a: bool): int
        if a
          return 1
        else
          return 0.f  // error, mismatching return type

    def foobar(a)
        return a  // return type will be same as argument type

In generator blocks, return must always return boolean expression,
where false indicates end of generation.

'return <- exp' syntax is for move-on-return::

    def make_array
        var a: array<int>
        a.resize(10)  // fill with something
        return <- a   // return will return

    let a <- make_array() //create array filled with make_array

------
yield
------

Yield serves similar purpose as ``return`` for generators (see :ref:`Generators <generators>`).

It is similar to return syntax, but can only be used inside ``generator`` blocks.

Yield must always produce a value which matches that of the generator::

    let gen <- generator<int>() <| $()
        yield 0         // int 0
        yield 1         // int 1
        return false

------------------
Finally statement
------------------

.. index::
    pair: finally; statement

::

    stat ::= finally visibility-block

Finally declares a block which will be executed once for any block (including control statements).
A finally block can't contain ``break``, ``continue``, or ``return`` statements.
It is designed to ensure execution after 'all is done'. Consider the following::

    def test(a: array<int>; b: int)
        for x in a
            if x == b
                return 10
         return -1
    finally
         print("print anyway")

    def test(a: array<int>; b: int)
        for x in a
            if x == b
                print("we found {x}")
                break
        finally
             print("we print this anyway")

Finally may be used for resource de-allocation.

It's possible to add code to the finally statement of the block with the ``defer`` macro::

    require daslib/defer

    def foo
        print("a\n")
    finally
        print("b\n")

    def bar
        defer <|
            print("b\n")
        print("a\n")

In the example above, functions ``foo`` and ``bar`` are semantically identical.
Multiple ``defer`` statements occur in reverse order.

The ``defer_delete`` macro adds a delete statement for its argument, and does not require a block.

---------------------------
Local variables declaration
---------------------------

.. index::
    pair: Local variables declaration; statement

::

    initz ::= id [:type] [= exp]
    initz ::= id [:type] [<- exp]
    initz ::= id [:type] [:= exp]
    scope ::= `inscope`
    ro_stat ::= 'let' [scope] initz
    rw_stat ::= 'var' [scope] initz

Local variables can be declared at any point in a function. They exist between their
declaration and the end of the visibility block where they have been declared.
``let`` declares read only variables, and ``var`` declares mutable (read-write) variables.

Copy ``=``, move ``->``, or clone ``:=`` semantics indicate how the variable is to be initialized.

If ``inscope`` is specified, the ``delete id`` statement is added in the finally section of the block, where the variable is declared.
It can't appear directly in the loop block, since finally section of the loop is executed only once.

--------------------
Function declaration
--------------------

.. index::
    pair: Function declaration; statement

::

    stat ::= 'def' id ['(' args ')'] [':' type ] visibility_block

    arg_decl = [var] id (',' id)* [':' type]
    args ::= (arg_decl)*

Declares a new function. Examples::

    def hello
        print("hello")

    def hello(): bool
        print("hello")
        return false

    def printVar(i: int)
        print("{i}")

    def printVarRef(i: int&)
        print("{i}")

    def setVar(var i: int&)
        i = i + 2

-----------
try/recover
-----------

.. index::
    pair: try/recover; statement

::

    stat ::= 'try' stat 'recover' visibility-block

The try statement encloses a block of code in which a panic condition can occur,
such as a fatal runtime error or a panic function. The try-recover clause provides the panic-handling
code.

It is important to understand that try/recover is not correct error handling code, and definitely not a way to implement control-flow.
Much like in the Go language, this is really an invalid situation which should not normally happen in a production environment.
Examples of potential exceptions are dereferencing a null pointer, indexing into an array out of bounds, etc.

-----------
panic
-----------

.. index::
    pair: panic; statement

::

    stat ::= 'panic' '(' [string-exp] ')'

Calling ``panic`` causes a runtime exception with string-exp available in the log.

----------------
global variables
----------------

.. index::
    pair: let; statement

::

    stat ::= 'let|var' { shared } {private} '\n' indent id '=' expression
    stat ::= 'let|var' { shared } {private} '\n' indent id '<-' expression
    stat ::= 'let|var' { shared } {private} '\n' indent id ':=' expression

Declares a constant global variable.
This variable is initialized once during initialization of the script (or each time when script init is manually called).

``shared`` indicates that the constant is to be initialized once,
and its memory is shared between multiple instances of the daScript context.

``private`` indicates that the variable is not visible outside of its module.

--------------
enum
--------------

.. index::
    pair: enum; statement

::

    enumerations ::= ( 'id' ) '\n'
    stat ::= 'enum' id indent enumerations unindent

Declares an enumeration (see :ref:`Constants & Enumerations <constants_and_enumerations>`).

--------------------
Expression statement
--------------------

.. index::
    pair: Expression statement; statement

::

    stat ::= exp

In daScript every expression is also allowed to be a statement.  If so, the result of the
expression is thrown away.

