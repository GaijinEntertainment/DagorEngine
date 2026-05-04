.. _tutorial_integration_cpp_sandbox:

.. index::
   single: Tutorial; C++ Integration; Sandbox

=====================================
 C++ Integration: Sandbox
=====================================

This tutorial shows how to restrict what daslang scripts can do,
which is essential when running untrusted code (user mods, plugin
systems, online playgrounds).  Two complementary approaches are shown:

* **Approach A — C++**: ``CodeOfPolicies`` + ``FileAccess`` subclass
* **Approach B — .das_project**: sandbox policy written in daslang

Topics covered:

* ``CodeOfPolicies`` — compile-time language restrictions
* Memory limits — heap and stack size caps
* Custom ``FileAccess`` — restrict which modules scripts can require
* ``.das_project`` — policy file with exported callback functions


Prerequisites
=============

* Tutorial 15 completed (:ref:`tutorial_integration_cpp_custom_annotations`).


Sandboxing approaches
=====================

daslang provides two complementary approaches:

**Approach A — C++ code** (Demos 1–4):

1. ``CodeOfPolicies`` — language-level restrictions passed to
   ``compileDaScript()``
2. ``FileAccess`` virtual overrides — fine-grained C++ control over
   modules, options, annotations

**Approach B — .das_project file** (Demos 5–6):

3. A ``.das_project`` file — a regular daslang script that exports
   callback functions.  The host loads it, and the compiler calls
   the callbacks during compilation of user scripts.

Both can be combined — ``CodeOfPolicies`` is layered on top of
either ``FileAccess`` subclass or ``.das_project`` callbacks.


CodeOfPolicies — language restrictions
========================================

Pass a ``CodeOfPolicies`` struct to ``compileDaScript()``:

.. code-block:: cpp

   CodeOfPolicies policies;
   policies.no_unsafe = true;                   // forbid unsafe blocks
   policies.no_global_variables = true;          // forbid module-scope var
   policies.no_init = true;                      // forbid [init] functions
   policies.max_heap_allocated = 1024 * 1024;    // 1 MB heap max
   policies.max_string_heap_allocated = 256*1024; // 256 KB strings
   policies.stack = 8 * 1024;                    // 8 KB stack

   auto program = compileDaScript(script, fAccess, tout,
                                  libGroup, policies);

Key policy flags:

+-----------------------------------+------------------------------------------+
| ``no_unsafe``                     | Forbids all ``unsafe`` blocks            |
+-----------------------------------+------------------------------------------+
| ``no_global_variables``           | Forbids module-scope ``var``             |
+-----------------------------------+------------------------------------------+
| ``no_global_heap``                | Forbids heap allocations from globals    |
+-----------------------------------+------------------------------------------+
| ``no_init``                       | Forbids ``[init]`` functions             |
+-----------------------------------+------------------------------------------+
| ``max_heap_allocated``            | Caps heap memory (0 = unlimited)         |
+-----------------------------------+------------------------------------------+
| ``max_string_heap_allocated``     | Caps string heap memory                  |
+-----------------------------------+------------------------------------------+
| ``stack``                         | Context stack size in bytes              |
+-----------------------------------+------------------------------------------+
| ``threadlock_context``            | Adds context mutex for thread safety     |
+-----------------------------------+------------------------------------------+

When a policy is violated, compilation fails with error ``40207``.


Custom FileAccess — module restrictions
==========================================

Subclass ``FsFileAccess`` and override virtual methods to control
what scripts can access:

.. code-block:: cpp

   class SandboxFileAccess : public FsFileAccess {
       das_set<string> allowedModules;
   public:
       SandboxFileAccess() {
           allowedModules.insert("$");      // built-in (always needed)
           allowedModules.insert("math");
           allowedModules.insert("strings");
       }

       bool isModuleAllowed(const string & mod,
                            const string &) const override {
           return allowedModules.count(mod) > 0;
       }

       bool canModuleBeUnsafe(const string &,
                              const string &) const override {
           return false;  // no module can use unsafe
       }

       bool isOptionAllowed(const string & opt,
                            const string &) const override {
           return opt != "persistent_heap";
       }
   };

Available virtual overrides:

+---------------------------+-----------------------------------------+
| ``isModuleAllowed()``     | Can this module be loaded at all?       |
+---------------------------+-----------------------------------------+
| ``canModuleBeUnsafe()``   | Can this module use ``unsafe``?         |
+---------------------------+-----------------------------------------+
| ``canBeRequired()``       | Can this module be ``require``'d?       |
+---------------------------+-----------------------------------------+
| ``isOptionAllowed()``     | Can this ``options`` keyword be used?   |
+---------------------------+-----------------------------------------+
| ``isAnnotationAllowed()`` | Can this annotation be used?            |
+---------------------------+-----------------------------------------+

All return ``true`` by default (no restrictions).


.das_project — policy in daslang
=================================

Instead of writing C++ code, you can define sandbox policies in a
``.das_project`` file — a regular ``.das`` script that exports
callback functions.  The compiler loads it, simulates it, and calls
the exported functions during compilation of user scripts.

Loading a project file from C++
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   string projectPath = getDasRoot()
       + "/path/to/sandbox.das_project";
   auto fAccess = make_smart<FsFileAccess>(
       projectPath, make_smart<FsFileAccess>());

   CodeOfPolicies policies;
   auto program = compileDaScript(scriptPath, fAccess, tout,
                                  libGroup, policies);

The ``FsFileAccess(projectPath, fallbackAccess)`` constructor
compiles and simulates the project file, then looks up exported
callback functions by name.

Project file callbacks
~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: das

   options gen2
   require strings
   require daslib/strings_boost

   typedef module_info = tuple<string; string; string> const
   var DAS_PAK_ROOT = "./"

   // REQUIRED — resolve every `require X` to a file path
   [export]
   def module_get(req, from : string) : module_info {
       // return (moduleName, fileName, importName)
   }

   // Whitelist which modules can be loaded
   [export]
   def module_allowed(mod, filename : string) : bool {
       if (mod == "$" || mod == "math" || mod == "strings") {
           return true
       }
       return false
   }

   // No module may use unsafe
   [export]
   def module_allowed_unsafe(mod, filename : string) : bool {
       return false
   }

   // Only safe options
   [export]
   def option_allowed(opt, from : string) : bool {
       return opt == "gen2" || opt == "indenting"
   }

   // Only safe annotations
   [export]
   def annotation_allowed(ann, from : string) : bool {
       return ann == "export" || ann == "private"
   }

Available callbacks:

+-------------------------------+-----------------------------------------------+
| ``module_get``                | Resolve ``require`` paths (REQUIRED)          |
+-------------------------------+-----------------------------------------------+
| ``module_allowed``            | Whitelist which modules can load              |
+-------------------------------+-----------------------------------------------+
| ``module_allowed_unsafe``     | Control ``unsafe`` per module                 |
+-------------------------------+-----------------------------------------------+
| ``option_allowed``            | Whitelist ``options`` directives              |
+-------------------------------+-----------------------------------------------+
| ``annotation_allowed``        | Whitelist annotations                         |
+-------------------------------+-----------------------------------------------+
| ``include_get``               | Resolve ``include`` directives (optional)     |
+-------------------------------+-----------------------------------------------+

``DAS_PAK_ROOT`` is set by the runtime to the directory containing
the ``.das_project`` file, useful for resolving relative paths.


C++ vs .das_project — when to use which
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

* **C++ subclass** — when you need dynamic logic, access to host
  state, or tight integration with your engine's asset pipeline
* **.das_project** — when you want to ship policy as data alongside
  scripts, let level designers tweak restrictions, or prototype
  policies without recompiling the host
* **Both combined** — ``CodeOfPolicies`` plus a ``.das_project``;
  the C++ policies add a hard floor, the project file refines what's
  allowed within that floor


Demo walkthrough
================

The tutorial runs six demos:

**Approach A — C++:**

1. **No restrictions** — normal compilation succeeds
2. **``no_unsafe`` policy** — a script with ``unsafe`` blocks is
   rejected; a safe script compiles fine
3. **Memory limits** — stack and heap sizes are capped
4. **Module restrictions** — ``SandboxFileAccess`` blocks modules
   not in the allow-list; ``canModuleBeUnsafe() = false`` prevents
   transitive dependencies from using unsafe

**Approach B — .das_project:**

5. **.das_project sandbox** — the safe script compiles under the
   project's policy callbacks
6. **.das_project blocks violations** — unsafe scripts and blocked
   modules are rejected by the project's callbacks


Building and running
====================

::

   cmake --build build --config Release --target integration_cpp_16
   bin\Release\integration_cpp_16.exe

Expected output::

   === Demo 1: No restrictions ===
   --- Normal mode ---
   === Sandbox Tutorial ===
   score = 42
   sum(1..10) = 55
   Hello from the sandbox!

   === Demo 2: Policies — no_unsafe ===
   --- Safe script with no_unsafe ---
   === Sandbox Tutorial ===
   ...

   --- Unsafe script with no_unsafe ---
     Compilation FAILED (expected in sandbox demo):
     error[40207]: unsafe function test
     ...

   === Demo 3: Memory limits ===
     Stack:       8192 bytes
     Max heap:    1048576 bytes
     Max strings: 262144 bytes
   --- Memory-limited ---
   === Sandbox Tutorial ===
   ...

   === Demo 4: Module restrictions ===
   --- Allowed script ---
   === Sandbox Tutorial ===
   ...

   --- Blocked module script ---
     Compilation FAILED (expected in sandbox demo):
     ...

   === Demo 5: .das_project sandbox ===
   --- Script under .das_project sandbox ---
   === Sandbox Tutorial ===
   score = 42
   sum(1..10) = 55
   Hello from the sandbox!

   === Demo 6: .das_project blocks violations ===
   --- Unsafe script under .das_project ---
     Compilation FAILED (expected in sandbox demo):
     error[40207]: unsafe function test
     ...

   --- Blocked module under .das_project ---
     Compilation FAILED (expected in sandbox demo):
     ...

   === Available sandbox approaches ===
   ...


.. seealso::

   Full source:
   :download:`16_sandbox.cpp <../../../../tutorials/integration/cpp/16_sandbox.cpp>`,
   :download:`16_sandbox.das <../../../../tutorials/integration/cpp/16_sandbox.das>`,
   :download:`16_sandbox.das_project <../../../../tutorials/integration/cpp/16_sandbox.das_project>`

   Previous tutorial: :ref:`tutorial_integration_cpp_custom_annotations`

   Next tutorial: :ref:`tutorial_integration_cpp_coroutines`
