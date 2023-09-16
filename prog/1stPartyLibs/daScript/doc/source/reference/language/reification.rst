.. _macros:

===========
Reification
===========

Expression reification is used to generate AST expression trees in a convenient way.
It provides a collection of escaping sequences to allow for different types of expression substitutions.
At the top level, reification is supported by multiple call macros, which are used to generate different AST objects.

Reification is implemented in daslib/templates_boost.

--------------
Simple example
--------------

Let's review the following example::

    var foo = "foo"
    var fun <- qmacro_function("madd") <| $ ( a, b )
        return $i(foo) * a + b
    print(describe(fun))

The output would be::

    def public madd (  a:auto const;  b:auto const ) : auto
        return (foo * a) + b

What happens here is that call to macro ``qmacro_function`` generates a new function named `madd`.
The arguments and body of that function are taken from the block, which is passed to the function.
The escape sequence $i takes its argument in the form of a string and converts it to an identifier (ExprVar).

------------
Quote macros
------------

Reification macros are similar to ``quote`` expressions because the arguments are not going through type inference.
Instead, an ast tree is generated and operated on.

******
qmacro
******

``qmacro`` is the simplest reification. The input is returned as is, after escape sequences are resolved::

    var expr <- qmacro(2+2)
    print(describe(expr))

prints::

    (2+2)

************
qmacro_block
************

``qmacro_block`` takes a block as an input and returns unquoted block. To illustrate the difference between ``qmacro`` and ``qmacro_block``,
let's review the following example::

    var blk1 <- qmacro <| $ ( a, b ) { return a+b; }
    var blk2 <- qmacro_block <| $ ( a, b ) { return a+b; }
    print("{blk1.__rtti}\n{blk2.__rtti}\n")

The output would be::

    ExprMakeBlock
    ExprBlock

This is because the block sub-expression is decorated, i.e. (ExprMakeBlock(ExprBlock (...))), and ``qmacro_block`` removes such decoration.

***********
qmacro_expr
***********

``qmacro_expr`` takes a block with a single expression as an input and returns that expression as the result.
Certain expressions like `return` and such can't be an argument to a call, so they can't be passed to ``qmacro`` directly.
The work around is to pass them as first line of a block::

    var expr <- qmacro_block <|
        return 13
    print(describe(expr))

prints::

    return 13

***********
qmacro_type
***********

``qmacro_type`` takes a type expression (type<...>) as an input and returns the subtype as a TypeDeclPtr, after resolving the escape sequences.
Consider the following example::

    var foo <- typeinfo(ast_typedecl type<int>)
    var typ <- qmacro_type <| type<$t(foo)?>
    print(describe(typ))

TypeDeclPtr foo is passed as a reification sequence to ``qmacro_type``, and a new pointer type is generated.
The output is::

    int?

***************
qmacro_function
***************

``qmacro_function`` takes two arguments. The first one is the generated function name. The second one is a block with a function body and arguments.
By default, the generated function only has the `FunctionFlags generated` flag set.

***************
qmacro_variable
***************

``qmacro_variable`` takes a variable name and type expression as an input, and returns the variable as a VariableDeclPtr,
after resolving the escape sequences::

    var vdecl <- qmacro_variable("foo", type<int>)
    print(describe(vdecl))

prints::

    foo:int

----------------
Escape sequences
----------------

Reification provides multiple escape sequences, which are used for miscellaneous template substitution.

*********
$i(ident)
*********

``$i`` takes a ``string`` or ``das_string`` as an argument and substitutes it with an identifier.
An identifier can be substituted for the variable name in both the variable declaration and use::

    var bus = "bus"
    var qb <- qmacro_block <|
        let $i(bus) = "busbus"
        let t = $i(bus)
    print(describe(qb))

prints::

	let  bus:auto const = "busbus"
	let  t:auto const = bus

**************
$f(field-name)
**************

``$f`` takes a ``string`` or ``das_string`` as an argument and substitutes it with a field name::

    var bar = "fieldname"
    var blk <- qmacro_block <|
        foo.$f(bar) = 13
    print(describe(blk))

prints::

    foo.fieldname = 13

*********
$v(value)
*********

``$v`` takes any value as an argument and substitutes it with an expression which generates that value.
The value does not have to be a constant expression, but the expression will be evaluated before its substituted.
Appropriate `make` infrastructure will be generated::

    var t = [[auto 1,2.,"3"]]
    var expr <- qmacro($v(t))
    print(describe(expr))

prints::

    [[1,2f,"3"]]

In the example above, a tuple is substituted with the expression that generates this tuple.

**************
$e(expression)
**************

``$e`` takes any expression as an argument in form of an ``ExpressionPtr``. The expression will be substituted as-is::

    var expr <- quote(2+2)
    var qb <- qmacro_block <|
        let foo = $e(expr)
    print(describe(qb))

prints::

    let foo:auto const = (2 + 2)

*****************
$b(array-of-expr)
*****************

``$b`` takes an ``array<ExpressionPtr>`` or ``das::vector<ExpressionPtr>`` aka ``dasvector`smart_ptr`Expression`` as an argument
and is replaced with each expression from the input array in sequential order::

    var qqblk : array<ExpressionPtr>
    for i in range(3)
        qqblk |> emplace_new <| qmacro(print("{$v(i)}\n"))
    var blk <- qmacro_block <|
        $b(qqblk)
    print(describe(blk))

prints::

    print(string_builder(0, "\n"))
    print(string_builder(1, "\n"))
    print(string_builder(2, "\n"))

*************
$a(arguments)
*************

``$a`` takes an ``array<ExpressionPtr>`` or ``das::vector<ExpressionPtr>`` aka ``dasvector`smart_ptr`Expression`` as an argument
and replaces call arguments with each expression from the input array in sequential order::

    var arguments <- [{ExpressionPtr quote(1+2); quote("foo")}]
    var blk <- qmacro <| somefunnycall(1,$a(arguments),2)
    print(describe(blk))

prints::

    somefunnycall(1,1 + 2,"foo",2)

Note how the other arguments of the function are preserved, and multiple arguments can be substituted at the same time.

Arguments can be substituted in the function declaration itself. In that case $a expects ``array<VariablePtr>``::

    var foo <- [{VariablePtr
        new [[Variable() name:="v1", _type<-qmacro_type(type<int>)]];
        new [[Variable() name:="v2", _type<-qmacro_type(type<float>), init<-qmacro(1.2)]]
    }]
    var fun <- qmacro_function("show") <| $ ( a: int; $a(foo); b : int )
        return a + b
    print(describe(fun))

prints::

    def public add ( a:int const; var v1:int; var v2:float = 1.2f; b:int const ) : int
        return a + b

********
$t(type)
********

``$t`` takes a ``TypeDeclPtr`` as an input and substitutes it with the type expression.
In the following example::

    var subtype <- typeinfo(ast_typedecl type<int>)
    var blk <- qmacro_block <|
        var a : $t(subtype)?
    print(describe(blk))

we create pointer to a subtype::

    var a:int? -const

*************
$c(call-name)
*************

``$c`` takes a ``string`` or ``das_string`` as an input, and substitutes the call expression name::

    var cll = "somefunnycall"
    var blk <- qmacro ( $c(cll)(1,2) )
    print(describe(blk))

prints::

    somefunnycall(1,2)

