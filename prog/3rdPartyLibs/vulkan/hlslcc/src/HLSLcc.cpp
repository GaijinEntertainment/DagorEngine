
#include "hlslcc.h"

#include <memory>
#include "internal_includes/HLSLCrossCompilerContext.h"
#include "internal_includes/toGLSL.h"
#include "internal_includes/toMetal.h"
#include "internal_includes/Shader.h"
#include "internal_includes/decode.h"

using namespace std;


#ifndef GL_VERTEX_SHADER_ARB
#define GL_VERTEX_SHADER_ARB              0x8B31
#endif
#ifndef GL_FRAGMENT_SHADER_ARB
#define GL_FRAGMENT_SHADER_ARB            0x8B30
#endif
#ifndef GL_GEOMETRY_SHADER
#define GL_GEOMETRY_SHADER 0x8DD9
#endif
#ifndef GL_TESS_EVALUATION_SHADER
#define GL_TESS_EVALUATION_SHADER 0x8E87
#endif
#ifndef GL_TESS_CONTROL_SHADER
#define GL_TESS_CONTROL_SHADER 0x8E88
#endif
#ifndef GL_COMPUTE_SHADER
#define GL_COMPUTE_SHADER 0x91B9
#endif


HLSLCC_API int HLSLCC_APIENTRY TranslateHLSLFromMem(const char* shader,
                                                    unsigned int flags,
                                                    GLLang language,
                                                    const GlExtensions *extensions,
                                                    GLSLCrossDependencyDataInterface* dependencies,
                                                    HLSLccSamplerPrecisionInfo& samplerPrecisions,
                                                    HLSLccReflection& reflectionCallbacks,
                                                    std::stringstream& info_log,
                                                    GLSLShader* result)
{
  unique_ptr<Shader> psShader(DecodeDXBC((uint32_t*)shader, flags));

  if (psShader)
  {
    psShader->sInfo.AddSamplerPrecisions(samplerPrecisions);

    if (psShader->ui32MajorVersion <= 3)
      flags &= ~HLSLCC_FLAG_COMBINE_TEXTURE_SAMPLERS;

    unique_ptr<GLSLCrossDependencyDataInterface> depPtr;
    if (!dependencies)
      depPtr.reset(new GLSLCrossDependencyData());
    HLSLCrossCompilerContext sContext(reflectionCallbacks, info_log, *psShader, flags, dependencies ? *dependencies : *depPtr);

    if (language == LANG_METAL)
    {
      // Tessellation or geometry shaders are not supported
      if (psShader->eShaderType == HULL_SHADER || psShader->eShaderType == DOMAIN_SHADER || psShader->eShaderType == GEOMETRY_SHADER)
      {
        info_log << "Shader type not supported" << endl;
        result->sourceCode = "";
        return 0;
      }
      ToMetal translator(&sContext);
      if (!translator.Translate())
      {
        bdestroy(sContext.glsl);
        for (auto& phase : psShader->asPhases)
        {
          bdestroy(phase.postShaderCode);
          bdestroy(phase.earlyMain);
        }

        return 0;
      }
    }
    else
    {
      ToGLSL translator(&sContext);
      language = translator.SetLanguage(language);
      translator.SetExtensions(extensions);
      if (!translator.Translate())
      {
        bdestroy(sContext.glsl);
        for (auto& phase : psShader->asPhases)
        {
          bdestroy(phase.postShaderCode);
          bdestroy(phase.earlyMain);
        }

        return 0;
      }
    }

    auto glslcstr = bstr2cstr(sContext.glsl, '\0');
    result->sourceCode = glslcstr;
    bcstrfree(glslcstr);

    bdestroy(sContext.glsl);
    for (auto& phase : psShader->asPhases)
    {
      bdestroy(phase.postShaderCode);
      bdestroy(phase.earlyMain);
    }

    result->reflection = psShader->sInfo;

    result->textureSamplers = psShader->textureSamplers;

    switch (psShader->eShaderType)
    {
    case VERTEX_SHADER:
      result->shaderType = GL_VERTEX_SHADER_ARB;
      break;
    case GEOMETRY_SHADER:
      result->shaderType = GL_GEOMETRY_SHADER;
      break;
    case DOMAIN_SHADER:
      result->shaderType = GL_TESS_EVALUATION_SHADER;
      break;
    case HULL_SHADER:
      result->shaderType = GL_TESS_CONTROL_SHADER;
      break;
    case COMPUTE_SHADER:
      result->shaderType = GL_COMPUTE_SHADER;
      break;
    case PIXEL_SHADER:
      result->shaderType = GL_FRAGMENT_SHADER_ARB;
      break;
    default:
      break;
    }
  }

  result->GLSLLanguage = language;

  return 1;
}

HLSLCC_API int HLSLCC_APIENTRY TranslateHLSLFromFile(const char* filename,
                                                     unsigned int flags,
                                                     GLLang language,
                                                     const GlExtensions *extensions,
                                                     GLSLCrossDependencyDataInterface* dependencies,
                                                     HLSLccSamplerPrecisionInfo& samplerPrecisions,
                                                     HLSLccReflection& reflectionCallbacks,
                                                     std::stringstream& info_log,
                                                     GLSLShader* result)
{
  auto shaderFile = fopen(filename, "rb");

  if (!shaderFile)
  {
    info_log << "Can't open file " << filename << endl;
    return 0;
  }

  fseek(shaderFile, 0, SEEK_END);
  long length;
  length = ftell(shaderFile);
  fseek(shaderFile, 0, SEEK_SET);

  vector<char> shader(length);
  fread(shader.data(), 1, length, shaderFile);

  fclose(shaderFile);

  return TranslateHLSLFromMem(shader.data(), flags, language, extensions, dependencies, samplerPrecisions, reflectionCallbacks, info_log, result);
}

