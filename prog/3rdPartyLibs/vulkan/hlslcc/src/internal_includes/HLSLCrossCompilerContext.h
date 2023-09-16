#pragma once

#include <stdint.h>
#include <string>
#include <unordered_set>
#include "bstrlib.h"
#include "internal_includes/UseDefineChains.h"

class Shader;
class GLSLCrossDependencyDataInterface;
class ShaderPhase;
class Translator;
class Operand;
class HLSLccReflection;
struct InfoCallback;

class HLSLCrossCompilerContext
{
public:
  HLSLCrossCompilerContext(HLSLccReflection &refl,
                           std::stringstream& info_log,
                           Shader& shader,
                           uint32_t flags,
                           GLSLCrossDependencyDataInterface& deps)
    : m_Reflection(refl)
    , infoLog(info_log)
    , psShader(shader)
    , flags(flags)
    , psDependencies(deps)
  {
  }

  bstring glsl = nullptr;
  bstring extensions = nullptr;

  bstring* currentGLSLString = nullptr;//either glsl or earlyMain of current phase

  uint32_t currentPhase = 0;

  int indent = 0;
  uint32_t flags = 0;
  const char *inputPrefix = nullptr; // Prefix for shader inputs
  const char *outputPrefix = nullptr; // Prefix for shader outputs

  void DoDataTypeAnalysis(ShaderPhase& phase);

  void ClearDependencyData();

  void AddIndentation();
  void beginBlock();
  void endBlock();

  // Currently active translator
  Translator *psTranslator = nullptr;

  Shader& psShader;
  HLSLccReflection &m_Reflection; // Callbacks for bindings and diagnostic info
  GLSLCrossDependencyDataInterface& psDependencies;
  std::stringstream& infoLog;
  std::unordered_set<std::string> activeExtensions;

  // Retrieve the name for which the input or output is declared as. Takes into account possible redirections.
  std::string GetDeclaredInputName(const Operand& psOperand, int *piRebase, bool iIgnoreRedirect, uint32_t *puiIgnoreSwizzle) const;
  std::string GetDeclaredOutputName(const Operand& psOperand, int* stream, uint32_t *puiIgnoreSwizzle, int *piRebase, bool iIgnoreRedirect) const;

  bool OutputNeedsDeclaring(const Operand* psOperand, const int count);

  void RequireExtension(const char* name)
  {
    if (!activeExtensions.count(name))
    {
      activeExtensions.insert(name);
      bformata(extensions, "#extension %s : require\n", name);
    }
  }
};
