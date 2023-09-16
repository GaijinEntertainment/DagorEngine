.. _introduction:

************
Introduction
************

.. index::
    single: introduction

Quirrel is a fast, high-level imperative, OO programming language, designed to be a lightweight but powerful
scripting tool that fits within the size, memory bandwidth, and real-time requirements of
applications like games.

Quirrel has simple integration and bindings in any C++ application.

Quirrel has C/Java/C++ like syntax with clear, rich and simple type system, 
but the language has a very dynamic nature like Python/Lua/JavaScript.
Easy to start while being much safer than Lua and JS, having the equal performance with Lua.

It is based on Squirrel Script language (http://www.squirrel-lang.org/), but modified to be safer, stricter and faster.
Quirrel is not compatible with the original Squirrel (see :ref:`Diff from original <diff_from_original>`),
but conversion usually can be done easliy.

Quirrel offers a wide range of features like dynamic typing, higher
order functions, generators, tail recursion, exception handling, courutines, automatic memory
management while fitting both compiler and virtual machine into about 6k lines of C++
code.

Repository is available on https://github.com/GaijinEntertainment/quirrel and your PR's and feedback are welcome!



.. _more_details_on_quirrel:

----------------------------
More details on Quirrel
----------------------------

.. index::
    single: more details on quirrel

- Quirrel is a mature scripting language used in tens of games and other apps (including web servers and embeddable platforms)
  with hundreds of millions installations.
  It was created to be embeddable language to C/C++ applications with easy bindings and clear mental model.
- Quirrel is familiar and convenient for those who has experience with C, C++, JavaScript or Python.
  It has a small and simple but sufficient :ref:`types system <datatypes_and_values>`:
  float and integer types (not just 'numeric'), built-in tables, classes and arrays,
  functions, closures, generators and coroutines (as well as null, string and boolean types).
  In most cases it is completely obvious how to bind native code, class of function, how to call native code and work with native types.
  And of course, arrays indices start with 0.
  To be obvious for C++ programmer in both programming and binding is one of the main ideas Quirrel Language
  Python and JavaScript are not lightweight nor easy embeddable; JS have also infamous type system with all its implicitness.
  Lua on the other hand is as small as Quirrel, but because of many differences from C/C++
  it often confuses even experienced programmers.
- Quirrel has a robust and predictable :ref:`memory model <embedding_memory_management>`.
  In most cases with reference count you never have unexpected freezes in runtime as well as predictable memory consumption.
  However, we are planning to add GC memory management without reference count,
  as it can be more beneficial in some ways of using language
  (when you always have enough computational time in separate thread to manage memory it allows you to have less latency)  
- Quirrel support concurrency with :ref:`coroutines <threads>`.
- Quirrel wants to be safe. The main intent to evolve Squirrel into Quirrel was to make it safer.
  Being a dynamic language by its nature doesn't mean that you can be more safe.
  Safety comes from different small syntactic and semantic changes, allowing more compile time validations and explicitly,
  making code less ambiguous and clear to reader and writer.
  Things like 'let' :ref:`Named bindings declaration <statements>`, :ref:`freezing <builtin_functions>` of tables and arrays,
  :ref:`Destructuring assignment <destructuring_assignment>` and imports, deprecation of modifying classes,
  redefine table keys and several others, as well as implementing static analyzer, helps to prevent many errors and allow code to be stricter.
- Quirrel is fast.
  It is at least as fast as Lua (on compilation and runtime),
  with some tweaks to be sometimes closer to LuaJit with no JIT.
  We are also going to implement AST-based compiler with optimizations on AST,
  allowing code to become even faster in real-time (that will also allow making code safer and to add power of macros).
  Class hinting allow classes to be as fast as in `Wren <https://wren.io>`_ or LuaJIT.


