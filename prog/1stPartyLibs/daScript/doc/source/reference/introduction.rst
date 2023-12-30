.. _introduction:

************
Introduction
************

.. index::
    single: introduction

Daslang is a high-performance, strong and statically typed scripting language, designed to be high-performance
as an embeddable "scripting" language for real-time applications (like games).

Daslang offers a wide range of features like strong static typing, generic programming with iterative type inference,
Ruby-like blocks, semantic indenting, native machine types, ahead-of-time "compilation" to C++, and fast and simplified bindings to C++ program.

It's philosophy is build around a modified Zen of Python.

* *Performance counts.*
* *But not at the cost of safety.*
* *Unless is explicitly unsafe to be performant.*
* *Readability counts.*
* *Explicit is better than implicit.*
* *Simple is better than complex.*
* *Complex is better than complicated.*
* *Flat is better than nested.*

Daslang is supposed to work as a "host data processor".
While it is technically possible to maintain persistent state within a script context (with a certain option set),
Daslang is designed to transform your host (C++) data/implement scripted behaviors.

In a certain sense, it is pure functional - i.e. all persistent state is out of the scope of the scripting context, and the script's state is temporal by its nature.
Thus, the memory model and management of persistent state are the responsibility of the application.  This leads to an extremely simple and fast memory model in Daslang itself.

+++++++++++++
Performance.
+++++++++++++

In a real world scenarios, it's interpretation is 10+ times faster than LuaJIT without JIT (and can be even faster than LuaJIT with JIT).
Even more important for embedded scripting languages, its interop with C++ is extremely fast (both-ways), an order of magnitude faster than most other popular scripting languages.
Fast calls from C++ to Daslang allow you to use Daslang for simple stored procedures, and makes it an ECS/Data Oriented Design friendly language.
Fast calls to C++ from Daslang allow you to write performant scripts which are processing host (C++) data, and rely on bound host (C++) functions.

It also allows Ahead-of-Time compilation, which is not only possible on all platforms (unlike JIT), but also always faster/not-slower (JIT is known to sometimes slow down scripts).

Daslang already has implemented AoT (C++ transpiler) which produces code more or less similar with C++11 performance of the same program.

`Table with performance comparisons on a synthetic samples/benchmarks
<https://docs.google.com/spreadsheets/d/1y1G4exD4J9o3kPYw6Y-eaVoffbJ5h_mWVG121wp2k9s/htmlview>`_.

+++++++++++++
How it looks?
+++++++++++++

Mandatory fibonacci samples::

    def fibR(n)
       if (n < 2)
           return n
       else
           return fibR(n - 1) + fibR(n - 2)

    def fibI(n)
       var last = 0
       var cur = 1
       for i in range(0, n - 1)
           let tmp = cur
           cur += last
           last = tmp
       return cur

The same samples with curly brackets, for those who prefer this type of syntax::

    def fibR(n) {
        if (n < 2) {
            return n;
        } else {
            return fibR(n - 1) + fibR(n - 2);
        }
    }
    def fibI(n) {
        var last = 0;
        var cur = 1;
        for i in range(0, n-1); {
            let tmp = cur;
            cur += last;
            last = tmp;
        }
        return cur;
    }

Please note, that semicolons(';') are mandatory within curly brackets. You can actually mix both ways in your codes, but for clarity in the documentation, we will only use the pythonic way.

++++++++++++++++++++++++++++++++++++
Generic programming and type system
++++++++++++++++++++++++++++++++++++

Although above sample may seem to be dynamically typed, it is actually generic programming.
The actual instance of the fibI/fibR functions is strongly typed and basically is just accepting and returning an ``int``. This is similar to templates in C++ (although C++ is not a strong-typed language) or ML.
Generic programming in Daslang allows very powerful compile-time type reflection mechanisms, significantly simplifying writing optimal and clear code.
Unlike C++ with it's SFINAE, you can use common conditionals (if) in order to change the instance of the function depending on type info of its arguments.
Consider the following example::

    def setSomeField(var obj; val)
        static_if typeinfo(has_field<someField> obj)
            obj.someField = val

This function sets `someField` in the provided argument *if* it is a struct with a `someField` member.

(For more info, see :ref:`Generic programming <generic_programming>`).

+++++++++++++++++++++++
Compilation time macros
+++++++++++++++++++++++

Daslang does a lot of heavy lifting during compilation time so that it does not have to do it at run time.
In fact, the Daslang compiler runs the Daslang interpreter for each module and has the entire AST available to it.

The following example modifies function calls at compilation time to add a precomputed hash of a constant string argument::

    [tag_function_macro(tag="get_hint_tag")]
    class GetHintFnMacro : AstFunctionAnnotation
        [unsafe] def override transform ( var call : smart_ptr<ExprCall>;
            var errors : das_string ) : ExpressionPtr
            if call.arguments[1] is ExprConstString
                let arg2 = reinterpret<ExprConstString?>(call.arguments[1])
                var mkc <- new [[ExprConstUInt() at=arg2.at, value=hash("{arg2.value}")]]
                push(call.arguments, ExpressionPtr(mkc))
                return <- ExpressionPtr(call)
            return [[ExpressionPtr]]


++++++++++++++++++++++++++++++++++++
Features
++++++++++++++++++++++++++++++++++++
Its (not) full list of features includes:

* strong typing
* Ruby-like blocks and lambda
* tables
* arrays
* string-builder
* native (C++ friendly) interop
* generics
* classes
* macros, including reader macros
* semantic indenting
* ECS-friendly interop
* easy-to-extend type system
* etc.
