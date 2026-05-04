.. _embedding_project_files:

.. index::
   single: Embedding; Project Files
   single: Embedding; .das_project
   single: Embedding; Module Resolution
   single: Embedding; Sandboxing

=========================================
 Project Files (.das_project)
=========================================

A ``.das_project`` file is a daslang script that controls how the compiler
resolves modules, includes, and security policies.  It is the primary mechanism
for customizing the build environment without modifying C++ code.

Use cases:

- **Custom module layout** — map ``require foo`` to any file path
- **Sandboxing** — restrict which modules, options, and annotations scripts can use
- **Include resolution** — implement custom ``#include`` lookup
- **Multi-project setups** — each project gets its own resolution rules

.. contents::
   :local:
   :depth: 2


How it works
============

When the compiler receives a ``.das_project`` path, it:

1. Compiles and simulates the project file as a regular daslang program
2. Looks up exported callback functions by name (``module_get``, ``module_allowed``, etc.)
3. Sets the global variable ``DAS_PAK_ROOT`` to the directory containing the project file
4. Runs ``[init]`` functions (useful for registering module resolvers)
5. Calls the callbacks during compilation of user scripts

The only **required** callback is ``module_get``.  All others are optional —
when absent, the compiler uses permissive defaults (everything allowed, standard
include resolution).


Using project files
===================

From the command line
---------------------

.. code-block:: bash

   daslang -project path/to/project.das_project script.das

From C++
--------

.. code-block:: cpp

   auto access = make_smart<FsFileAccess>("project.das_project",
                                           make_smart<FsFileAccess>());
   // use 'access' when calling compileDaScript()

From daslang (runtime compilation)
-----------------------------------

.. code-block:: das

   var inscope access <- make_file_access("path/to/project.das_project")
   compile_file("script.das", access, unsafe(addr(mg)), cop) <| $(ok; program; issues) {
       // ...
   }

Pass an empty string to ``make_file_access("")`` for default filesystem access
without a project file.


Writing a project file
======================

A project file is an ordinary ``.das`` file.  Export the callbacks you need
using ``[export]``.

Minimal example
---------------

A minimal project file that resolves ``require daslib/...`` and relative imports:

.. code-block:: das

   options gen2

   require strings
   require daslib/strings_boost

   typedef module_info = tuple<string; string; string> const

   var DAS_PAK_ROOT = "./"

   [export]
   def module_get(req, from : string) : module_info {
       let rs <- split_by_chars(req, "./")
       let mod_name = rs[length(rs) - 1]
       // daslib modules
       if (length(rs) == 2 && rs[0] == "daslib") {
           return (mod_name, "{get_das_root()}/daslib/{mod_name}.das", "")
       }
       // relative modules
       var fr <- split_by_chars(from, "/")
       if (length(fr) > 0) {
           pop(fr)
       }
       for (se in rs) {
           push(fr, se)
       }
       let path_name = join(fr, "/") + ".das"
       return (mod_name, path_name, "")
   }

The return type ``module_info`` is a tuple of three strings:

1. **Module name** — how the module is known internally
2. **File path** — filesystem path to the ``.das`` file
3. **Import name** — alias (empty string if none)


DAS_PAK_ROOT
------------

If the project file declares a global variable named ``DAS_PAK_ROOT``, the
compiler automatically sets it to the directory containing the project file.
This lets callbacks resolve paths relative to the project root:

.. code-block:: das

   var DAS_PAK_ROOT = "./"

   [export]
   def module_get(req, from : string) : module_info {
       // resolve relative to project root
       return (req, "{DAS_PAK_ROOT}{req}.das", "")
   }


Including other project files
-----------------------------

Project files can include other project files and ``.das_module`` descriptors
using the ``include`` directive:

.. code-block:: das

   include common.das_project
   include foo/foo.das_module


Callback reference
==================

.. list-table::
   :widths: 30 70
   :header-rows: 1

   * - Callback
     - Description
   * - ``module_get(req, from : string) : module_info``
     - **Required.** Resolve a ``require`` statement. ``req`` is the module path
       (e.g. ``"foo"`` or ``"daslib/json"``), ``from`` is the file containing
       the ``require``. Return ``(module_name, file_path, import_name)``.
   * - ``include_get(inc, from : string) : string``
     - Resolve an ``include`` directive. Return the absolute file path.
   * - ``module_allowed(mod, filename : string) : bool``
     - Return ``true`` if the module ``mod`` may be loaded. Called for each
       ``require`` statement. When absent, all modules are allowed.
   * - ``module_allowed_unsafe(mod, filename : string) : bool``
     - Return ``true`` if the module ``mod`` may use ``unsafe`` blocks.
       When absent, ``unsafe`` is allowed everywhere.
   * - ``option_allowed(opt, from : string) : bool``
     - Return ``true`` if the ``options`` directive ``opt`` is accepted.
       When absent, all options are allowed.
   * - ``annotation_allowed(ann, from : string) : bool``
     - Return ``true`` if the annotation ``ann`` is accepted.
       When absent, all annotations are allowed.
   * - ``can_module_be_required(mod, fname : string; isPublic : bool) : bool``
     - Fine-grained control over ``require`` — receives the ``public`` flag.
       When absent, all requires are allowed.
   * - ``is_pod_in_scope_allowed(moduleName, fileName : string) : bool``
     - Control whether ``var inscope`` is allowed for POD types in the given
       module. When absent, ``inscope`` is allowed.
   * - ``is_same_file_name(a, b : string) : bool``
     - Custom file path comparison (e.g. case-insensitive matching on Windows).
       When absent, the default comparison is used.
   * - ``dyn_modules_folder() : string``
     - Return the directory to scan for ``.das_module`` dynamic module descriptors.
       When absent, the default ``modules/`` directory is used.


Sandboxing example
==================

A project file can lock down the scripting environment by combining
``module_allowed``, ``option_allowed``, and ``annotation_allowed``:

.. code-block:: das

   options gen2

   require strings
   require daslib/strings_boost

   typedef module_info = tuple<string; string; string> const
   var DAS_PAK_ROOT = "./"

   [export]
   def module_get(req, from : string) : module_info {
       let rs <- split_by_chars(req, "./")
       let mod_name = rs[length(rs) - 1]
       if (length(rs) == 2 && rs[0] == "daslib") {
           return (mod_name, "{get_das_root()}/daslib/{mod_name}.das", "")
       }
       var fr <- split_by_chars(from, "/")
       if (length(fr) > 0) {
           pop(fr)
       }
       for (se in rs) {
           push(fr, se)
       }
       return (mod_name, join(fr, "/") + ".das", "")
   }

   [export]
   def module_allowed(mod, filename : string) : bool {
       // only allow core and safe modules
       return mod == "$" || mod == "math" || mod == "strings"
   }

   [export]
   def module_allowed_unsafe(mod, filename : string) : bool {
       return false    // no unsafe anywhere
   }

   [export]
   def option_allowed(opt, from : string) : bool {
       return opt == "gen2" || opt == "indenting"
   }

   [export]
   def annotation_allowed(ann, from : string) : bool {
       return ann == "export" || ann == "private"
   }

With this project file, user scripts cannot ``require daslib/fio``, use ``unsafe``
blocks, enable ``options rtti``, or apply annotations like ``[function_macro]``.

.. seealso::

   :ref:`tutorial_integration_cpp_sandbox` — C++ integration tutorial demonstrating sandboxing

   :ref:`tutorial_integration_c_sandbox` — C integration tutorial with detailed callback reference

   :ref:`embedding_external_modules` — ``.das_module`` descriptors and dynamic module loading
