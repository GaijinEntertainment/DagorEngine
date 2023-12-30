.. _temporary:

===============
Temporary types
===============

Temporary types are designed to address lifetime issues of data, which are exposed to Daslang directly from C++.

Let's review the following C++ example::

    void peek_das_string(const string & str, const TBlock<void,TTemporary<const char *>> & block, Context * context) {
        vec4f args[1];
        args[0] = cast<const char *>::from(str.c_str());
        context->invoke(block, args, nullptr);
    }

The C++ function here exposes a pointer a to c-string, internal to std::string.
From Daslang's perspective, the declaration of the function looks like this::

    def peek ( str : das_string; blk : block<(arg:string#):void> )

Where string# is a temporary version of a Daslang string type.

The main idea behind temporary types is that they can't `escape` outside of the scope of the block they are passed to.

Temporary values accomplish this by following certain rules.

Temporary values can't be copied or moved::

    def sample ( var t : das_string )
        var s : string
        peek(t) <| $ ( boo : string# )
            s = boo // error, can't copy temporary value

Temporary values can't be returned or passed to functions, which require regular values::

    def accept_string(s:string)
        print("s={s}\n")

    def sample ( var t : das_string )
        peek(t) <| $ ( boo : string# )
            accept_string(boo) // error

This causes the following error::

    30304: no matching functions or generics accept_string ( string const&# )
    candidate function:
            accept_string ( s : string const ) : void
                    invalid argument s. expecting string const, passing string const&#

Values need to be marked as ``implicit`` to accept both temporary and regular values.
These functions implicitly promise that the data will not be cached (copied, moved) in any form::

    def accept_any_string(s:string implicit)
        print("s={s}\n")

    def sample ( var t : das_string )
        peek(t) <| $ ( boo : string# )
            accept_any_string(boo)

Temporary values can and are intended to be cloned::

    def sample ( var t : das_string )
        peek(t) <| $ ( boo : string# )
            var boo_clone : string := boo
            accept_string(boo_clone)

Returning a temporary value is an unsafe operation.

A pointer to the temporary value can be received for the corresponding scope via the ``safe_addr`` macro::

    require daslib/safe_addr

    def foo
        var a = 13
        ...
        var b = safe_addr(a)    // b is int?#, and this operation does not require unsafe
        ...
