.. _diff_from_original:

*******************************************
Differences from Squirrel language
*******************************************

.. index::
    single: diff_from_original


Squirrel language was used a lot in all Gaijin projects since 2006.
Gaijin used Lua before but found Lua too unsafe and hard to debug, and also hard sometimes to bind with native code due to lack of types.
It is well known and there are lot info on this in the Internet.
Squirrel has not only C++ like syntax (which is helpful for C++ programmers sometimes), but also has type system much closer to C\C++ code.
It also has safer syntax and semantic.
However during heavy usage we found that some features of language are not used in our real codebase of millions of LoC in 20+ projects on 10+ platforms.
Some language features were also not very safe, or inconsistent. We needed stricter language with tools to easier refactor and support.
And of course in game-development any extra performance is always welcome.
Basically in any doubt we used 'Zen of Python'.

We renamed language from Squirrel to Quirrel to avoid ambiguity.

------------
New features
------------

* Check for conflicting local variable, local functions, function arguments and constant names
* Check for conflicting table slots declaration
* Null propagation, null-call and null-coalescing operators
* 'not in' operator
* Destructuring assignment
* ES2015-style shorthand table initialization
* String interpolation
* Functions declared in expressions can be named now (local foo = function foo() {})
* Named bindings declaration ('let')
* Support call()/acall() delegates for classes
* min(), max(), clamp() global functions added to base library
* string default delegates: .subst(), .replace(), .join(), .split(), .concat() methods added
* table default delegates: .map(), .each(), .findindex(), .findvalue(), .reduce(),
  __merge(), .__update() methods added
* array default delegates: .each(), .findvalue(), .replace()  methods added
* Support negative indexes in array.slice()
* Class methods hinting (that can gives performance of LuaJIT in nonJIT mode)
* Compiler directives for stricter and thus safer language (some of them can be used to test upcoming or planned language changes)
* Added C APIs not using stack pushes/pops
* Variable change tracking


----------------
Removed features
----------------

* Support for comma operator removed (were never used, but were error-prone, like table = {["foo", "bar"] = null} instead of [["foo","bar"] = null])
* Class and member attributes removed (were never used in any real code)
* Adding table/class methods with class::method syntaxic sugar disallowed (as There should be one -- and preferably only one -- obvious way to do it.)
* # single line comments removed (as There should be one -- and preferably only one -- obvious way to do it.)
* Support for post-initializer syntax removed (was ambiguous, especially because of optional comma)
* Removed support for octal numbers (error-prone)

----------------
Changes
----------------

* Functions and classes are locally scoped
* Referencing the closure environment object ('this') is explicit
* There is no fallback to global root table for undeclared variables, need to use `::` operator now
* Constants and enums are now locally scoped by default
* 'global' keyword for declaring global constants and enum explicitly
* Arrays/strings .find() default delegate renamed to .indexof()
* getinfos() renamed to getfuncinfos() and now can be applied to callable objects (instances, tables)
* array.append() can take multiple arguments
* changed arguments order for array.filter() to unify with other functions

--------------------------------
Possible future changes
--------------------------------

* adding operator\syntax for type methods call (for tables) and remove default delegates (remove ambiguity in {__update = @(...) null}.__update({}) and gain some performance)
* remove + for string concatenation
* don't accept non-boolean values in logical operation ({} && 0 || null || "foo", {} ? 1 : 2, if ("foo") print(a))
* add typedef and typehinting for analyzer like Python does (function (var: Int):Int{}. @(v:String):String v.tostring())
* change compiler to AST base (source->ast->IL->bytecode) with optimizations

