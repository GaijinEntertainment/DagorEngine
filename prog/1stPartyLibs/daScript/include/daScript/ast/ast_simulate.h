#include "daScript/ast/ast.h"
#include "daScript/simulate/simulate.h"

namespace das {
    DAS_API SimNode * simulateExpression ( Context & context, Expression * expr );
    DAS_API SimNode * trySimulateExpression ( Context & context, const Expression * expr, uint32_t extraOffset, const TypeDeclPtr & r2vType );
    DAS_API SimNode * GetR2V ( Context & context, const LineInfo & at, const TypeDeclPtr & type, SimNode * expr );
}
