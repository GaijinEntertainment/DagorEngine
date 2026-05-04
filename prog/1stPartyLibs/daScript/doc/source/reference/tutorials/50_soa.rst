.. _tutorial_soa:

==========================================
Structure-of-Arrays (SOA)
==========================================

.. index::
    single: Tutorial; SOA
    single: Tutorial; Structure-of-Arrays
    single: Tutorial; Particles
    single: Tutorial; from_array
    single: Tutorial; to_array

This tutorial covers ``daslib/soa`` — a compile-time macro that transforms
regular structs into a Structure-of-Arrays layout.  The ``[soa]`` annotation
generates parallel arrays for every field, plus all the container operations
you need (push, erase, pop, clear, resize, reserve, swap, from_array,
to_array).

Prerequisites: familiarity with structs and arrays.

.. code-block:: das

    options gen2
    options no_unused_function_arguments = false

    require daslib/soa


What is SOA?
=============

Normally a struct is stored as Array-of-Structures (AOS)::

    [ {x,y,z,w}, {x,y,z,w}, {x,y,z,w}, ... ]

Structure-of-Arrays (SOA) rearranges this into parallel arrays::

    xs: [x, x, x, ...]
    ys: [y, y, y, ...]
    ...

This is friendlier to CPU caches when iterating over a single field
(e.g. updating positions), because the data is contiguous.

The ``[soa]`` annotation automates this transformation.  For a struct
``Particle`` with fields ``pos``, ``vel``, ``life``, ``color``, it
generates ``Particle`SOA`` where each field is an ``array<FieldType>``.


Basic SOA
==========

Annotate a struct with ``[soa]`` and the macro generates the SOA layout
plus all access functions:

.. code-block:: das

    [soa]
    struct Particle {
        pos   : float3
        vel   : float3
        life  : float
        color : float4
    }

Declare and use the SOA container:

.. code-block:: das

    def demo_basic_soa() {
        print("=== basic SOA ===\n")
        var particles : Particle`SOA

        particles |> push <| Particle(
            pos = float3(0.0), vel = float3(1.0, 0.0, 0.0),
            life = 3.0, color = float4(1.0, 0.0, 0.0, 1.0))
        particles |> push <| Particle(
            pos = float3(5.0), vel = float3(0.0, 1.0, 0.0),
            life = 5.0, color = float4(0.0, 1.0, 0.0, 1.0))
        particles |> push <| Particle(
            pos = float3(10.0), vel = float3(0.0, 0.0, 1.0),
            life = 2.0, color = float4(0.0, 0.0, 1.0, 1.0))
        print("  count: {length(particles)}\n")

        // Indexed access — macro rewrites soa[i].field to soa.field[i]
        print("  particles[0].pos  = {particles[0].pos}\n")
        print("  particles[1].life = {particles[1].life}\n")
        print("  particles[2].vel  = {particles[2].vel}\n")
    }


Iteration
==========

The ``SoaForLoop`` macro transforms ``for (it in soa)`` into a
multi-source loop over the individual column arrays.  Only the fields
you actually access are iterated:

.. code-block:: das

    def demo_iteration() {
        print("\n=== iteration ===\n")
        var particles : Particle`SOA
        for (i in range(4)) {
            particles |> push <| Particle(
                pos = float3(float(i)), life = float(4 - i))
        }

        for (it in particles) {
            print("    pos={it.pos} life={it.life}\n")
        }

        // Mixed iteration with an index counter
        for (idx, it in count(), particles) {
            print("    [{idx}] pos={it.pos}\n")
        }
    }


Container operations
=====================

``push``, ``push_clone``, ``emplace``, ``erase``, ``pop``, and ``clear``
all work the same as on regular arrays:

.. code-block:: das

    def demo_container_ops() {
        print("\n=== container operations ===\n")
        var soa : Particle`SOA

        // push — move semantics
        soa |> push <| Particle(pos = float3(1.0), life = 10.0)
        soa |> push <| Particle(pos = float3(2.0), life = 20.0)
        soa |> push <| Particle(pos = float3(3.0), life = 30.0)

        // push_clone — copy from a const value
        let p = Particle(pos = float3(4.0), life = 40.0)
        soa |> push_clone(p)

        // erase — remove by index
        soa |> erase(1)

        // pop — remove last element
        soa |> pop()

        // clear — remove all elements
        soa |> clear()
    }


Sizing — resize, reserve, capacity
====================================

Pre-allocate memory with ``reserve``, query with ``capacity``, and
change the element count with ``resize``:

.. code-block:: das

    def demo_sizing() {
        print("\n=== sizing ===\n")
        var soa : Particle`SOA

        soa |> reserve(100)
        print("  capacity={capacity(soa)}\n")

        for (i in range(10)) {
            soa |> push <| Particle(
                pos = float3(float(i)), life = float(i))
        }

        // resize — truncate or extend with defaults
        soa |> resize(5)
        soa |> resize(8)
    }


Swap and sorting
=================

``swap`` exchanges all fields of two elements at once.  Combine it with
any sorting algorithm:

.. code-block:: das

    [soa]
    struct SortItem {
        key : int
        tag : string
    }

    def demo_swap_and_sort() {
        var soa : SortItem`SOA
        soa |> push <| SortItem(key = 30, tag = "gamma")
        soa |> push <| SortItem(key = 10, tag = "alpha")
        soa |> push <| SortItem(key = 20, tag = "beta")

        soa |> swap(0, 2)

        // Bubble sort
        let n = length(soa)
        for (pass_idx in range(n)) {
            for (k in range(n - 1)) {
                if (soa[k].key > soa[k + 1].key) {
                    soa |> swap(k, k + 1)
                }
            }
        }
    }


Bulk conversion — from_array, to_array
========================================

Convert between AOS (``array<T>``) and SOA (``T`SOA``) layouts:

.. code-block:: das

    [soa]
    struct Vec2 {
        x : float
        y : float
    }

    def demo_conversion() {
        var arr : array<Vec2>
        arr |> push <| Vec2(x = 1.0, y = 2.0)
        arr |> push <| Vec2(x = 3.0, y = 4.0)
        arr |> push <| Vec2(x = 5.0, y = 6.0)

        // AOS → SOA
        var soa : Vec2`SOA
        soa |> from_array(arr)

        // SOA → AOS
        var result <- to_array(soa)
    }


Particle simulation
====================

A complete example: spawn particles, simulate physics, remove dead ones:

.. code-block:: das

    def demo_particle_sim() {
        var particles : Particle`SOA
        particles |> reserve(100)

        for (i in range(5)) {
            let fi = float(i)
            particles |> push <| Particle(
                pos   = float3(fi * 2.0, 0.0, 0.0),
                vel   = float3(0.0, 1.0 + fi * 0.5, 0.0),
                life  = 3.0 + fi,
                color = float4(fi / 4.0, 1.0 - fi / 4.0, 0.5, 1.0)
            )
        }

        let dt = 1.0
        for (step in range(3)) {
            for (it in particles) {
                it.pos += it.vel * dt
                it.life -= dt
            }
            // Remove dead — iterate backwards
            var i = length(particles) - 1
            while (i >= 0) {
                if (particles[i].life <= 0.0) {
                    particles |> erase(i)
                }
                i --
            }
        }
    }


Game entity table
==================

SOA works well for game entity tables with mixed field types:

.. code-block:: das

    [soa]
    struct GameEntity {
        id     : int
        name   : string
        health : float
        alive  : bool
    }

    def demo_entity_table() {
        var entities : GameEntity`SOA
        entities |> push <| GameEntity(
            id = 1, name = "warrior", health = 100.0, alive = true)
        entities |> push <| GameEntity(
            id = 2, name = "mage",    health = 60.0,  alive = true)
        entities |> push <| GameEntity(
            id = 3, name = "archer",  health = 80.0,  alive = true)

        // Apply damage
        for (it in entities) {
            it.health -= 55.0
            if (it.health <= 0.0) {
                it.alive = false
            }
        }

        // Convert to AOS for serialization
        var snapshot <- to_array(entities)
    }


Full source
============

The complete tutorial source is in
``tutorials/language/50_soa.das``.

Run it with::

    daslang.exe tutorials/language/50_soa.das


.. seealso::

   Full source: :download:`tutorials/language/50_soa.das <../../../../tutorials/language/50_soa.das>`

   :ref:`stdlib_soa` — SOA module reference.

   Previous tutorial: :ref:`tutorial_async`

   Next tutorial: :ref:`tutorial_delegate`
