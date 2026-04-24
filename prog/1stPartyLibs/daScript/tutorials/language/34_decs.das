// Tutorial 34: Entity Component System (DECS)
//
// This tutorial covers:
//   - Creating entities with dynamic components
//   - Querying entities by component composition
//   - Modifying and deleting entities
//   - Using decs_template structs for structured components
//   - REQUIRE and REQUIRE_NOT filters
//   - find_query for early-exit searches
//   - update_entity to add/remove/modify components
//   - Default and optional component values in queries
//   - Stage functions for game-loop-style processing
//   - Serialization and deserialization of ECS state
//   - Utility functions: is_alive, entity_count, get_component
//
// requires: daslib/decs_boost
// Run: daslang.exe tutorials/language/34_decs.das

options gen2
options persistent_heap

require daslib/decs_boost
require daslib/archive

// === What is DECS? ===

// DECS is daslang's built-in Entity Component System.
// - An "entity" is just an ID (EntityId) with associated data.
// - "Components" are named, typed values attached to entities.
// - Entities with the same set of component names share an "archetype".
// - You iterate entities using "queries" that match component sets.
// All mutations (create, delete, update) are deferred until `commit()`.

// === Creating entities ===

def creating_entities() {
    print("\n=== Creating entities ===\n")

    restart()  // clear ECS world

    // create_entity takes a block that receives the entity ID and a ComponentMap.
    // The `:=` operator sets a component by name and value.
    let player = create_entity() @(eid, cmp) {
        cmp.name := "hero"
        cmp.hp   := 100
        cmp.pos  := float3(0, 0, 0)
    }
    let enemy = create_entity() @(eid, cmp) {
        cmp.name  := "goblin"
        cmp.hp    := 30
        cmp.pos   := float3(10, 0, 5)
        cmp.loot  := "gold"
    }

    // Entities are not visible until commit()
    commit()

    print("Created player={player.id} enemy={enemy.id}\n")
    // output: Created player=0x0 enemy=0x1
}

// === Querying entities ===

def querying_entities() {
    print("\n=== Querying entities ===\n")

    // A global query iterates ALL entities that have the listed components.
    // Component names in the block arguments match component names of entities.
    print("All entities with hp and name:\n")
    query() $(name : string; hp : int; pos : float3) {
        print("  {name}: hp={hp} pos={pos}\n")
    }
    // output:
    //   hero: hp=100 pos=0,0,0
    //   goblin: hp=30 pos=10,0,5
}

// === Query by EntityId ===

def query_by_eid() {
    print("\n=== Query by EntityId ===\n")

    restart()
    let eid = create_entity() @(eid, cmp) {
        cmp.tag := "target"
        cmp.val := 42
    }
    commit()

    // Query a specific entity by passing its EntityId
    query(eid) $(tag : string; val : int) {
        print("Found entity: tag={tag} val={val}\n")
    }
    // output: Found entity: tag=target val=42
}

// === Mutable queries ===

def mutable_queries() {
    print("\n=== Mutable queries ===\n")

    restart()
    for (i in range(3)) {
        create_entity() @(eid, cmp) {
            cmp.pos := float3(float(i), 0, 0)
            cmp.vel := float3(1, 0, 0)
        }
    }
    commit()

    // Use `var` and `&` to mutate components in-place.
    query() $(var pos : float3&; vel : float3) {
        pos += vel
    }

    print("After moving:\n")
    query() $(pos : float3) {
        print("  pos={pos}\n")
    }
    // output:
    //   pos=1,0,0
    //   pos=2,0,0
    //   pos=3,0,0
}

// === REQUIRE and REQUIRE_NOT ===

def require_filters() {
    print("\n=== REQUIRE / REQUIRE_NOT ===\n")

    restart()
    create_entity() @(eid, cmp) {
        cmp.name := "warrior"
        cmp.hp   := 100
        cmp.weapon := "sword"
    }
    create_entity() @(eid, cmp) {
        cmp.name := "mage"
        cmp.hp   := 60
    }
    create_entity() @(eid, cmp) {
        cmp.name := "paladin"
        cmp.hp   := 120
        cmp.weapon := "mace"
        cmp.shield := true
    }
    commit()

    // REQUIRE(weapon): only entities WITH a "weapon" component
    print("Armed units:\n")
    query <| $ [REQUIRE(weapon)] (name : string; hp : int) {
        print("  {name} hp={hp}\n")
    }
    // output:
    //   warrior hp=100
    //   paladin hp=120

    // REQUIRE_NOT(shield): exclude entities WITH "shield"
    print("Without shield:\n")
    query <| $ [REQUIRE_NOT(shield)] (name : string) {
        print("  {name}\n")
    }
    // output:
    //   warrior
    //   mage
}

// === find_query ===

def find_query_demo() {
    print("\n=== find_query ===\n")

    restart()
    for (i in range(10)) {
        create_entity() @(eid, cmp) {
            cmp.idx := i
        }
    }
    commit()

    // find_query stops iteration as soon as the block returns true.
    let found = find_query() $(idx : int) {
        if (idx == 7) {
            print("Found idx=7!\n")
            return true
        }
        return false
    }
    print("find_query returned: {found}\n")
    // output:
    //   Found idx=7!
    //   find_query returned: true
}

// === Deleting entities ===

def deleting_entities() {
    print("\n=== Deleting entities ===\n")

    restart()
    var eids : array<EntityId>
    eids |> reserve(5)
    for (i in range(5)) {
        let eid = create_entity() @(eid, cmp) {
            cmp.idx := i
        }
        eids |> push(eid)
    }
    commit()

    // Delete is deferred — entity is removed on commit
    delete_entity(eids[1])
    delete_entity(eids[3])
    commit()

    print("Remaining entities:\n")
    query() $(idx : int) {
        print("  idx={idx}\n")
    }
    // output:
    //   idx=0
    //   idx=4
    //   idx=2
}

// === Updating entities ===

def updating_entities() {
    print("\n=== Updating entities ===\n")

    restart()
    let eid = create_entity() @(eid, cmp) {
        cmp.hp := 100
        cmp.name := "boss"
    }
    commit()

    // update_entity lets you modify, add, or remove components.
    // If components change, the entity moves to a new archetype.
    update_entity(eid) @(eid, cmp) {
        // Modify existing component
        var hp = 0
        hp = get(cmp, "hp", hp)
        cmp |> set("hp", hp - 25)
        // Add a new component
        cmp.enraged := true
    }
    commit()

    query(eid) $(hp : int; name : string; enraged : bool) {
        print("{name}: hp={hp} enraged={enraged}\n")
    }
    // output: boss: hp=75 enraged=true

    // Remove a component via update
    update_entity(eid) @(eid, cmp) {
        cmp |> remove("enraged")
    }
    commit()

    var stillEnraged = false
    query(eid) $(enraged : bool) {
        stillEnraged = true
    }
    print("Still enraged: {stillEnraged}\n")
    // output: Still enraged: false
}

// === Default values in queries ===

def default_value_queries() {
    print("\n=== Default values ===\n")

    restart()
    create_entity() @(eid, cmp) {
        cmp.name := "visible"
        cmp.alpha := 1.0
    }
    create_entity() @(eid, cmp) {
        cmp.name := "default"
    }
    commit()

    // If an entity lacks a queried component, it uses the default value.
    // Default-valued parameters must be `const` (no `var`, no `&`).
    query() $(name : string; alpha : float = 0.5) {
        print("  {name}: alpha={alpha}\n")
    }
    // output:
    //   visible: alpha=1
    //   default: alpha=0.5
}

// === Templates ===

[decs_template]
struct Particle {
    pos  : float3
    vel  : float3
    life : int
}

def template_demo() {
    print("\n=== decs_template ===\n")

    restart()
    // apply_decs_template adds all struct fields as components
    // with a "StructName_" prefix by default.
    for (i in range(3)) {
        create_entity() @(eid, cmp) {
            apply_decs_template(cmp, Particle(
                pos  = float3(float(i), 0, 0),
                vel  = float3(1, 0, 0),
                life = 100
            ))
        }
    }
    commit()

    // Query using the struct type — compiler expands to individual components
    query() $(var p : Particle) {
        p.pos += p.vel
        p.life -= 1
    }

    print("After update:\n")
    query() $(p : Particle) {
        print("  pos={p.pos} life={p.life}\n")
    }
    // output:
    //   pos=1,0,0 life=99
    //   pos=2,0,0 life=99
    //   pos=3,0,0 life=99
}

// === Stage functions ===

// Stage functions are annotated with [decs(stage=name)].
// They become queries that run when you call decs_stage("name").
// decs_stage commits before and after running all registered functions.

[decs(stage = simulate)]
def simulate_particles(var p : Particle) {
    p.pos += p.vel
    p.life -= 1
}

def stage_demo() {
    print("\n=== Stages ===\n")

    restart()
    create_entity() @(eid, cmp) {
        apply_decs_template(cmp, Particle(
            pos  = float3(0, 0, 0),
            vel  = float3(0, 1, 0),
            life = 10
        ))
    }
    commit()

    // Run 3 simulation steps
    for (step in range(3)) {
        decs_stage("simulate")
    }

    query() $(p : Particle) {
        print("After 3 steps: pos={p.pos} life={p.life}\n")
    }
    // output: After 3 steps: pos=0,3,0 life=7
}

// === Nested queries ===

def nested_query_demo() {
    print("\n=== Nested queries ===\n")

    restart()
    create_entity() @(eid, cmp) {
        cmp.name := "A"
        cmp.val  := 10
    }
    create_entity() @(eid, cmp) {
        cmp.name := "B"
        cmp.val  := 20
    }
    commit()

    // Queries can be nested — inner query sees all matching entities
    query() $(name : string) {
        var sum = 0
        query() $(val : int) {
            sum += val
        }
        print("  {name} sees total val={sum}\n")
    }
    // output:
    //   A sees total val=30
    //   B sees total val=30
}

// === Serialization ===

def serialization_demo() {
    print("\n=== Serialization ===\n")

    restart()
    create_entity() @(eid, cmp) {
        cmp.score := 42
        cmp.tag   := "saved"
    }
    commit()

    // Save
    var data <- mem_archive_save(decsState)
    let savedSize = length(data)
    print("Saved {savedSize} bytes\n")

    // Clear and restore
    restart()
    mem_archive_load(data, decsState)

    // Verify
    query() $(score : int; tag : string) {
        print("Restored: score={score} tag={tag}\n")
    }
    // output:
    //   Saved ... bytes
    //   Restored: score=42 tag=saved
}

// === Entity ID recycling ===

def entity_recycling_demo() {
    print("\n=== Entity ID recycling ===\n")

    restart()
    let eid1 = create_entity() @(eid, cmp) {
        cmp.val := 1
    }
    commit()
    let gen1 = eid1.generation
    let id1  = eid1.id
    print("First entity: id={id1} gen={gen1}\n")

    delete_entity(eid1)
    commit()

    let eid2 = create_entity() @(eid, cmp) {
        cmp.val := 2
    }
    commit()
    print("New entity: id={eid2.id} gen={eid2.generation}\n")
    // The ID slot is reused but the generation increments,
    // so stale EntityIds cannot accidentally access new entities.
    print("Same slot: {eid2.id == id1}, different gen: {eid2.generation != gen1}\n")
    // output:
    //   First entity: id=0x0 gen=1
    //   New entity: id=0x0 gen=2
    //   Same slot: true, different gen: true
}

// === Archetype inspection ===

def archetype_inspection() {
    print("\n=== Archetype inspection ===\n")

    restart()
    // Entities with different component sets go to different archetypes
    create_entity() @(eid, cmp) {
        cmp.x := 1
    }
    create_entity() @(eid, cmp) {
        cmp.x := 2
        cmp.y := 3
    }
    commit()

    print("Number of archetypes: {length(decsState.allArchetypes)}\n")
    for (arch, i in decsState.allArchetypes, count()) {
        print("  Archetype {i}: {arch.size} entities, components:")
        for (c in arch.components) {
            print(" {c.name}")
        }
        print("\n")
    }
    // output:
    //   Number of archetypes: 2
    //   Archetype 0: 1 entities, components: eid x
    //   Archetype 1: 1 entities, components: eid x y
}

// === Utility functions ===

def utility_functions_demo() {
    print("\n=== Utility functions ===\n")

    restart()

    // is_alive checks whether an EntityId refers to a living entity.
    // Before any entities exist, INVALID_ENTITY_ID is not alive:
    print("INVALID alive? {is_alive(INVALID_ENTITY_ID)}\n")
    // output: INVALID alive? false

    // Create some entities
    let hero = create_entity() @(eid, cmp) {
        cmp.name := "hero"
        cmp.hp   := 100
        cmp.pos  := float3(1, 2, 3)
    }
    let goblin = create_entity() @(eid, cmp) {
        cmp.name := "goblin"
        cmp.hp   := 30
    }
    commit()

    print("hero alive? {is_alive(hero)}\n")
    // output: hero alive? true

    // entity_count returns the total number of alive entities
    print("entity count: {entity_count()}\n")
    // output: entity count: 2

    // Delete the goblin and check again
    delete_entity(goblin)
    commit()
    print("goblin alive after delete? {is_alive(goblin)}\n")
    print("entity count after delete: {entity_count()}\n")
    // output: goblin alive after delete? false
    // output: entity count after delete: 1

    // get_component retrieves a single component value by entity ID and name.
    // The type is inferred from the default value. If the entity is dead or
    // the component is missing, the default is returned.
    let hp = get_component(hero, "hp", 0)
    print("hero hp: {hp}\n")
    // output: hero hp: 100

    let pos = get_component(hero, "pos", float3(0))
    print("hero pos: {pos}\n")
    // output: hero pos: 1,2,3

    // Missing component returns default
    let shield = get_component(hero, "shield", -1)
    print("hero shield (missing): {shield}\n")
    // output: hero shield (missing): -1

    // Dead entity returns default
    let dead_hp = get_component(goblin, "hp", -999)
    print("dead goblin hp: {dead_hp}\n")
    // output: dead goblin hp: -999
}

// === Bulk entity creation ===

// create_entities creates many entities at once, faster than
// calling create_entity in a loop. All entities are immediately
// visible (no commit needed).

def bulk_create_demo() {
    print("\n=== Bulk entity creation ===\n")

    restart()

    // Simple bulk creation via ComponentMap
    create_entities(5) <| $(eid : EntityId; i : int; var cmp : ComponentMap) {
        apply_decs_template(cmp, Particle(
            pos  = float3(float(i), 0, 0),
            vel  = float3(0, 1, 0),
            life = 100 + i
        ))
    }

    print("After bulk create:\n")
    query() $(p : Particle) {
        print("  pos={p.pos} life={p.life}\n")
    }
    // output:
    //   pos=0,0,0 life=100
    //   pos=1,0,0 life=101
    //   pos=2,0,0 life=102
    //   pos=3,0,0 life=103
    //   pos=4,0,0 life=104
}

// === High-performance bulk creation with create_entities`T ===

// For maximum performance, create_entities`T bypasses ComponentMap
// entirely. The [decs_template] macro generates a type-specific
// bulk creation function that writes directly into archetype storage.

def bulk_create_direct_demo() {
    print("\n=== Direct bulk creation ===\n")

    restart()

    // create_entities`Particle — generated by [decs_template].
    // The block receives (eid, index, var entity) — just set fields.
    create_entities`Particle(5) <| $(eid : EntityId; i : int; var p : Particle) {
        p.pos  = float3(float(i) * 2.0, 0.0, 0.0)
        p.vel  = float3(0, 1, 0)
        p.life = 200 + i
    }

    print("After direct bulk create:\n")
    query() $(p : Particle) {
        print("  pos={p.pos} life={p.life}\n")
    }
    // output:
    //   pos=0,0,0 life=200
    //   pos=2,0,0 life=201
    //   pos=4,0,0 life=202
    //   pos=6,0,0 life=203
    //   pos=8,0,0 life=204

    // Multiple batches can coexist
    create_entities`Particle(3) <| $(eid : EntityId; i : int; var p : Particle) {
        p.pos  = float3(0, 0, float(i))
        p.vel  = float3(1, 0, 0)
        p.life = 50
    }
    print("Total entities: {entity_count()}\n")
    // output: Total entities: 8
}

[export]
def main() {
    creating_entities()
    querying_entities()
    query_by_eid()
    mutable_queries()
    require_filters()
    find_query_demo()
    deleting_entities()
    updating_entities()
    default_value_queries()
    template_demo()
    stage_demo()
    nested_query_demo()
    serialization_demo()
    entity_recycling_demo()
    archetype_inspection()
    utility_functions_demo()
    bulk_create_demo()
    bulk_create_direct_demo()
    return
}
