.. _blocks:

=====
Block
=====

Blocks are nameless functions which captures the local context by reference.
Blocks offer significant performance advantages over lambdas (see :ref:`Lambda <lambdas>`).

The block type can be declared with a function-like syntax::

    block_type ::= block { optional_block_type }
    optional_block_type ::= < { optional_block_arguments } { : return_type } >
    optional_block_arguments := ( block_argument_list )
    block_argument_list := argument_name : type | block_argument_list ; argument_name : type

    block < (arg1:int;arg2:float&):bool >

Blocks capture the current stack, so blocks can be passed, but never returned.
Block variables can only be passed as arguments.
Global or local block variables are prohibited; returning the block type is also prohibited::

    def goo ( b : block )
        ...

    def foo ( b : block < (arg1:int; arg2:float&) : bool >
        ...

Blocks can be called via ``invoke``::

    def radd(var ext:int&;b:block<(var arg:int&):int>):int
        return invoke(b,ext)

Typeless blocks are typically declared via pipe syntax::

    goo() <|                                // block without arguments
        print("inside goo")

.. _blocks_declarations:

Similarly typed blocks are typically declared via pipe syntax::

    var v1 = 1                              // block with arguments
    res = radd(v1) <| $(var a:int&):int
        return a++

Blocks can also be declared via inline syntax::

    res = radd(v1, $(var a:int&) : int { return a++; }) // equivalent to example above

There is a simplified syntax for blocks that only contain a return expression::

    res = radd(v1, $(var a:int&) : int => a++ )         // equivalent to example above

If a block is sufficiently specified in the generic or function,
block types will be automatically inferred::

    res = radd(v1, $(a) => a++ )                        // equivalent to example above

Nested blocks are allowed::

    def passthroughFoo(a:Foo; blk:block<(b:Foo):void>)
        invoke(blk,a)

    passthrough(1) <| $ ( a )
        assert(a==1)
        passthrough(2) <| $ ( b )
            assert(a==1 && b==2)
            passthrough(3) <| $ ( c )
                assert(a==1 && b==2 && c==3)

Loop control expressions are not allowed to cross block boundaries::

    while true
        take_any() <|
            break               // 30801, captured block can't break outside the block

Blocks can have annotations::

    def queryOne(dt:float=1.0f)
        testProfile::queryEs() <| $ [es] (var pos:float3&;vel:float3 const)  // [es] is annotation
            pos += vel * dt

Block annotations can be implemented via appropriate macros (see :ref:`Macro <macros>`).

Local block variables are allowed::

    var blk = $ <| ( a, b : int )
        return a + b
    verify ( 3 == invoke(blk,1,2) )
    verify ( 7 == invoke(blk,3,4) )

They can't be copied, or moved.