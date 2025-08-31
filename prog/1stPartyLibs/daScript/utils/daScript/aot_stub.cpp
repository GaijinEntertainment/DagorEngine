#include "daScript/misc/platform.h"
#include "daScript/simulate/aot_library.h"

/*
    DAS_AOT_LIB("daslib/functional.das" AOT_GENERATED_SRC dasAotStub daScript)
    DAS_AOT_LIB("daslib/json.das" AOT_GENERATED_SRC dasAotStub daScript)
    DAS_AOT_LIB("daslib/json_boost.das" AOT_GENERATED_SRC dasAotStub daScript)
    DAS_AOT_LIB("daslib/regex.das" AOT_GENERATED_SRC dasAotStub daScript)
    DAS_AOT_LIB("daslib/regex_boost.das" AOT_GENERATED_SRC dasAotStub daScript)
    DAS_AOT_LIB("daslib/strings_boost.das" AOT_GENERATED_SRC dasAotStub daScript)
    DAS_AOT_LIB("daslib/random.das" AOT_GENERATED_SRC dasAotStub daScript)
    DAS_AOT_LIB("daslib/math_boost.das" AOT_GENERATED_SRC dasAotStub daScript)
    DAS_AOT_LIB("daslib/utf8_utils.das" AOT_GENERATED_SRC dasAotStub daScript)
*/

namespace das {

    extern AotListBase impl_aot_functional;
    extern AotListBase impl_aot_json;
    extern AotListBase impl_aot_json_boost;
    extern AotListBase impl_aot_regex;
    extern AotListBase impl_aot_regex_boost;
    extern AotListBase impl_aot_strings_boost;
    extern AotListBase impl_aot_random;
    extern AotListBase impl_aot_math_boost;
    extern AotListBase impl_aot_utf8_utils;

    vector<void *> force_aot_stub() {
        vector<void *> stubs = {
            &impl_aot_functional,
            &impl_aot_json,
            &impl_aot_json_boost,
            &impl_aot_regex,
            &impl_aot_regex_boost,
            &impl_aot_strings_boost,
            &impl_aot_random,
            &impl_aot_math_boost,
            &impl_aot_utf8_utils
        };
        return stubs;
    }

}
