#pragma once

#include "daScript/ast/ast.h"

namespace das {

    struct ProgramCfg;

    // Mark provably-in-range ExprAt nodes noBoundCheck, using CFG range facts (opt-in via bound_check_elision).
    void markNoBoundCheck ( Program * program, const ProgramCfg * pcfg, TextWriter & logs );

}
