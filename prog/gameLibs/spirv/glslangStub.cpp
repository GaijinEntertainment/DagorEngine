//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//

#include <spirv/compiler.h>

namespace spirv
{
CompileToSpirVResult compileGLSL(dag::ConstSpan<const char *>, EShLanguage, CompileFlags) { return CompileToSpirVResult(); }
CompileToSpirVResult compileHLSL(dag::ConstSpan<const char *>, const char *, EShLanguage, CompileFlags)
{
  return CompileToSpirVResult();
}
} // namespace spirv
