.. _tutorial_decs:

==========================================
Entity Component System (DECS)
==========================================

.. index::
    single: Tutorial; DECS
    single: Tutorial; Entity Component System
    single: Tutorial; ECS
    single: Tutorial; Entities
    single: Tutorial; Components
    single: Tutorial; Archetypes
    single: Tutorial; Queries
    single: Tutorial; Stages

This tutorial covers the ``decs`` module — daslang's built-in Entity Component
System. DECS provides a lightweight ECS where entities are identified by
``EntityId``, carry dynamically typed components, and are grouped into
archetypes based on their component sets. All mutations are deferred and
applied on ``commit()``.

.. code-block:: das

    require daslib/decs_boost

Core concepts
=============

* **Entity** — an ``EntityId`` with associated component data.
* **Component** — a named, typed value attached to an entity (set via ``:=``
  on a ``ComponentMap``).
* **Archetype** — a storage bucket for entities with the same set of component
  names. Created automatically.
* **Deferred execution** — ``create_entity``, ``delete_entity``, and
  ``update_entity`` are all deferred; call ``commit()`` to apply them.

Creating entities
=================

``create_entity`` takes a block that receives the ``EntityId`` and a
``ComponentMap``. Use ``:=`` to set components:

.. code-block:: das

    let player = create_entity() @(eid, cmp) {
        cmp.name := "hero"
        cmp.hp   := 100
        cmp.pos  := float3(0, 0, 0)
    }
    commit()  // entity becomes visible

Querying entities
=================

Global queries iterate all entities matching the listed component types.
Component names in the block signature match component names on entities:

.. code-block:: das

    query() $(name : string; hp : int; pos : float3) {
        print("  {name}: hp={hp} pos={pos}\n")
    }

Query a specific entity by passing its ``EntityId``:

.. code-block:: das

    query(eid) $(tag : string; val : int) {
        print("Found: tag={tag} val={val}\n")
    }

Mutable queries
===============

Use ``var`` and ``&`` to modify components in place:

.. code-block:: das

    query() $(var pos : float3&; vel : float3) {
        pos += vel
    }

REQUIRE and REQUIRE_NOT
=======================

Filter queries to entities that have (or lack) specific components, even when
you don't need those components as block arguments:

.. code-block:: das

    // Only entities WITH a "weapon" component
    query <| $ [REQUIRE(weapon)] (name : string; hp : int) {
        print("  {name} hp={hp}\n")
    }

    // Exclude entities WITH a "shield" component
    query <| $ [REQUIRE_NOT(shield)] (name : string) {
        print("  {name}\n")
    }

find_query
==========

``find_query`` stops iteration when the block returns ``true``, providing
an early-exit search:

.. code-block:: das

    let found = find_query() $(idx : int) {
        if (idx == 7) {
            return true
        }
        return false
    }

Deleting entities
=================

``delete_entity`` is deferred until ``commit()``:

.. code-block:: das

    delete_entity(eid)
    commit()

Updating entities
=================

``update_entity`` lets you modify, add, or remove components. If the component
set changes, the entity moves to a different archetype:

.. code-block:: das

    update_entity(eid) @(eid, cmp) {
        var hp = 0
        hp = get(cmp, "hp", hp)
        cmp |> set("hp", hp - 25)
        cmp.enraged := true
    }
    commit()

    // Remove a component
    update_entity(eid) @(eid, cmp) {
        cmp |> remove("enraged")
    }
    commit()

Default values in queries
=========================

If an entity lacks a queried component, the default value is used. Parameters
with defaults must be ``const`` (no ``var``, no ``&``):

.. code-block:: das

    query() $(name : string; alpha : float = 0.5) {
        print("  {name}: alpha={alpha}\n")
    }

Templates
=========

``[decs_template]`` structs map struct fields to components with an automatic
prefix (``StructName_`` by default). This generates ``apply_decs_template``
and ``remove_decs_template`` functions:

.. code-block:: das

    [decs_template]
    struct Particle {
        pos  : float3
        vel  : float3
        life : int
    }

    // Create entity with template
    create_entity() @(eid, cmp) {
        apply_decs_template(cmp, Particle(
            pos  = float3(0, 0, 0),
            vel  = float3(1, 0, 0),
            life = 100
        ))
    }

    // Query using template struct
    query() $(var p : Particle) {
        p.pos += p.vel
        p.life -= 1
    }

Stage functions
===============

Stage functions are annotated with ``[decs(stage=name)]`` and become queries
that run when you call ``decs_stage("name")``. The stage commits automatically
before and after running all registered functions:

.. code-block:: das

    [decs(stage = simulate)]
    def simulate_particles(var p : Particle) {
        p.pos += p.vel
        p.life -= 1
    }

    // Run 3 simulation steps
    for (step in range(3)) {
        decs_stage("simulate")
    }

Nested queries
==============

Queries can be nested — the inner query sees all matching entities:

.. code-block:: das

    query() $(name : string) {
        var sum = 0
        query() $(val : int) {
            sum += val
        }
        print("  {name} sees total val={sum}\n")
    }

Serialization
=============

The entire ECS state can be saved and restored using the ``archive`` module:

.. code-block:: das

    require daslib/archive

    var data <- mem_archive_save(decsState)
    restart()
    mem_archive_load(data, decsState)

Entity ID recycling
===================

When an entity is deleted, its ID slot is recycled. The generation counter
increments so that stale ``EntityId`` values cannot accidentally access the
new entity occupying the same slot:

.. code-block:: das

    let eid1 = create_entity() @(eid, cmp) {
        cmp.val := 1
    }
    commit()
    delete_entity(eid1)
    commit()

    let eid2 = create_entity() @(eid, cmp) {
        cmp.val := 2
    }
    commit()
    // eid2.id == eid1.id but eid2.generation != eid1.generation

Archetype inspection
====================

You can inspect the world state through ``decsState``:

.. code-block:: das

    print("Archetypes: {length(decsState.allArchetypes)}\n")
    for (arch in decsState.allArchetypes) {
        print("  {arch.size} entities, components:")
        for (c in arch.components) {
            print(" {c.name}")
        }
        print("\n")
    }

Bulk entity creation
====================

``create_entities`` creates many entities at once, much faster than calling
``create_entity`` in a loop. Entities are immediately visible — no ``commit()``
needed:

.. code-block:: das

    create_entities(100) <| $(eid : EntityId; i : int; var cmp : ComponentMap) {
        apply_decs_template(cmp, Particle(
            pos  = float3(float(i), 0, 0),
            vel  = float3(0, 1, 0),
            life = 100
        ))
    }

High-performance bulk creation
==============================

For maximum performance, ``create_entities`T`` bypasses ``ComponentMap``
entirely. The ``[decs_template]`` macro generates a type-specific bulk
creation function that writes directly into archetype storage:

.. code-block:: das

    create_entities`Particle(1000) <| $(eid : EntityId; i : int; var p : Particle) {
        p.pos  = float3(float(i), 0.0, 0.0)
        p.vel  = float3(0, 1, 0)
        p.life = 100
    }

The block receives the ``EntityId``, index ``i``, and a mutable struct
instance. Just set the fields you need — the macro handles the archetype
writes internally. This is ~7x faster than the ``ComponentMap``-based path
at scale (1000+ entities).

Utility functions
=================

``is_alive`` checks whether an ``EntityId`` still refers to a living entity.
It returns ``false`` for ``INVALID_ENTITY_ID``, deleted entities, and stale
generation IDs after slot recycling:

.. code-block:: das

    print("alive? {is_alive(hero)}\n")        // true
    delete_entity(hero)
    commit()
    print("alive? {is_alive(hero)}\n")        // false

``entity_count`` returns the total number of alive entities across all
archetypes:

.. code-block:: das

    print("entity count: {entity_count()}\n")

``get_component`` retrieves a single component value by entity ID and name.
The type is inferred from the default value parameter. If the entity is dead
or the component is not present, the default is returned:

.. code-block:: das

    let hp = get_component(hero, "hp", 0)            // returns int
    let pos = get_component(hero, "pos", float3(0))   // returns float3
    let missing = get_component(hero, "shield", -1)    // returns -1 (not found)
    let dead = get_component(deleted_eid, "hp", -999)  // returns -999 (dead)

.. seealso::

   Full source: :download:`tutorials/language/34_decs.das <../../../../tutorials/language/34_decs.das>`

   Previous tutorial: :ref:`tutorial_algorithm`

   Next tutorial: :ref:`tutorial_jobque`

   :ref:`Structures <structs>` — struct language reference.

   :ref:`Iterators <iterators>` — iterator language reference.

   :ref:`Lambda <lambdas>` — lambda language reference.
