#ifndef LANGUAGES_H
#define LANGUAGES_H

#include "hlslcc.h"

inline int InOutSupported(const GLLang eLang)
{
  return !(eLang == LANG_ES_100 || eLang == LANG_120);
}

inline int WriteToFragData(const GLLang eLang)
{
  return (eLang == LANG_ES_100 || eLang == LANG_120);
}

inline bool ShaderBitEncodingSupported(const GLLang eLang)
{
  return !(eLang != LANG_ES_300 && eLang != LANG_ES_310 && eLang < LANG_330);
}

inline bool HaveOverloadedTextureFuncs(const GLLang eLang)
{
  return !(eLang == LANG_ES_100 || eLang == LANG_120);
}

//Only enable for ES.
//Not present in 120, ignored in other desktop languages.
inline bool HavePrecisionQualifers(const GLLang eLang)
{
  return (eLang >= LANG_ES_100 && eLang <= LANG_ES_310);
}

inline bool HaveCubemapArray(const GLLang eLang)
{
  return (eLang >= LANG_400 && eLang <= LANG_GL_LAST);
}

inline bool IsESLanguage(const GLLang eLang)
{
  return (eLang >= LANG_ES_FIRST && eLang <= LANG_ES_LAST);
}

inline bool IsDesktopGLLanguage(const GLLang eLang)
{
  return (eLang >= LANG_GL_FIRST && eLang <= LANG_GL_LAST);
}

//Only on vertex inputs and pixel outputs.
inline bool HaveLimitedInOutLocationQualifier(const GLLang eLang, const GlExtensions *extensions)
{
  return  (eLang >= LANG_330 || eLang == LANG_ES_300 || eLang == LANG_ES_310 || (extensions && extensions->ARB_explicit_attrib_location));
}

inline bool HaveInOutLocationQualifier(const GLLang eLang)
{
  return (eLang >= LANG_410 || eLang == LANG_ES_310);
}

//layout(binding = X) uniform {uniformA; uniformB;}
//layout(location = X) uniform uniform_name;
inline bool HaveUniformBindingsAndLocations(const GLLang eLang, const GlExtensions *extensions, unsigned int flags)
{
  if (flags & HLSLCC_FLAG_DISABLE_EXPLICIT_LOCATIONS)
    return false;

  return (eLang >= LANG_430 || eLang == LANG_ES_310 || (extensions && extensions->ARB_explicit_uniform_location && extensions->ARB_shading_language_420pack));
}

inline bool DualSourceBlendSupported(const GLLang eLang)
{
  return (eLang >= LANG_330);
}
// TODO: vulkan target has no subroutines
inline bool SubroutinesSupported(const GLLang eLang)
{
  return (eLang >= LANG_400);
}

//Before 430, flat/smooth/centroid/noperspective must match
//between fragment and its previous stage.
//HLSL bytecode only tells us the interpolation in pixel shader.
inline bool PixelInterpDependency(const GLLang eLang)
{
  return (eLang < LANG_430);
}

inline bool HaveUVec(const GLLang eLang)
{
  return !(eLang == LANG_ES_100 || eLang == LANG_120);
}

inline bool HaveGather(const GLLang eLang)
{
  return (eLang >= LANG_400 || eLang == LANG_ES_310);
}

inline bool HaveGatherNonConstOffset(const GLLang eLang)
{
  return (eLang >= LANG_420 || eLang == LANG_ES_310);
}


inline bool HaveQueryLod(const GLLang eLang)
{
  return (eLang >= LANG_400);
}

inline bool HaveQueryLevels(const GLLang eLang)
{
  return (eLang >= LANG_430);
}

inline bool HaveFragmentCoordConventions(const GLLang eLang)
{
  return (eLang >= LANG_150);
}

inline bool HaveGeometryShaderARB(const GLLang eLang)
{
  return (eLang >= LANG_150);
}

inline bool HaveAtomicCounter(const GLLang eLang)
{
  return (eLang >= LANG_420 || eLang == LANG_ES_310);
}

inline bool HaveAtomicMem(const GLLang eLang)
{
  return (eLang >= LANG_430 || eLang == LANG_ES_310);
}

inline bool HaveImageAtomics(const GLLang eLang)
{
  return (eLang >= LANG_420);
}

inline bool HaveCompute(const GLLang eLang)
{
  return (eLang >= LANG_430 || eLang == LANG_ES_310);
}

inline bool HaveImageLoadStore(const GLLang eLang)
{
  return (eLang >= LANG_420 || eLang == LANG_ES_310);
}

#endif
