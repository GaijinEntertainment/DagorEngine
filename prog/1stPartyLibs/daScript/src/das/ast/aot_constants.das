options gen2

module aot_constants

let AOT_INCLUDES = "#include \"daScript/misc/platform.h\"

#include \"daScript/simulate/simulate.h\"
#include \"daScript/simulate/aot.h\"
#include \"daScript/simulate/aot_library.h\"

";

let AOT_HEADERS = "#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable:4100)   // unreferenced formal parameter
#pragma warning(disable:4189)   // local variable is initialized but not referenced
#pragma warning(disable:4244)   // conversion from 'int32_t' to 'float', possible loss of data
#pragma warning(disable:4114)   // same qualifier more than once
#pragma warning(disable:4623)   // default constructor was implicitly defined as deleted
#pragma warning(disable:4946)   // reinterpret_cast used between related classes
#pragma warning(disable:4269)   // 'const' automatic data initialized with compiler generated default constructor produces unreliable results
#pragma warning(disable:4555)   // result of expression not used
#endif
#if defined(__EDG__)
#pragma diag_suppress 826
#elif defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored \"-Wunused-parameter\"
#pragma GCC diagnostic ignored \"-Wunused-variable\"
#pragma GCC diagnostic ignored \"-Wunused-function\"
#pragma GCC diagnostic ignored \"-Wwrite-strings\"
#pragma GCC diagnostic ignored \"-Wreturn-local-addr\"
#pragma GCC diagnostic ignored \"-Wignored-qualifiers\"
#pragma GCC diagnostic ignored \"-Wsign-compare\"
#pragma GCC diagnostic ignored \"-Wsubobject-linkage\"
#endif
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored \"-Wunused-parameter\"
#pragma clang diagnostic ignored \"-Wwritable-strings\"
#pragma clang diagnostic ignored \"-Wunused-variable\"
#pragma clang diagnostic ignored \"-Wunused-but-set-variable\"
#pragma clang diagnostic ignored \"-Wunsequenced\"
#pragma clang diagnostic ignored \"-Wunused-function\"
#endif

";

let AOT_FOOTER = "
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
#if defined(__EDG__)
#pragma diag_default 826
#elif defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif
#if defined(__clang__)
#pragma clang diagnostic pop
#endif
";
