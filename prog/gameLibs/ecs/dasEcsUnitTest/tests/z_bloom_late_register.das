require ecs
require %daslibEcs/ecs_template

// This file is compiled AFTER bloom_filter_test.das (alphabetical order: 'z' > 'b').
// It registers the bloom_abd template via [ecs_template]. The template definition is
// stored in the TemplateDB, but component indices are NOT created until an entity of
// this template is first instantiated (lazy registration in instantiateTemplate).
//
// When bloom_filter_test.das creates an entity via createEntitySync("bloom_abd"),
// the archetype is created and components are resolved. By that point,
// bloom_test_nope should have been re-resolved in the query, but the bloom filter
// bug in updateAllQueriesInternal prevents re-resolution.

// Template with bloom_a, bloom_b, and bloom_test_nope (all three components listed directly).
// Used by bloom_filter_test.das to create entities that SHOULD be excluded by REQUIRE_NOT.
[ecs_template]
struct bloom_abd
  bloom_a : int = 0
  bloom_b : int = 0
  bloom_test_nope : int = 0
