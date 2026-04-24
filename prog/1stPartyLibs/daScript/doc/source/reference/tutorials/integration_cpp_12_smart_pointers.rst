.. _tutorial_integration_cpp_smart_pointers:

.. index::
   single: Tutorial; C++ Integration; Smart Pointers

=======================================
 C++ Integration: Smart Pointers
=======================================

This tutorial shows how to expose reference-counted C++ types to
daslang using ``smart_ptr<T>``.  Topics covered:

* Inheriting from ``das::ptr_ref_count`` for reference counting
* ``ManagedStructureAnnotation`` with ``canNew`` / ``canDelete``
* ``var inscope`` — automatic cleanup of smart pointers
* Factory functions returning ``smart_ptr<T>``
* ``smart_ptr_clone`` and ``smart_ptr_use_count``
* ``new T`` for heap allocation from scripts


Prerequisites
=============

* Tutorial 11 completed (:ref:`tutorial_integration_cpp_context_variables`).
* Understanding of reference counting concepts.


Making a type reference-counted
==================================

A C++ type becomes smart-pointer-compatible by inheriting from
``das::ptr_ref_count``, which provides ``addRef()``, ``delRef()``,
and ``use_count()``:

.. code-block:: cpp

   #include "daScript/daScript.h"

   class Entity : public das::ptr_ref_count {
   public:
       das::string name;
       float x, y;
       int32_t health;

       Entity() : name("unnamed"), x(0), y(0), health(100) {
           printf("  Entity constructed\n");
       }
       ~Entity() {
           printf("  Entity destroyed\n");
       }
       // ... methods ...
   };

When ``delRef()`` decrements the count to zero, the object is
automatically deleted.


Annotation — ``canNew`` and ``canDelete``
============================================

``ManagedStructureAnnotation`` takes two boolean template parameters
that control what scripts can do:

.. code-block:: cpp

   struct EntityAnnotation
       : ManagedStructureAnnotation<Entity, true, true>
       //                                  ^^^^  ^^^^
       //                         canNew ---+      |
       //                         canDelete -------+
   {
       EntityAnnotation(ModuleLibrary & ml)
           : ManagedStructureAnnotation("Entity", ml)
       {
           addField<DAS_BIND_MANAGED_FIELD(name)>("name", "name");
           addField<DAS_BIND_MANAGED_FIELD(x)>("x", "x");
           addField<DAS_BIND_MANAGED_FIELD(y)>("y", "y");
           addField<DAS_BIND_MANAGED_FIELD(health)>("health", "health");
           addProperty<DAS_BIND_MANAGED_PROP(is_alive)>(
               "is_alive", "is_alive");
       }
   };

``isSmart()`` is auto-detected — ``ManagedStructureAnnotation`` checks
``is_base_of<ptr_ref_count, Entity>`` at compile time.

+---------------+--------------------------------------------------+
| ``canNew``    | ``new Entity`` allocates + ``addRef()``          |
+---------------+--------------------------------------------------+
| ``canDelete`` | ``delete ptr`` calls ``delRef()``                |
+---------------+--------------------------------------------------+


Factory returning ``smart_ptr<T>``
====================================

The typical pattern is a factory function that returns ``smart_ptr``:

.. code-block:: cpp

   smart_ptr<Entity> make_entity(const char * name, float x, float y) {
       auto e = make_smart<Entity>();
       e->name = name;
       e->x = x;
       e->y = y;
       return e;
   }

   // Registration:
   addExtern<DAS_BIND_FUN(make_entity)>(*this, lib, "make_entity",
       SideEffects::modifyExternal, "make_entity")
           ->args({"name", "x", "y"});


Using smart pointers in daslang
==================================

Smart pointer variables must be declared with ``var inscope``, which
ensures ``delRef()`` is called when the variable goes out of scope:

.. code-block:: das

   options gen2
   require tutorial_12_cpp

   [export]
   def test() {
       // Factory returns smart_ptr<Entity> — use <- to move
       var inscope hero <- make_entity("Hero", 0.0, 0.0)
       print("{hero.name} hp={hero.health}\n")

       // Dereference with * when passing to functions taking Entity &
       move_entity(*hero, 3.0, 4.0)

       // Clone increases reference count
       var inscope hero2 : smart_ptr<Entity>
       smart_ptr_clone(hero2, hero)
       print("use_count = {int(smart_ptr_use_count(hero))}\n")  // 2

       // `new Entity` — heap allocation (requires unsafe)
       unsafe {
           var inscope fresh <- new Entity
           fresh.health = 50
       }
       // fresh destroyed here (delRef → ref_count==0 → delete)

Key points:

* ``var inscope`` — required for ``smart_ptr`` variables
* ``<-`` — move semantics (not ``=``)
* ``*ptr`` — dereference to get ``Entity &`` for function calls
* ``smart_ptr_clone(dest, src)`` — clone (addRef)
* ``smart_ptr_use_count(ptr)`` — returns ``uint`` (cast to ``int`` for decimal printing)
* ``new Entity`` — allocates + addRef (needs ``unsafe``)


Building and running
====================

::

   cmake --build build --config Release --target integration_cpp_12
   bin\Release\integration_cpp_12.exe

Expected output::

   === Factory-created entities ===
     [C++] Entity('unnamed') constructed
     [C++] Entity('unnamed') constructed
   hero:  Hero at (0, 0) hp=100
   enemy: Goblin at (10, 5) hp=100
   use_count(hero)  = 1
   use_count(enemy) = 1

   === Moving and combat ===
   hero moved to (3, 4)
   distance = 7.071068
   enemy hp after 30 damage = 70
   enemy.is_alive = true
   enemy hp after 100 more  = 0
   enemy.is_alive = false

   === Cloning smart_ptr ===
   After clone:
     use_count(hero)  = 2
     use_count(hero2) = 2
     hero2.name = Hero

   === new Entity() ===
     [C++] Entity('unnamed') constructed
   fresh.name   = Newbie
   fresh.health = 50
   use_count    = 1
     [C++] Entity('Newbie') destroyed

   === End of test (hero, enemy, hero2 destroyed here) ===
     [C++] Entity('Goblin') destroyed
     [C++] Entity('Hero') destroyed


.. seealso::

   Full source:
   :download:`12_smart_pointers.cpp <../../../../tutorials/integration/cpp/12_smart_pointers.cpp>`,
   :download:`12_smart_pointers.das <../../../../tutorials/integration/cpp/12_smart_pointers.das>`

   Previous tutorial: :ref:`tutorial_integration_cpp_context_variables`

   Next tutorial: :ref:`tutorial_integration_cpp_aot`