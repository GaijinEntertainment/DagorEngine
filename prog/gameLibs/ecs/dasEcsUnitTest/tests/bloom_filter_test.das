options no_global_variables = false
require ecs
require DasEcsUnitTest

// Test: bloom filter in updateAllQueriesInternal prevents re-resolution of
// partially-resolved queries, causing stale REQUIRE_NOT checks.
//
// Setup:
//   bloom_filter_test.das is compiled BEFORE z_bloom_late_register.das (alphabetical order).
//   The ES query below has REQUIRE_NOT=bloom_test_nope, but bloom_test_nope is NOT registered
//   when this file's queries are created. The query becomes RESOLVED but not FULLY_RESOLVED:
//   bloom_test_nope gets INVALID_COMPONENT_INDEX in the NO components array.
//
//   bloom_a and bloom_b ARE registered by the time this query is first resolved, because
//   scene.blk contains a bloom_ab entity (created during scene loading, before EventStart).
//   This gives the query a non-zero qbm (required bitmask).
//
//   At EventStart, bloom_test_setup creates a "just_nope" entity, which registers
//   bloom_test_nope as a new component (setting shouldResolveQueries=true). The just_nope
//   archetype does NOT contain bloom_a/bloom_b, so the bloom filter check:
//     (newArchsBitmask & qbm) != qbm
//   causes the query to be SKIPPED, consuming shouldResolveQueries without re-resolving.
//
//   Then bloom_abd is created. Since bloom_test_nope was already registered by just_nope,
//   no new component types are created, so shouldResolveQueries stays false. The query
//   is not re-resolved, and bloom_test_nope remains INVALID_COMPONENT_INDEX in the NO array.
//
// Bug manifestation:
//   REQUIRE_NOT check uses getComponentId(INVALID_COMPONENT_INDEX) which always returns
//   INVALID_ARCHETYPE_COMPONENT_ID -- the archetype appears to NOT have the component even
//   when it does. Entities with bloom_test_nope are incorrectly matched by the query.

[es(REQUIRE_NOT=bloom_test_nope)]
def bloom_query_act(info : UpdateStageInfoAct; bloom_a : int; bloom_b : int)
  set_test_value("bloom_last_a", bloom_a)
  set_test_value("bloom_last_b", bloom_b)
  let prev = get_test_value("bloom_match_count")
  set_test_value("bloom_match_count", (prev >= 0 ? prev : 0) + 1)

[es(on_event=EventStart)]
def bloom_test_setup(evt : Event)
  set_test_value("bloom_match_count", 0)

  // Register bloom_test_nope component by creating an entity that has it.
  // This sets shouldResolveQueries=true but the bloom filter skips our query
  // (just_nope archetype doesn't have bloom_a/bloom_b bits) and consumes the flag.
  createEntitySync("just_nope")

  // Entity that should NOT match: has bloom_a + bloom_b + bloom_test_nope
  // (bloom_abd template defined in z_bloom_late_register.das)
  // Since shouldResolveQueries is now false, the query is not re-resolved.
  createEntitySync("bloom_abd")

[es(on_event=EventEnd)]
def bloom_test_verify(evt : Event)
  let count = get_test_value("bloom_match_count")
  // 100 update frames (DEF_RUNS) * 1 correctly matching entity (scene bloom_ab) = 100
  // Bug: 100 * 2 = 200 (bloom_abd entity incorrectly matches due to stale REQUIRE_NOT)
  verify(count == 100)
