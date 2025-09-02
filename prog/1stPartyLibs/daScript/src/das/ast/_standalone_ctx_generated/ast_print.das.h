#include "daScript/misc/platform.h"

#include "daScript/simulate/simulate.h"
#include "daScript/simulate/aot.h"
#include "daScript/simulate/aot_library.h"

namespace das {
namespace ast_print {


class Standalone : public Context {
public:
    Standalone();
    auto allExpr ( int32_t arg ) -> void;
    auto test ( ) -> bool;
    auto printAst ( smart_ptr_raw<Program> prog, StringBuilderWriter * writer ) -> void;
    auto printExpr ( smart_ptr_raw<Expression> expr, StringBuilderWriter * writer ) -> void;
    auto printFunc ( smart_ptr_raw<Function> func, StringBuilderWriter * writer ) -> void;
    auto setFlags ( smart_ptr_raw<Program> prog ) -> void;
};
} // namespace ast_print
} // namespace das
