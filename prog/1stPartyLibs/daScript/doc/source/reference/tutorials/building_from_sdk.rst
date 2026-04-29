.. _tutorial_building_from_sdk:

.. index::
   single: Tutorial; Building from SDK
   single: Tutorial; CMake; find_package
   single: Tutorial; SDK; Installation
   single: Tutorial; getDasRoot

============================================
 Building Projects Against the Installed SDK
============================================

This guide explains how to build your own C or C++ applications that embed
daslang, using the installed SDK rather than building from the full source
tree.  It also covers building the integration tutorials from the SDK.


.. _building_from_sdk_prerequisites:

Prerequisites
=============

* A daslang SDK installed via ``cmake --install``.  The default layout is::

     <sdk-root>/
       bin/             daslang.exe, libDaScript.dll
       include/         daScript.h, daScriptC.h, etc.
       lib/
         cmake/DAS/     DASConfig.cmake, DASTargets.cmake
       daslib/          Standard library modules
       utils/           AOT tool scripts
       tutorials/
         integration/
           cpp/         C++ tutorial sources + CMakeLists.txt
           c/           C tutorial sources + CMakeLists.txt

* CMake 3.16 or later.
* A C++17 compiler (MSVC 2019+, GCC 9+, Clang 10+).


.. _building_from_sdk_find_package:

Using ``find_package(DAS)``
===========================

The SDK ships a CMake package configuration that provides imported targets.
In your ``CMakeLists.txt``:

.. code-block:: cmake

   cmake_minimum_required(VERSION 3.16)
   project(my_daslang_app CXX)

   find_package(DAS REQUIRED)
   find_package(Threads REQUIRED)

Point CMake to the SDK root when configuring:

.. code-block:: bash

   cmake -DCMAKE_PREFIX_PATH=/path/to/daslang ..

``find_package(DAS)`` sets the following:

=========================  ==============================================
Variable / Target          Description
=========================  ==============================================
``DAS::libDaScriptDyn``    Shared library (``libDaScript.dll`` / ``.so``)
``DAS::libDaScript``       Static library
``DAS::daslang``           The ``daslang`` executable (for AOT codegen)
``DAS_VERSION``            SDK version string (e.g. ``0.6.0``)
``DAS_DIR``                Path to ``lib/cmake/DAS/``
=========================  ==============================================

.. tip::

   Always link against ``DAS::libDaScriptDyn`` (the shared library) for
   tutorial and application builds.  This avoids having to rebuild the entire
   engine and keeps binary size small.


.. _building_from_sdk_minimal:

A minimal project
=================

.. code-block:: cmake

   cmake_minimum_required(VERSION 3.16)
   project(hello_daslang CXX)

   find_package(DAS REQUIRED)
   find_package(Threads REQUIRED)

   # Derive SDK root from the DAS package location.
   get_filename_component(DAS_SDK_ROOT "${DAS_DIR}/../../.." ABSOLUTE)

   # Place executables in <sdk>/bin/ so getDasRoot() resolves correctly.
   set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${DAS_SDK_ROOT}/bin")

   add_executable(hello_daslang main.cpp)
   target_compile_definitions(hello_daslang PRIVATE DAS_MOD_EXPORTS)
   target_link_libraries(hello_daslang PRIVATE DAS::libDaScriptDyn Threads::Threads)
   target_compile_features(hello_daslang PRIVATE cxx_std_17)

Key points:

``DAS_MOD_EXPORTS``
   Required preprocessor definition — ensures correct DLL import/export
   declarations on Windows.

``Threads::Threads``
   Required on all platforms — daslang uses threading internally.

``CMAKE_RUNTIME_OUTPUT_DIRECTORY``
   Set to ``<sdk>/bin/`` so the executable lands next to ``daslang.exe``.
   This is important because ``getDasRoot()`` auto-detects the SDK root by
   walking up from the executable's location, looking for a ``bin/`` parent
   directory.  If your executable is in a different location, ``getDasRoot()``
   will return the wrong path and the runtime will fail to find ``daslib/``,
   scripts, and other SDK resources.

For C programs, the pattern is similar — use ``target_link_libraries`` with
``DAS::libDaScriptDyn`` and set ``LINKER_LANGUAGE CXX`` since the daslang
runtime is C++:

.. code-block:: cmake

   add_executable(hello_daslang_c main.c)
   target_compile_definitions(hello_daslang_c PRIVATE DAS_MOD_EXPORTS)
   target_link_libraries(hello_daslang_c PRIVATE DAS::libDaScriptDyn Threads::Threads)
   set_target_properties(hello_daslang_c PROPERTIES
       LINKER_LANGUAGE CXX
       CXX_STANDARD 17
   )


.. _building_from_sdk_getdasroot:

Understanding ``getDasRoot()``
==============================

``getDasRoot()`` is a function that returns the root directory of the
daslang SDK as a string.  The integration tutorials use it to build paths to
``.das`` scripts:

.. code-block:: cpp

   auto program = compileDaScript(getDasRoot() + "/tutorials/integration/cpp/01_hello_world.das",
                                  fAccess, tout, dummyLibGroup);

Internally, ``getDasRoot()`` locates the directory of the running executable,
then walks up the path looking for a ``bin/`` parent.  When it finds one, it
returns the parent of ``bin/`` as the SDK root.

This means your executable **must** be placed in ``<sdk>/bin/`` for
``getDasRoot()`` to work correctly.  The recommended CMake pattern is:

.. code-block:: cmake

   get_filename_component(DAS_SDK_ROOT "${DAS_DIR}/../../.." ABSOLUTE)
   set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${DAS_SDK_ROOT}/bin")


.. _building_from_sdk_aot:

Adding AOT compilation
======================

AOT (Ahead-of-Time) compilation translates daslang functions into C++ source
code at build time for near-native performance.  The SDK includes the AOT
tool scripts in ``utils/aot/main.das``.

To add AOT to your CMake project, define a macro that runs ``daslang`` as a
build tool:

.. code-block:: cmake

   # Derive SDK root
   get_filename_component(DAS_SDK_ROOT "${DAS_DIR}/../../.." ABSOLUTE)

   macro(MY_AOT_GENERATE input out_var target_name)
       get_filename_component(_abs "${input}" ABSOLUTE)
       get_filename_component(_name "${input}" NAME)
       set(_out_dir "${CMAKE_CURRENT_BINARY_DIR}/_aot_generated")
       set(_out_src "${_out_dir}/${target_name}_${_name}.cpp")
       file(MAKE_DIRECTORY "${_out_dir}")
       add_custom_command(
           OUTPUT  "${_out_src}"
           DEPENDS "${_abs}"
           COMMENT "AOT: ${_name}"
           COMMAND $<TARGET_FILE:DAS::daslang>
                   "${DAS_SDK_ROOT}/utils/aot/main.das"
                   -- -aot "${_abs}" "${_out_src}"
       )
       set(${out_var} "${_out_src}")
       set_source_files_properties("${_out_src}" PROPERTIES GENERATED TRUE)
   endmacro()

Then use it:

.. code-block:: cmake

   set(AOT_SRC)
   MY_AOT_GENERATE("my_script.das" AOT_SRC my_app)

   add_executable(my_app main.cpp "${AOT_SRC}")
   target_compile_definitions(my_app PRIVATE DAS_MOD_EXPORTS)
   target_link_libraries(my_app PRIVATE DAS::libDaScriptDyn Threads::Threads)
   target_compile_features(my_app PRIVATE cxx_std_17)

The command invoked is::

   daslang.exe <sdk>/utils/aot/main.das -- -aot my_script.das output.cpp

The ``-aot`` flag generates a ``.cpp`` file with C++ implementations of all
daslang functions plus self-registration code.  At runtime, when
``CodeOfPolicies::aot`` is set to ``true``, ``simulate()`` links the
AOT-compiled functions automatically — no additional setup needed.

For standalone-context generation (``-ctx`` mode), see
:ref:`tutorial_integration_cpp_standalone_contexts`.


.. _building_from_sdk_tutorials:

Building the integration tutorials
===================================

The SDK ships with ready-to-use ``CMakeLists.txt`` files for all integration
tutorials.  After installing, you can build them directly from the SDK.


C++ tutorials
-------------

.. code-block:: bash

   mkdir build_cpp && cd build_cpp
   cmake -DCMAKE_PREFIX_PATH=/path/to/daslang /path/to/daslang/tutorials/integration/cpp
   cmake --build . --config Release

This builds all 22 C++ integration tutorials.  The executables are placed
in ``<sdk>/bin/`` and can be run directly from there.

On Windows with a typical install to ``D:\daslang``:

.. code-block:: powershell

   mkdir build_cpp; cd build_cpp
   cmake -DCMAKE_PREFIX_PATH=D:\daslang D:\daslang\tutorials\integration\cpp
   cmake --build . --config Release
   D:\daslang\bin\integration_cpp_01.exe

C tutorials
-----------

.. code-block:: bash

   mkdir build_c && cd build_c
   cmake -DCMAKE_PREFIX_PATH=/path/to/daslang /path/to/daslang/tutorials/integration/c
   cmake --build . --config Release

This builds all 10 C integration tutorials.  On Windows:

.. code-block:: powershell

   mkdir build_c; cd build_c
   cmake -DCMAKE_PREFIX_PATH=D:\daslang D:\daslang\tutorials\integration\c
   cmake --build . --config Release
   D:\daslang\bin\integration_c_01.exe


Individual targets
------------------

You can build a single tutorial by specifying its target name::

   cmake --build . --config Release --target integration_cpp_01

Target names follow the pattern ``integration_cpp_NN`` and
``integration_c_NN``.


AOT tutorials
-------------

The AOT tutorials (C++ tutorial 13, C tutorial 09) are included in the
standalone builds.  CMake automatically runs ``daslang`` to generate the
AOT C++ source from the ``.das`` script during the build.  No additional
steps are needed — just build as shown above.

For details on how AOT is integrated in CMake, see
:ref:`tutorial_integration_cpp_aot` and :ref:`tutorial_integration_c_aot`.


.. _building_from_sdk_targets:

Available imported targets
==========================

The following CMake targets are available after ``find_package(DAS REQUIRED)``:

``DAS::libDaScriptDyn``
   The daslang shared library.  Use this for application builds.
   Includes all necessary include directories and compile definitions
   transitively — you do not need to add ``target_include_directories``
   manually.

``DAS::libDaScript``
   The daslang static library.  Links the entire engine statically.
   Produces larger binaries but removes the DLL dependency.

``DAS::daslang``
   The ``daslang`` command-line tool.  Use it in ``add_custom_command``
   for AOT code generation.  Access via ``$<TARGET_FILE:DAS::daslang>``.


.. _building_from_sdk_troubleshooting:

Troubleshooting
===============

"Could not find a package configuration file provided by DAS"
   Ensure ``CMAKE_PREFIX_PATH`` points to the SDK root (the directory
   containing ``lib/cmake/DAS/``).

Scripts not found at runtime
   Make sure the executable is in ``<sdk>/bin/``.  Set
   ``CMAKE_RUNTIME_OUTPUT_DIRECTORY`` to ``${DAS_SDK_ROOT}/bin``.
   If you cannot place the executable there, you must modify the source
   to set the DAS root explicitly instead of relying on ``getDasRoot()``.

AOT header not found (``dag_noise/dag_uint_noise.h``, ``module_builtin_*.h``)
   These headers are installed under ``<sdk>/include/``.  If your AOT-generated
   code includes them and the compiler cannot find them, verify that
   ``DAS::libDaScriptDyn`` is listed in ``target_link_libraries`` — it
   provides the include directories transitively.

Linker errors on Windows
   Make sure you define ``DAS_MOD_EXPORTS`` on your target:
   ``target_compile_definitions(my_app PRIVATE DAS_MOD_EXPORTS)``.

Multi-config generators (Visual Studio)
   If executables end up in ``bin/Release/`` instead of ``bin/``, set the
   per-config output directory explicitly:

   .. code-block:: cmake

      foreach(cfg DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
          set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${cfg} "${DAS_SDK_ROOT}/bin")
      endforeach()


.. seealso::

   :ref:`tutorial_integration_cpp_hello_world` — your first C++ daslang program

   :ref:`tutorial_integration_c_hello_world` — your first C daslang program

   :ref:`tutorial_integration_cpp_aot` — AOT compilation in depth (C++)

   :ref:`tutorial_integration_c_aot` — AOT compilation in depth (C)

