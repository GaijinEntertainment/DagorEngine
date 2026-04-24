.. _modules:

=======
Modules
=======

Modules provide infrastructure for code reuse,
as well as mechanism to expose C++ functionality to Daslang.
A module is a collection of types, constants, and functions.
Modules can be native to Daslang, as well as built-in.

For an overview of how ``module``, ``require``, and ``options`` fit into the overall file layout,
see :ref:`Program Structure <program_structure>`.

To request a module, use the ``require`` keyword:

.. code-block:: das

    require math
    require daslib/ast public
    require daslib/ast_boost

The ``public`` modifier indicates that the included module is visible to everything that includes the current module.

Module names may contain ``/`` and ``.`` symbols.
The project is responsible for resolving module names into file names (see :ref:`Project <modules_project>`).

--------------
Native modules
--------------

A native module is a separate Daslang file, with an optional ``module`` name:

.. code-block:: das

    module custom       // specifies module name
    ...
    def foo             // defines function in module
    ...

If not specified, the module name defaults to that of the file name.

Modules can be ``private`` or ``public``:

.. code-block:: das

    module Foo private

    module Foo public

The default publicity of functions, structures, and enumerations is that of the module
(i.e. if the module is public and a function's publicity is not specified, that function is public).


Module can be made visible to all modules in the project via the ``inscope`` modifier:

.. code-block:: das

    module Foo inscope

---------------
Builtin modules
---------------

Built-in modules are the way to expose C++ functionality to Daslang (see :ref:`Builtin modules <embedding_modules>`).

--------------
Shared modules
--------------

Shared modules are modules that are shared between compilation of multiple contexts.
Typically, modules are compiled anew for each context, but when the 'shared' keyword is specified, the module gets promoted to a builtin module:

.. code-block:: das

    module Foo shared

That way only one instance of the module is created per compilation environment.
Macros in shared modules can't expect the module to be unique, since sharing of the modules can be disabled via the code of policies.

--------------------------
Module function visibility
--------------------------

When calling a function, the name of the module can be specified explicitly or implicitly:

.. code-block:: das

    let s1 = sin(0.0)           // implicit, assumed math::sin
    let s2 = math::sin(0.0)     // explicit, always math::sin

If the function does not exist in that module, a compilation error will occur.
If the function is private or not directly visible, a compilation error will occur.
If multiple functions match an implicit function call, a compilation error will occur.

Module names ``_`` and ``__`` are reserved to specify the `current module` and the `current module only`, respectively.
It is particularly important for generic functions, which are always instanced as private functions in the current module:

.. code-block:: das

    module b

    [generic]
    def from_b_get_fun_4() {
        return  _::fun_4()      //  call `fun_4', as if it was implicitly called from b
    }

    [generic]
    def from_b_get_fun_5() {
        return  __::fun_5()     // always b::fun_5
    }

Specifying an empty prefix is the same as specifying no prefix.

Without the ``_`` or ``__`` module prefixes, overwritten functions would not be visible from generics.
That is why the ``:=`` and ``delete`` operators are always replaced with ``_::clone`` or ``_::finalize`` calls.

.. seealso::

    :ref:`Program structure <program_structure>` for module declaration and ``require`` statements,
    :ref:`Constants and enumerations <constants_and_enumerations>` for module-scoped constants,
    :ref:`Options <options>` for per-module options.

