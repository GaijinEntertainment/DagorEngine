#include "shadersBinaryData.h"

namespace shader_assert
{
class ScopedShaderAssert
{
  const int mShaderClassId;

public:
  ScopedShaderAssert(const shaderbindump::ShaderClass &sh_class);
  ~ScopedShaderAssert();
};
} // namespace shader_assert
