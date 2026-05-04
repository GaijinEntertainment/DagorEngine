.. _tutorial_integration_cpp_custom_annotations:

.. index::
   single: Tutorial; C++ Integration; Custom Annotations

============================================
 C++ Integration: Custom Annotations
============================================

This tutorial shows how to create custom annotations in C++ that
modify daslang compilation behavior.  Topics covered:

* ``FunctionAnnotation`` — hooks into function compilation
* ``StructureAnnotation`` — hooks into struct compilation
* ``apply()`` / ``finalize()`` — the function annotation lifecycle
* ``touch()`` / ``look()`` — the structure annotation lifecycle
* Adding fields to structs at compile time


Prerequisites
=============

* Tutorial 14 completed (:ref:`tutorial_integration_cpp_serialization`).


What are annotations?
=====================

Annotations are compile-time hooks defined in C++ and used from
daslang with the ``[annotation_name]`` syntax.  They let the host
application validate, modify, or transform script code during
compilation.

Built-in examples: ``[export]``, ``[private]``, ``[deprecated]``.
This tutorial shows how to create your own.


Annotation class hierarchy
==========================

.. code-block:: text

   Annotation
   ├── FunctionAnnotation      — [name] on functions
   │   └── MarkFunctionAnnotation  — convenience base
   ├── StructureAnnotation     — [name] on structs
   └── TypeAnnotation          — describes C++ types
       └── ManagedStructureAnnotation<T>

``TypeAnnotation`` / ``ManagedStructureAnnotation`` describe **how a
C++ type is exposed** (fields, size, simulation nodes).
``StructureAnnotation`` is a **compile-time hook** on daslang
``struct`` declarations — they are unrelated concepts.


FunctionAnnotation — ``[log_calls]``
======================================

A ``FunctionAnnotation`` is called during compilation whenever the
annotated function is parsed and type-checked:

.. code-block:: cpp

   struct LogCallsAnnotation : FunctionAnnotation {
       LogCallsAnnotation() : FunctionAnnotation("log_calls") {}

       // Called during parsing — can modify function flags
       bool apply(const FunctionPtr & func, ModuleGroup &,
                  const AnnotationArgumentList &, string &) override {
           printf("[log_calls] apply: %s\n", func->name.c_str());
           return true;  // true = success, false = error
       }

       // Not supported on blocks
       bool apply(ExprBlock *, ModuleGroup &,
                  const AnnotationArgumentList &,
                  string & err) override {
           err = "not supported for blocks";
           return false;
       }

       // Called after type inference
       bool finalize(const FunctionPtr & func, ModuleGroup &,
                     const AnnotationArgumentList &,
                     const AnnotationArgumentList &,
                     string &) override {
           printf("[log_calls] finalize: %s (args: %d)\n",
                  func->name.c_str(), (int)func->arguments.size());
           return true;
       }

       bool finalize(ExprBlock *, ModuleGroup &,
                     const AnnotationArgumentList &,
                     const AnnotationArgumentList &,
                     string &) override { return true; }
   };

Key virtual methods:

+-----------------------+-------------------------------------------+
| ``apply()``           | Parse time — modify flags, validate       |
+-----------------------+-------------------------------------------+
| ``finalize()``        | After inference — validate with types     |
+-----------------------+-------------------------------------------+
| ``verifyCall()``      | Each call site — check arguments          |
+-----------------------+-------------------------------------------+
| ``transformCall()``   | Inference — rewrite call AST              |
+-----------------------+-------------------------------------------+
| ``simulate()``        | Simulation — return custom SimNode        |
+-----------------------+-------------------------------------------+


StructureAnnotation — ``[add_field]``
==========================================

A ``StructureAnnotation`` is called during compilation of annotated
structs.  The key hook is ``touch()``, which runs **before** type
inference and can modify the struct:

.. code-block:: cpp

   struct AddFieldAnnotation : StructureAnnotation {
       AddFieldAnnotation() : StructureAnnotation("add_field") {}

       // Called BEFORE type inference — can modify struct
       bool touch(const StructurePtr & st, ModuleGroup &,
                  const AnnotationArgumentList &, string &) override {
           if (!st->findField("id")) {
               st->fields.emplace_back(
                   "id",                             // name
                   make_smart<TypeDecl>(Type::tInt),  // type
                   nullptr,                          // no init
                   AnnotationArgumentList(),         // no ann
                   false,                            // no move
                   LineInfo()                        // loc
               );
               // Must invalidate the lookup cache!
               st->fieldLookup.clear();
           }
           return true;
       }

       // Called AFTER type inference — read-only validation
       bool look(const StructurePtr & st, ModuleGroup &,
                 const AnnotationArgumentList &,
                 string &) override {
           printf("struct '%s' has %d fields\n",
                  st->name.c_str(), (int)st->fields.size());
           return true;
       }
   };

Key points:

* Push to ``st->fields`` directly — there is no ``addField`` method
* **Must call** ``st->fieldLookup.clear()`` after adding fields
* Guard with ``findField()`` to avoid duplicates (``touch`` can be
  called multiple times)
* ``touch`` → before inference (can modify)
* ``look`` → after inference (read-only)
* ``patch`` → after inference (can request re-inference)


Registration
============

Register annotations in the module constructor:

.. code-block:: cpp

   addAnnotation(make_smart<LogCallsAnnotation>());
   addAnnotation(make_smart<AddFieldAnnotation>());


Using from daslang
====================

.. code-block:: das

   options gen2
   require tutorial_15_cpp

   [log_calls]
   def attack(target : string; damage : int) {
       print("  {target} takes {damage} damage!\n")
   }

   [add_field]
   struct Monster {
       name : string
       hp   : int
   }

   [export]
   def test() {
       attack("Goblin", 25)
       // Monster now has an "id" field added by [add_field]
       var m = Monster(name = "Dragon", hp = 500, id = 42)
       print("{m.name}: id={m.id}\n")


Building and running
====================

::

   cmake --build build --config Release --target integration_cpp_15
   bin\Release\integration_cpp_15.exe

Expected output::

   --- Compilation (annotation hooks fire here) ---
     [log_calls] apply:    attack
     [log_calls] apply:    heal
     [add_field] added 'id : int' to struct 'Monster'
     [add_field] look: struct 'Monster' has 3 fields
     [log_calls] finalize: attack (args: 2)
     [log_calls] finalize: heal (args: 2)

   --- Running script ---
   === Custom Annotations Tutorial ===

   --- Annotated functions ---
     Goblin takes 25 damage!
     Hero heals for 10 hp

   --- Normal function ---
     (this function has no annotation)

   --- Annotated struct ---
     Dragon: hp=500, id=42

Note how the annotation hooks fire during **compilation**, before
the script runs.


.. seealso::

   Full source:
   :download:`15_custom_annotations.cpp <../../../../tutorials/integration/cpp/15_custom_annotations.cpp>`,
   :download:`15_custom_annotations.das <../../../../tutorials/integration/cpp/15_custom_annotations.das>`

   Previous tutorial: :ref:`tutorial_integration_cpp_serialization`

   Next tutorial: :ref:`tutorial_integration_cpp_sandbox`
