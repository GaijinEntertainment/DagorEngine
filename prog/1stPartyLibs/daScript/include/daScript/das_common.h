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

}

#endif //DAS_DAS_COMMON_H
