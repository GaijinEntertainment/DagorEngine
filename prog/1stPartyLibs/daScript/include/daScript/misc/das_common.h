#ifndef DAS_DAS_COMMON_H
#define DAS_DAS_COMMON_H

#include <daScript/simulate/simulate.h>
#include <daScript/das_project_specific.h>
#include <daScript/ast/ast.h>

namespace das {
    inline ContextPtr SimulateWithErrReport(ProgramPtr program, TextWriter &tw) {
        ContextPtr pctx ( get_context(program->getContextStackSize()) );
        if ( !program->simulate(*pctx, tw) ) {
            tw << "failed to simulate\n";
            for ( auto & err : program->errors ) {
                tw << reportError(err.at, err.what, err.extra, err.fixme, err.cerr);
            }
            return nullptr;
        }
        return pctx;
    }

    template <typename K, typename V, typename Compare = std::less<K>>
    vector<pair<K, V>> ordered(const das_hash_map<K, V> &unsorted_map, Compare cmp = {}) {
        static_assert(!is_pointer_v<K> ||
                      !is_same_v<Compare, less<K>>,
                      "When K is pointer you should provide user-defined comparator. "
                      "Because we use this method to avoid nondeterminism in map traversal.");
        vector<pair<K, V>> sorted_vector(unsorted_map.begin(), unsorted_map.end());

        // Sort the vector by key
        sort(sorted_vector.begin(), sorted_vector.end(),
                  [&cmp](const auto &p1, const auto &p2) { return cmp(p1.first, p2.first); } );
        return sorted_vector;
    }

    // Insert-only variant — same body, different parameter type. Two overloads
    // rather than a generic template so the static_assert / signature stay symmetric.
    // Guarded: under DAS_CUSTOM_HASH=0, das_insert_only_hash_map aliases to the
    // same std::unordered_map as das_hash_map, so the regular overload above
    // already serves insert-only args.
#if DAS_CUSTOM_HASH
    template <typename K, typename V, typename Compare = std::less<K>>
    vector<pair<K, V>> ordered(const das_insert_only_hash_map<K, V> &unsorted_map, Compare cmp = {}) {
        static_assert(!is_pointer_v<K> ||
                      !is_same_v<Compare, less<K>>,
                      "When K is pointer you should provide user-defined comparator. "
                      "Because we use this method to avoid nondeterminism in map traversal.");
        vector<pair<K, V>> sorted_vector(unsorted_map.begin(), unsorted_map.end());
        sort(sorted_vector.begin(), sorted_vector.end(),
                  [&cmp](const auto &p1, const auto &p2) { return cmp(p1.first, p2.first); } );
        return sorted_vector;
    }
#endif

    template <typename K, typename Compare = less<K>>
    vector<K> ordered(const das_set<K> &unsorted_map, Compare cmp = {}) {
        static_assert(!is_pointer_v<K> ||
                      !is_same_v<Compare, less<K>>,
                      "When K is pointer you should provide user-defined comparator. "
                      "Because we use this method to avoid nondeterminism in set traversal.");
        vector<K> sorted_vector(unsorted_map.begin(), unsorted_map.end());

        // Sort the vector by key
        sort(sorted_vector.begin(), sorted_vector.end(), cmp);
        return sorted_vector;
    }

    // See note on the insert-only map overload above.
#if DAS_CUSTOM_HASH
    template <typename K, typename Compare = less<K>>
    vector<K> ordered(const das_insert_only_hash_set<K> &unsorted_map, Compare cmp = {}) {
        static_assert(!is_pointer_v<K> ||
                      !is_same_v<Compare, less<K>>,
                      "When K is pointer you should provide user-defined comparator. "
                      "Because we use this method to avoid nondeterminism in set traversal.");
        vector<K> sorted_vector(unsorted_map.begin(), unsorted_map.end());
        sort(sorted_vector.begin(), sorted_vector.end(), cmp);
        return sorted_vector;
    }
#endif


    bool starts_with ( const string & name, const char * template_name );

}

#endif //DAS_DAS_COMMON_H
