.. _tutorial_integration_c_sandbox:

.. index::
   single: Tutorial; C Integration; Sandbox
   single: Tutorial; C Integration; das_project
   single: Tutorial; C Integration; File Access Lock
   single: Tutorial; C Integration; Policy Enforcement
   single: Tutorial; C Integration; CodeOfPolicies

=============================================
 C Integration: Sandbox
=============================================

This tutorial demonstrates how to run daslang code in a **sandboxed
environment** from a C host.  The sandbox has two independent layers:

.. list-table::
   :header-rows: 1
   :widths: 20 40 40

   * - Layer
     - Controls
     - API
   * - **Filesystem lock**
     - Which files exist (physical)
     - ``das_fileaccess_introduce_*``, ``das_fileaccess_lock``
   * - **Policy (.das_project)**
     - What is allowed among existing files
     - ``das_fileaccess_make_project``, callback functions


Filesystem locking
==================

The filesystem lock prevents the compiler from accessing any file that
was not **pre-introduced** into the file access cache:

.. code-block:: c

   das_file_access * fa = das_fileaccess_make_project(project_path);

   // Pre-cache all standard library files
   das_fileaccess_introduce_daslib(fa);

   // Introduce user scripts as virtual files (inline C strings)
   das_fileaccess_introduce_file(fa, "helpers.das", HELPER_SOURCE, 0);
   das_fileaccess_introduce_file(fa, "script.das", SCRIPT_SOURCE, 0);

   // Lock — getNewFileInfo() now returns null for uncached files
   das_fileaccess_lock(fa);

The ``0`` in ``das_fileaccess_introduce_file`` means the file access
**borrows** the string pointer (no copy) — suitable for static C string
constants.  Pass ``1`` for dynamically allocated strings that should be
owned and freed by the file access.

Available ``introduce`` functions:

============================================  =================================
Function                                      Purpose
============================================  =================================
``das_fileaccess_introduce_file``              Register a virtual file from a string
``das_fileaccess_introduce_file_from_disk``    Read a disk file into cache
``das_fileaccess_introduce_daslib``            Cache all ``daslib/*.das`` files
``das_fileaccess_introduce_native_modules``    Cache all native plugin modules
``das_fileaccess_introduce_native_module``     Cache a single native module
============================================  =================================


The .das_project file
=====================

A ``.das_project`` is a regular daslang file that the compiler loads
**before** compiling user scripts.  It exports ``[export]`` callback
functions that the compiler invokes during compilation.

The project file is loaded via ``das_fileaccess_make_project``:

.. code-block:: c

   das_file_access * fa = das_fileaccess_make_project(
       "path/to/project.das_project");

Internally, the constructor:

1. Compiles the ``.das_project`` file as a normal daslang program
2. Simulates it in a fresh context
3. Looks up ``[export]`` functions by name (``module_get``,
   ``module_allowed``, etc.)
4. Runs ``[init]`` functions
5. Sets the ``DAS_PAK_ROOT`` variable to the project file's directory

The project file itself is **not** subject to sandbox restrictions —
it compiles with full filesystem access.  Restrictions only apply to
**subsequent** user script compilations.


Callback reference
==================

The compiler recognizes these exported callback function names:

.. list-table::
   :header-rows: 1
   :widths: 30 10 60

   * - Function
     - Required
     - Purpose
   * - ``module_get(req, from : string) : module_info``
     - **Yes**
     - Resolve ``require`` paths to ``(moduleName, fileName, importName)``
   * - ``module_allowed(mod, filename : string) : bool``
     - No
     - Whitelist which modules can be loaded
   * - ``module_allowed_unsafe(mod, filename : string) : bool``
     - No
     - Control whether ``unsafe`` blocks are permitted
   * - ``option_allowed(opt, from : string) : bool``
     - No
     - Whitelist which ``options`` directives are accepted
   * - ``annotation_allowed(ann, from : string) : bool``
     - No
     - Whitelist which annotations are accepted
   * - ``can_module_be_required(mod, fname : string; isPublic : bool) : bool``
     - No
     - Fine-grained control over ``require`` statements
   * - ``is_pod_in_scope_allowed(moduleName, fileName : string) : bool``
     - No
     - Control ``var inscope`` for POD types
   * - ``include_get(inc, from : string) : string``
     - No
     - Resolve ``include`` directives
   * - ``is_same_file_name(a, b : string) : bool``
     - No
     - Custom file path comparison
   * - ``dyn_modules_folder() : string``
     - No
     - Set dynamic module search folder

``module_info`` is defined as ``tuple<string; string; string> const`` —
the three fields are ``(moduleName, fileName, importName)``.

Callbacks that are not exported fall back to permissive defaults
(everything allowed).  If ``module_get`` is missing, the project is
treated as failed and all callbacks revert to defaults.

This tutorial demonstrates the five most impactful callbacks.


module_get — path resolution
----------------------------

When a ``.das_project`` is active, the built-in ``daslib/`` resolution
logic is **completely bypassed** — ``module_get`` is the sole path
resolver.  It must handle ``daslib/X`` paths as well as relative module
paths:

.. code-block:: das

    [export]
    def module_get(req, from : string) : module_info {
        let rs <- split_by_chars(req, "./")
        let mod_name = rs[length(rs) - 1]
        if (length(rs) == 2 && rs[0] == "daslib") {
            return (mod_name, "{get_das_root()}/daslib/{mod_name}.das", "")
        }
        // Resolve relative to the requiring file
        var fr <- split_by_chars(from, "/")
        if (length(fr) > 0) { pop(fr) }
        for (se in rs) { push(fr, se) }
        return (mod_name, join(fr, "/") + ".das", "")
    }


module_allowed — module whitelist
---------------------------------

Called for **every** module (native C++ and compiled ``.das``) that is
loaded during compilation.  The ``mod`` parameter is the short module
name — for example ``"strings"``, ``"fio"``, ``"sandbox_helpers"`` —
not the ``require`` path.

The built-in core module ``"$"`` must always be whitelisted:

.. code-block:: das

    [export]
    def module_allowed(mod, filename : string) : bool {
        if (mod == "$") { return true }
        if (mod == "math" || mod == "strings") { return true }
        if (mod == "strings_boost" || mod == "sandbox_helpers") { return true }
        return false
    }

Returning ``false`` produces a ``module not allowed`` compilation error.


module_allowed_unsafe — forbid unsafe
--------------------------------------

Called once per compiled module.  Returning ``false`` forbids all
``unsafe`` blocks in that module:

.. code-block:: das

    [export]
    def module_allowed_unsafe(mod, filename : string) : bool {
        return false   // no module may use unsafe
    }


option_allowed — restrict options
---------------------------------

Called during parsing for each ``options`` declaration.  Returning
``false`` rejects the option with an ``invalid option`` error:

.. code-block:: das

    [export]
    def option_allowed(opt, from : string) : bool {
        if (opt == "gen2" || opt == "indenting") { return true }
        return false
    }


annotation_allowed — restrict annotations
------------------------------------------

Called for each annotation used in the script:

.. code-block:: das

    [export]
    def annotation_allowed(ann, from : string) : bool {
        if (ann == "export" || ann == "private") { return true }
        return false
    }


The C host code
===============

The tutorial embeds four inline daslang scripts as C string constants:

- **HELPER_MODULE** — a utility module (``sandbox_helpers``) with two
  simple functions (``clamp_value`` and ``greet``)
- **VALID_SCRIPT** — requires allowed modules, calls helper functions
- **VIOLATES_OPTION** — uses ``options persistent_heap`` (banned)
- **VIOLATES_UNSAFE** — uses an ``unsafe`` block (forbidden)
- **VIOLATES_MODULE** — requires ``fio`` (not whitelisted)


Setting up the sandbox
----------------------

.. code-block:: c

   // 1. Load the .das_project
   das_file_access * fa = das_fileaccess_make_project(project_path);

   // 2. Pre-cache daslib (needed for module path resolution)
   das_fileaccess_introduce_daslib(fa);

   // 3. Introduce user files
   das_fileaccess_introduce_file(fa, "sandbox_helpers.das", HELPER_MODULE, 0);
   das_fileaccess_introduce_file(fa, "user_script.das", VALID_SCRIPT, 0);

   // 4. Lock filesystem
   das_fileaccess_lock(fa);

   // 5. Compile — only cached files accessible, policy enforced
   das_program * program = das_program_compile("user_script.das",
                                                fa, tout, module_group);


Reporting violations
--------------------

When policy violations occur, the compiler reports them as errors.
Iterate ``das_program_get_error`` and format with ``das_error_report``:

.. code-block:: c

   int err_count = das_program_err_count(program);
   for (int i = 0; i < err_count; i++) {
       das_error * error = das_program_get_error(program, i);
       char buf[1024];
       das_error_report(error, buf, sizeof(buf));
       printf("  error %d: %s\n", i, buf);
   }


Building and running
====================

Build with CMake::

   cmake --build build --config Release --target integration_c_06

Run::

   bin/Release/integration_c_06

Expected output (Part 1 — valid script)::

   === Part 1: Valid script in sandbox ===

   clamped: 100
   Hello, sandbox!
   split: [[ a; b; c]]

Expected output (Part 2 — banned option)::

   === Part 2: Banned option ===

   Compilation produced 1 error(s):
     error 0: option persistent_heap is not allowed here ...

Expected output (Part 3 — unsafe forbidden)::

   === Part 3: Unsafe block forbidden ===

   Compilation produced 1 error(s):
     error 0: unsafe function test ...

Expected output (Part 4 — banned module)::

   === Part 4: Banned module ===

   Compilation produced 1 error(s):
     error 0: module not allowed 'fio' ...

Expected output (Part 5 — CodeOfPolicies via C API)::

   === Part 5: CodeOfPolicies via C API ===

   Compilation with DAS_POLICY_NO_UNSAFE produced 1 error(s):
     error 0: unsafe function test ...

Part 5 uses ``das_policies_make`` and ``das_policies_set_bool`` to set
``DAS_POLICY_NO_UNSAFE`` directly from C, without a ``.das_project``
file.  The error is identical to the one produced by the project-based
sandbox in Part 3.


.. seealso::

   Full source:
   :download:`06_sandbox.c <../../../../tutorials/integration/c/06_sandbox.c>`,
   :download:`06_sandbox.das_project <../../../../tutorials/integration/c/06_sandbox.das_project>`

   Previous tutorial: :ref:`tutorial_integration_c_unaligned_advanced`

   Next tutorial: :ref:`tutorial_integration_c_context_variables`

   :ref:`type_mangling` — complete type mangling reference

   daScriptC.h API header: ``include/daScript/daScriptC.h``
