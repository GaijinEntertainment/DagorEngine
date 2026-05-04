.. _embedding_external_modules:

.. index::
   single: Embedding; External Modules
   single: Embedding; .das_module
   single: Embedding; Shared Modules
   single: Embedding; Dynamic Modules
   single: Embedding; find_package(DAS)

=========================================
 External Modules
=========================================

This page explains how to create, build, and distribute daslang modules
**outside** the main daScript repository.  External modules are compiled
and distributed separately from the daslang compiler, and can contain
C++ bindings, pure daslang code, or both.

For binding C++ functions and types *within* the compiler source tree,
see :ref:`embedding_modules`.  For step-by-step integration tutorials,
see :ref:`tutorial_integration_cpp_hello_world`.

.. contents::
   :local:
   :depth: 2


Overview
========

daslang has two compiler binaries:

.. list-table::
   :widths: 25 25 50
   :header-rows: 1

   * - Binary
     - CMake target
     - Module resolution
   * - ``daslang_static``
     - ``daslang_static``
     - All modules are compiled-in via ``NEED_MODULE`` / ``NATIVE_MODULE``
       macros and linked statically.
   * - ``daslang`` (default)
     - ``daslang``
     - Modules are discovered at runtime through ``.das_module`` descriptor
       scripts and ``.shared_module`` shared libraries.

The **static** binary is self-contained — every module is compiled into
the binary, and no external files are needed at runtime (except for
daslang source files).  It cannot load external modules.

The **dynamic** binary (the default, and the one shipped in the install)
discovers modules at startup by scanning the ``modules/`` directory for
``.das_module`` descriptor files.  This is the binary that supports
external modules.

When you install daslang (via ``cmake --install``), the installed SDK
contains:

* ``bin/daslang`` — the dynamic compiler binary
* ``lib/`` — ``libDaScriptDyn`` shared library (and static ``libDaScript``)
* ``include/`` — C++ headers
* ``lib/cmake/DAS/`` — CMake package files (``DASConfig.cmake``, targets)
* ``daslib/`` — standard library ``.das`` files
* ``modules/`` — built-in module directories with ``.das_module`` descriptors
  and ``.shared_module`` shared libraries


Module types
============

There are three types of external modules:

**C++ module** (with shared library)
   Contains C++ code compiled into a ``.shared_module`` DLL.  Functions
   and types are registered through the daScript C++ or C binding API.
   The ``.das_module`` descriptor uses ``register_dynamic_module`` to
   load the shared library.

**Pure daslang module** (no C++ code)
   Implemented entirely in ``.das`` files.  The ``.das_module``
   descriptor uses ``register_native_path`` to map ``require`` paths
   to ``.das`` files on disk.

**Mixed module** (C++ and daslang)
   Contains both a ``.shared_module`` DLL and ``.das`` files.  The
   ``.das_module`` descriptor uses both ``register_dynamic_module``
   and ``register_native_path``.


The ``.das_module`` descriptor
==============================

Every module in the ``modules/`` directory needs a ``.das_module`` file.
This is a daslang script that the dynamic binary executes at startup
to discover the module's components.

The file must:

* Be named exactly ``.das_module`` (dot-prefixed, no other name)
* Live at ``modules/<module_name>/.das_module``
* Export an ``initialize`` function

Basic structure:

.. code-block:: das

   options gen2
   require daslib/fio

   [export]
   def initialize(project_path : string) {
       // registration calls go here
   }

The ``project_path`` argument is the absolute path to the module's
directory (e.g., ``/path/to/modules/myModule``).  Use string
interpolation to build paths: ``"{project_path}/myLib.shared_module"``.


Registration functions
----------------------

``register_dynamic_module(path, class_name)``
   Loads a ``.shared_module`` shared library and registers a C++ module
   class.  The ``class_name`` must match the ``REGISTER_MODULE`` or
   ``REGISTER_DYN_MODULE`` call in the C++ source:

   .. code-block:: das

      register_dynamic_module("{project_path}/myLib.shared_module", "Module_MyMod")

   Guard with ``das_is_dll_build()`` — this function only makes sense
   in the dynamic binary:

   .. code-block:: das

      if (das_is_dll_build()) {
          register_dynamic_module("{project_path}/myLib.shared_module", "Module_MyMod")
      }

``register_native_path(top, from, to)``
   Maps a ``require`` path to a ``.das`` file on disk.  When a script
   says ``require foo/bar``:

   * ``top`` = ``"foo"`` (the module namespace, before the first ``/``)
   * ``from`` = ``"bar"`` (the path after the namespace)
   * ``to`` = absolute path to the ``.das`` file

   .. code-block:: das

      register_native_path("mymod", "utils", "{project_path}/das/utils.das")
      // Now `require mymod/utils` resolves to <project_path>/das/utils.das

   This function does not need a ``das_is_dll_build()`` guard.


Complete examples
-----------------

**C++ module** (one module class):

.. code-block:: das

   options gen2
   require daslib/fio

   [export]
   def initialize(project_path : string) {
       if (das_is_dll_build()) {
           register_dynamic_module("{project_path}/dasModuleUnitTest.shared_module", "Module_UnitTest")
       }
   }

**Pure daslang module** (multiple ``.das`` files):

.. code-block:: das

   options gen2
   require daslib/fio

   [export]
   def initialize(project_path : string) {
       let paths = ["peg", "meta_ast", "parse_macro", "parser_generator"]
       for (path in paths) {
           register_native_path("peg", "{path}", "{project_path}/peg/{path}.das")
       }
   }

**Mixed module** (C++ shared library + daslang files):

.. code-block:: das

   options gen2
   require daslib/fio

   [export]
   def initialize(project_path : string) {
       let daslib_paths = [
           "llvm_boost", "llvm_debug", "llvm_jit", "llvm_targets",
           "llvm_jit_intrin", "llvm_jit_common", "llvm_dll_utils"
       ]
       let bindings_paths = [
           "llvm_const", "llvm_enum", "llvm_func", "llvm_struct"
       ]
       for (path in daslib_paths) {
           register_native_path("llvm", "daslib/{path}", "{project_path}/daslib/{path}.das")
       }
       for (path in bindings_paths) {
           register_native_path("llvm", "bindings/{path}", "{project_path}/bindings/{path}.das")
       }
   }

**Multiple C++ module classes** from one shared library:

.. code-block:: das

   options gen2
   require daslib/fio

   [export]
   def initialize(project_path : string) {
       if (das_is_dll_build()) {
           register_dynamic_module("{project_path}/dasModuleImgui.shared_module", "Module_dasIMGUI")
           register_dynamic_module("{project_path}/dasModuleImgui.shared_module", "Module_dasIMGUI_NODE_EDITOR")
           register_dynamic_module("{project_path}/imguiApp.shared_module", "Module_imgui_app")
       }
   }


Module resolution order
=======================

When the compiler encounters a ``require foo/bar`` statement, it
resolves the module through the following chain (in order):

1. **Parse** the require path: ``top = "foo"``, ``mod_name = "bar"``
2. **daslib** — if ``top == "daslib"``, resolve from the ``daslib/``
   directory
3. **Static modules** — try ``NATIVE_MODULE`` macro matches (compiled
   into the static binary only)
4. **Dynamic modules** — try ``g_dyn_modules_resolve`` entries
   populated by ``.das_module`` scripts via ``register_native_path``
5. **Extra roots** — try the ``extraRoots`` map (set by the host
   application)
6. **dastest** — try the ``dastest`` module name convention


Building a C++ module
=====================

A C++ module is a shared library (``.shared_module``) that links against
``libDaScriptDyn`` and exports a module registration function.

C++ module class
----------------

Create a class that inherits from ``das::Module`` and register
functions, types, and enumerations in the constructor:

.. code-block:: cpp

   #include "daScript/daScript.h"
   #include "daScript/daScriptModule.h"

   const char * hello(const char * name, das::Context * ctx, das::LineInfoArg * at) {
       return ctx->allocateString(das::string("Hello, ") + name + "!", at);
   }

   class Module_Hello : public das::Module {
   public:
       Module_Hello() : Module("HelloModule") {
           das::ModuleLibrary lib(this);
           lib.addBuiltInModule();
           das::addExtern<DAS_BIND_FUN(hello)>(*this, lib,
               "hello", das::SideEffects::none, "hello");
       }
   };

   REGISTER_DYN_MODULE(Module_Hello, Module_Hello);
   REGISTER_MODULE(Module_Hello);

.. note::

   Use both ``REGISTER_DYN_MODULE`` and ``REGISTER_MODULE``.
   ``REGISTER_DYN_MODULE`` provides the exported entry point for the
   dynamic binary.  ``REGISTER_MODULE`` provides compatibility with the
   static binary.

C module (using the C API)
--------------------------

For modules written in C (or any language with C FFI), use the C API
from ``daScript/daScriptC.h``:

.. code-block:: c

   #include "daScript/daScriptC.h"

   vec4f hello_from_c(das_context * ctx, das_node * node, vec4f * args) {
       return das_result_string("Hello from C module!");
   }

   #ifdef _MSC_VER
       #define EXPORT_API __declspec(dllexport)
   #else
       #define EXPORT_API __attribute__((visibility("default")))
   #endif

   EXPORT_API das_module * register_dyn_Module_Hello() {
       das_module * mod = das_module_create("HelloModule");
       das_module_group * lib = das_modulegroup_make();
       das_module_bind_interop_function(mod, lib, &hello_from_c,
           "hello_from_c", "hello_from_c", SIDEEFFECTS_modifyExternal, "s ");
       das_modulegroup_release(lib);
       return mod;
   }

Note that C files must not use ``extern "C"`` wrappers — C linkage is
the default in ``.c`` files.

CMake setup
-----------

External modules use ``find_package(DAS)`` to locate the installed
daslang SDK.  The SDK exports:

* ``DAS::libDaScriptDyn`` — the dynamic runtime library
* ``DAS::libDaScript`` — the static runtime library
* ``DAS::daslang`` — the compiler binary (for AOT generation)

A minimal ``CMakeLists.txt`` for a C++ module:

.. code-block:: cmake

   cmake_minimum_required(VERSION 3.16)
   project(MyModule)

   find_package(DAS REQUIRED)

   add_library(myModule SHARED my_module.cpp)
   target_link_libraries(myModule PRIVATE DAS::libDaScriptDyn)

   # Output as .shared_module in the module directory
   set_target_properties(myModule PROPERTIES
       LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
       RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
       SUFFIX ".shared_module"
       PREFIX ""
   )

The key points are:

* Link against ``DAS::libDaScriptDyn`` (not ``DAS::libDaScript``)
* Set the output suffix to ``.shared_module``
* The shared library should be placed in the module directory (next to
  the ``.das_module`` descriptor)


AOT for external modules
-------------------------

External C++ modules can use ahead-of-time compilation for daslang
helper functions.  Use the installed ``DAS::daslang`` target to run
the AOT compiler:

.. code-block:: cmake

   add_custom_command(
       OUTPUT "helper.aot.cpp"
       DEPENDS DAS::daslang "helper.das"
       WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
       COMMAND DAS::daslang -aotlib
           "${CMAKE_CURRENT_SOURCE_DIR}/helper.das"
           "${CMAKE_CURRENT_BINARY_DIR}/helper.aot.cpp"
   )

   add_library(myModule SHARED my_module.cpp helper.aot.cpp)

Use ``-aotlib`` when AOT-compiling a module helper (generates a linkable
object).  Use ``-aot`` for standalone scripts.

.. note::

   Always use absolute paths (``CMAKE_CURRENT_SOURCE_DIR`` /
   ``CMAKE_CURRENT_BINARY_DIR``) in AOT custom commands.  Relative
   paths fail when the build directory differs from the source directory.


Building a pure daslang module
==============================

Pure daslang modules are the simplest — no C++ compilation is needed.
The module is just a directory of ``.das`` files with a ``.das_module``
descriptor.

Directory layout::

   modules/
     myModule/
       .das_module
       src/
         utils.das
         parser.das

The ``.das_module`` maps ``require`` paths:

.. code-block:: das

   options gen2
   require daslib/fio

   [export]
   def initialize(project_path : string) {
       register_native_path("mymod", "utils", "{project_path}/src/utils.das")
       register_native_path("mymod", "parser", "{project_path}/src/parser.das")
   }

Scripts can then use ``require mymod/utils`` and ``require mymod/parser``.


Using an external module from a host application
=================================================

Once a module is built and its ``.das_module`` descriptor is in place,
the host application needs to tell the daslang runtime where to find
modules.

Dynamic module discovery
------------------------

The dynamic binary discovers modules automatically from the
``modules/`` directory relative to the daslang root.  Call
``require_dynamic_modules`` before ``Module::Initialize()``:

.. code-block:: cpp

   #include "daScript/daScript.h"
   #include "daScript/ast/dyn_modules.h"

   int main() {
       das::TextPrinter tout;
       auto fAccess = das::make_smart<das::FsFileAccess>();

       NEED_ALL_DEFAULT_MODULES
       das::require_dynamic_modules(fAccess, das::getDasRoot(), "./", tout);
       das::Module::Initialize();

       // ... compile and run scripts as normal
   }

``require_dynamic_modules`` scans every directory under ``modules/``
for a ``.das_module`` file and executes it.  The ``getDasRoot()``
function returns the daslang installation root (set via the
``DAS_ROOT`` environment variable or auto-detected).


Installing external modules
============================

For distribution, install the module directory into the SDK's
``modules/`` folder.

If your module is part of the main daScript build tree, use the
``ADD_MODULE_LIB`` / ``ADD_MODULE_DAS`` CMake macros — they handle
both static and dynamic builds automatically.

For standalone modules, add install rules in your ``CMakeLists.txt``:

.. code-block:: cmake

   # Install .shared_module and .das_module to the SDK
   install(TARGETS myModule
       RUNTIME DESTINATION modules/myModule
       LIBRARY DESTINATION modules/myModule
   )
   install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/.das_module
       DESTINATION modules/myModule
   )
   # Install any .das files
   install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/utils.das
       DESTINATION modules/myModule/src
   )

After installation, the module directory should contain at minimum:

* ``.das_module`` — the descriptor
* ``.shared_module`` — the shared library (for C/C++ modules)
* Any ``.das`` files the module provides


Example: dascript-demo
======================

The `dascript-demo <https://github.com/aleksisch/dascript-demo>`_
repository is a complete, standalone example of using the installed
daslang SDK from an external project.  It demonstrates:

* Using ``find_package(DAS)`` to locate the SDK
* Embedding daslang in a C++ host application
* Embedding daslang in a C host application
* Creating external C and C++ modules with ``.das_module`` descriptors
* AOT compilation for external modules

Repository structure::

   dascript-demo/
     CMakeLists.txt          # Top-level: find_package(DAS), host executables
     main.cpp                # C++ host — compiles and runs hello.das
     main-C.c                # C host — same, using the C API
     hello.das               # Test script requiring external modules
     modules/
       moduleHello/          # C module example
         .das_module          # Descriptor (register_dynamic_module + register_native_path)
         hello_module.c       # C source — uses daScriptC.h
         hello_module.das     # Pure-das companion module
         CMakeLists.txt       # Builds hello_Module.mod shared library
       moduleHelloCPP/        # C++ module example with AOT
         .das_module           # Descriptor
         hello_module.cpp      # C++ source — Module class + REGISTER_DYN_MODULE
         hello_module.das      # Helper .das file (AOT-compiled into the module)
         CMakeLists.txt        # Builds shared library + AOT custom command

The top-level ``CMakeLists.txt`` is minimal:

.. code-block:: cmake

   cmake_minimum_required(VERSION 3.16)
   project(das-example)

   find_package(DAS REQUIRED)

   # C++ host
   add_executable(hello_from_das_cpp main.cpp)
   target_link_libraries(hello_from_das_cpp PRIVATE DAS::libDaScriptDyn)

   # C host
   add_executable(hello_from_das_c main-C.c)
   target_link_libraries(hello_from_das_c PRIVATE DAS::libDaScriptDyn)

The C++ host (``main.cpp``) follows the standard pattern — compile,
simulate, evaluate — with the addition of
``require_dynamic_modules`` to discover external modules:

.. code-block:: cpp

   #include "daScript/daScript.h"
   #include "daScript/ast/dyn_modules.h"

   int main() {
       das::TextPrinter tout;
       auto faccess = das::smart_ptr<das::FsFileAccess>(new das::FsFileAccess);

       NEED_ALL_DEFAULT_MODULES
       das::require_dynamic_modules(faccess, das::getDasRoot(), "./", tout);
       das::Module::Initialize();

       // compile, simulate, evaluate hello.das ...
   }

The test script (``hello.das``) requires modules from both external
module directories:

.. code-block:: das

   options gen2
   require Hello/hello_module
   require Hello/hello_module_cpp
   require HelloModule
   require HelloCPP

   [export]
   def main() {
       print("Hello world!\n")
       print("{hello_from_das()}\n")       // from das module
       print("{hello_from_c_module()}\n")  // from C module
       print("{hello_from_cpp_module("Daslang")}\n")  // from C++ module
   }


In-tree modules vs external modules
====================================

If your module is part of the main daScript repository (under
``modules/``), use the built-in CMake macros:

``ADD_MODULE_LIB(libName, dllName, sources...)``
   Creates both a static library (``libName``) for the static binary
   and a ``.shared_module`` DLL (``dllName``) for the dynamic binary.
   Both are built automatically.

``ADD_MODULE_CPP(ClassName)``
   Registers the module class in ``external_need.inc`` so the static
   binary includes it via ``NEED_MODULE``.

``ADD_MODULE_DAS(category, subfolder, native)``
   Registers a pure-das module in ``external_resolve.inc`` for static
   binary resolution.

These macros handle the dual static/dynamic build automatically.  You
still need to write a ``.das_module`` descriptor for the dynamic build.

For modules outside the daScript tree, use ``find_package(DAS)``
and link against ``DAS::libDaScriptDyn`` as shown above.


.. seealso::

   * :ref:`utils_daspkg` -- daspkg package manager (automated install, update, build)
   * :ref:`embedding_modules` -- C++ API reference for module bindings
   * :ref:`aot` -- AOT compilation details
   * :ref:`tutorial_integration_cpp_hello_world` -- step-by-step C++ host tutorial
   * `dascript-demo repository <https://github.com/aleksisch/dascript-demo>`_ -- complete external project example
