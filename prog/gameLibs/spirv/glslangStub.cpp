// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <spirv/compiler.h>

namespace spirv
{
CompileToSpirVResult compileGLSL(dag::ConstSpan<const char *>, EShLanguage, CompileFlags) { return CompileToSpirVResult(); }
CompileToSpirVResult compileHLSL(dag::ConstSpan<const char *>, const char *, EShLanguage, CompileFlags)
{
  return CompileToSpirVResult();
}
} // namespace spirv
