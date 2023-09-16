#ifndef HLSLCC_H_
#define HLSLCC_H_

#include <string>
#include <vector>
#include <map>
#include <sstream>

#if defined (_WIN32) && defined(HLSLCC_DYNLIB)
    #define HLSLCC_APIENTRY __stdcall
    #if defined(libHLSLcc_EXPORTS)
        #define HLSLCC_API __declspec(dllexport)
    #else
        #define HLSLCC_API __declspec(dllimport)
    #endif
#else
    #define HLSLCC_APIENTRY
    #define HLSLCC_API
#endif

#include <stdint.h>
#include <string.h>

typedef enum
{
    LANG_DEFAULT,// Depends on the HLSL shader model.
    LANG_ES_100, LANG_ES_FIRST=LANG_ES_100,
    LANG_ES_300,
    LANG_ES_310, LANG_ES_LAST = LANG_ES_310,
    LANG_120, LANG_GL_FIRST = LANG_120,
    LANG_130,
    LANG_140,
    LANG_150,
    LANG_330,
    LANG_400,
    LANG_410,
    LANG_420,
    LANG_430,
    LANG_440, LANG_GL_LAST = LANG_440,
    LANG_METAL,
} GLLang;

typedef struct GlExtensions {
  uint32_t ARB_explicit_attrib_location : 1;
  uint32_t ARB_explicit_uniform_location : 1;
  uint32_t ARB_shading_language_420pack : 1;
}GlExtensions;

#include "ShaderInfo.h"

typedef std::vector<std::string> TextureSamplerPairs;

typedef enum INTERPOLATION_MODE
{
    INTERPOLATION_UNDEFINED = 0,
    INTERPOLATION_CONSTANT = 1,
    INTERPOLATION_LINEAR = 2,
    INTERPOLATION_LINEAR_CENTROID = 3,
    INTERPOLATION_LINEAR_NOPERSPECTIVE = 4,
    INTERPOLATION_LINEAR_NOPERSPECTIVE_CENTROID = 5,
    INTERPOLATION_LINEAR_SAMPLE = 6,
    INTERPOLATION_LINEAR_NOPERSPECTIVE_SAMPLE = 7,
} INTERPOLATION_MODE;

#define PS_FLAG_VERTEX_SHADER	0x1
#define PS_FLAG_HULL_SHADER		0x2
#define PS_FLAG_DOMAIN_SHADER	0x4
#define PS_FLAG_GEOMETRY_SHADER 0x8
#define PS_FLAG_PIXEL_SHADER	0x10

#define TO_FLAG_NONE    0x0
#define TO_FLAG_INTEGER 0x1
#define TO_FLAG_NAME_ONLY 0x2
#define TO_FLAG_DECLARATION_NAME 0x4
#define TO_FLAG_DESTINATION 0x8 //Operand is being written to by assignment.
#define TO_FLAG_UNSIGNED_INTEGER 0x10
#define TO_FLAG_DOUBLE 0x20
// --- TO_AUTO_BITCAST_TO_FLOAT ---
//If the operand is an integer temp variable then this flag
//indicates that the temp has a valid floating point encoding
//and that the current expression expects the operand to be floating point
//and therefore intBitsToFloat must be applied to that variable.
#define TO_AUTO_BITCAST_TO_FLOAT 0x40
#define TO_AUTO_BITCAST_TO_INT 0x80
#define TO_AUTO_BITCAST_TO_UINT 0x100
// AUTO_EXPAND flags automatically expand the operand to at least (i/u)vecX
// to match HLSL functionality.
#define TO_AUTO_EXPAND_TO_VEC2 0x200
#define TO_AUTO_EXPAND_TO_VEC3 0x400
#define TO_AUTO_EXPAND_TO_VEC4 0x800
#define TO_FLAG_BOOL 0x1000
// These flags are only used for Metal:
// Force downscaling of the operand to match
// the other operand (Metal doesn't like mixing halfs with floats)
#define TO_FLAG_FORCE_HALF 0x2000

#define TO_FLAG_FORCE_SWIZZLE 0x4000

typedef enum
{
	INVALID_SHADER = -1,
	PIXEL_SHADER,
	VERTEX_SHADER,
	GEOMETRY_SHADER,
	HULL_SHADER,
	DOMAIN_SHADER,
	COMPUTE_SHADER,
} SHADER_TYPE;

// Enum for texture dimension reflection data 
typedef enum
{
	TD_FLOAT = 0,
	TD_INT,
	TD_2D,
	TD_3D,
	TD_CUBE,
	TD_2DSHADOW,
	TD_2DARRAY,
	TD_CUBEARRAY
} HLSLCC_TEX_DIMENSION;

// The prefix for all temporary variables used by the generated code.
// Using a texture or uniform name like this will cause conflicts
#define HLSLCC_TEMP_PREFIX "u_xlat"

//The shader stages (Vertex, Pixel et al) do not depend on each other
//in HLSL. GLSL is a different story. HLSLCrossCompiler requires
//that hull shaders must be compiled before domain shaders, and
//the pixel shader must be compiled before all of the others.
//During compilation the GLSLCrossDependencyData struct will
//carry over any information needed about a different shader stage
//in order to construct valid GLSL shader combinations.

//Using GLSLCrossDependencyData is optional. However some shader
//combinations may show link failures, or runtime errors.
class GLSLCrossDependencyDataInterface
{
public:
  GLSLCrossDependencyDataInterface() = default;
  virtual ~GLSLCrossDependencyDataInterface() = default;
  // A container for a single Vulkan resource binding (<set, binding> pair)
  typedef std::pair<uint32_t, uint32_t> VulkanResourceBinding;

  virtual uint32_t GetVaryingLocation(const std::string &name, SHADER_TYPE eShaderType, bool isInput) = 0;
  virtual VulkanResourceBinding GetVulkanResourceBinding(std::string &name, SHADER_TYPE eShaderType, const ResourceBinding& binding, bool isDepthSampler, bool allocRoomForCounter = false, uint32_t preferredSet = 0, uint32_t constBufferSize = 0) = 0;
  virtual INTERPOLATION_MODE GetInterpolationMode(uint32_t regNo) = 0;
  virtual void SetInterpolationMode(uint32_t regNo, INTERPOLATION_MODE mode) = 0;
  virtual void ClearCrossDependencyData() = 0;
  virtual void setTessPartitioning(TESSELLATOR_PARTITIONING tp) = 0;
  virtual TESSELLATOR_PARTITIONING getTessPartitioning() = 0;
  virtual void setTessOutPrimitive(TESSELLATOR_OUTPUT_PRIMITIVE top) = 0;
  virtual TESSELLATOR_OUTPUT_PRIMITIVE getTessOutPrimitive() = 0;
  virtual uint32_t getProgramStagesMask() = 0;
  virtual void FragmentColorTarget(const char* name, uint32_t location, uint32_t index) = 0;
};
class GLSLCrossDependencyData : public GLSLCrossDependencyDataInterface
{
public:
  virtual void FragmentColorTarget(const char*, uint32_t, uint32_t) override
  {
  }
  //Required if PixelInterpDependency is true
  std::vector<INTERPOLATION_MODE> pixelInterpolation;

  // Map of varying locations, indexed by varying names.
  typedef std::map<std::string, uint32_t> VaryingLocations;

  static const int MAX_NAMESPACES = 6; // Max namespaces: vert input, hull input, domain input, geom input, ps input, (ps output)

  VaryingLocations varyingLocationsMap[MAX_NAMESPACES];
  uint32_t nextAvailableVaryingLocation[MAX_NAMESPACES];

  typedef std::map<std::string, VulkanResourceBinding> VulkanResourceBindings;
  VulkanResourceBindings m_VulkanResourceBindings;
  uint32_t m_NextAvailableVulkanResourceBinding[8]; // one per set. 

  inline int GetVaryingNamespace(SHADER_TYPE eShaderType, bool isInput)
  {
    switch (eShaderType)
    {
    case VERTEX_SHADER:
      return isInput ? 0 : 1;

    case HULL_SHADER:
      return isInput ? 1 : 2;

    case DOMAIN_SHADER:
      return isInput ? 2 : 3;

    case GEOMETRY_SHADER:
      // The input depends on whether there's a tessellation shader before us
      if (isInput)
      {
        return ui32ProgramStages & PS_FLAG_DOMAIN_SHADER ? 3 : 1;
      }
      return 4;

    case PIXEL_SHADER:
      // The inputs can come from geom shader, domain shader or directly from vertex shader
      if (isInput)
      {
        if (ui32ProgramStages & PS_FLAG_GEOMETRY_SHADER)
        {
          return 4;
        }
        else if (ui32ProgramStages & PS_FLAG_DOMAIN_SHADER)
        {
          return 3;
        }
        else
        {
          return 1;
        }
      }
      return 5; // This value never really used
    default:
      return 0;
    }
  }



public:
  GLSLCrossDependencyData()
    : ui32ProgramStages(0)
  {
    memset(nextAvailableVaryingLocation, 0, sizeof(nextAvailableVaryingLocation));
    memset(m_NextAvailableVulkanResourceBinding, 0, sizeof(m_NextAvailableVulkanResourceBinding));
  }


  // Retrieve the location for a varying with a given name.
  // If the name doesn't already have an allocated location, allocate one
  // and store it into the map.
  virtual uint32_t GetVaryingLocation(const std::string &name, SHADER_TYPE eShaderType, bool isInput) override
  {
    int nspace = GetVaryingNamespace(eShaderType, isInput);
    VaryingLocations::iterator itr = varyingLocationsMap[nspace].find(name);
    if (itr != varyingLocationsMap[nspace].end())
      return itr->second;

    uint32_t newKey = nextAvailableVaryingLocation[nspace];
    nextAvailableVaryingLocation[nspace]++;
    varyingLocationsMap[nspace].insert(std::make_pair(name, newKey));
    return newKey;
  }

  // Retrieve the binding for a resource (texture, constant buffer, image) with a given name
  // If not found, allocate a new one (in set 0) and return that
  // The returned value is a pair of <set, binding>
  // If the name contains "hlslcc_set_X_bind_Y", those values (from the first found occurence in the name)
  // will be used instead, and all occurences of that string will be removed from name, so name parameter can be modified
  // if allocRoomForCounter is true, the following binding number in the same set will be allocated with name + '_counter'
  virtual VulkanResourceBinding GetVulkanResourceBinding(std::string &name,
                                                         SHADER_TYPE eShaderType,
                                                         const ResourceBinding& binding,
                                                         bool isDepthSampler,
                                                         bool allocRoomForCounter = false,
                                                         uint32_t preferredSet = 0,
                                                         uint32_t constBufferSize = 0) override
  {
    (void)constBufferSize;
    (void)isDepthSampler;
    (void)binding;
    (void)eShaderType;
    // scan for the special marker
    const char *marker = "Xhlslcc_set_%d_bind_%dX";
    uint32_t Set = 0, Binding = 0;
    size_t startLoc = name.find("Xhlslcc");
    if ((startLoc != std::string::npos) && (sscanf(name.c_str() + startLoc, marker, &Set, &Binding) == 2))
    {
      // Get rid of all markers
      while ((startLoc = name.find("Xhlslcc")) != std::string::npos)
      {
        size_t endLoc = name.find('X', startLoc + 1);
        if (endLoc == std::string::npos)
          break;
        name.erase(startLoc, endLoc - startLoc + 1);
      }
      // Add to map
      VulkanResourceBinding newBind = std::make_pair(Set, Binding);
      m_VulkanResourceBindings.insert(std::make_pair(name, newBind));
      if (allocRoomForCounter)
      {
        VulkanResourceBinding counterBind = std::make_pair(Set, Binding + 1);
        m_VulkanResourceBindings.insert(std::make_pair(name + "_counter", counterBind));
      }

      return newBind;
    }

    VulkanResourceBindings::iterator itr = m_VulkanResourceBindings.find(name);
    if (itr != m_VulkanResourceBindings.end())
      return itr->second;

    // Allocate a new one
    VulkanResourceBinding newBind = std::make_pair(preferredSet, m_NextAvailableVulkanResourceBinding[preferredSet]);
    m_NextAvailableVulkanResourceBinding[preferredSet]++;
    m_VulkanResourceBindings.insert(std::make_pair(name, newBind));
    if (allocRoomForCounter)
    {
      VulkanResourceBinding counterBind = std::make_pair(preferredSet, m_NextAvailableVulkanResourceBinding[preferredSet]);
      m_NextAvailableVulkanResourceBinding[preferredSet]++;
      m_VulkanResourceBindings.insert(std::make_pair(name + "_counter", counterBind));
    }
    return newBind;
  }

  //dcl_tessellator_partitioning and dcl_tessellator_output_primitive appear in hull shader for D3D,
  //but they appear on inputs inside domain shaders for GL.
  //Hull shader must be compiled before domain so the
  //ensure correct partitioning and primitive type information
  //can be saved when compiling hull and passed to domain compilation.
  TESSELLATOR_PARTITIONING eTessPartitioning;
  TESSELLATOR_OUTPUT_PRIMITIVE eTessOutPrim;

  virtual void setTessPartitioning(TESSELLATOR_PARTITIONING tp) override
  {
    eTessPartitioning = tp;
  }
  virtual TESSELLATOR_PARTITIONING getTessPartitioning() override
  {
    return eTessPartitioning;
  }
  virtual void setTessOutPrimitive(TESSELLATOR_OUTPUT_PRIMITIVE top)
  {
    eTessOutPrim = top;
  }
  virtual TESSELLATOR_OUTPUT_PRIMITIVE getTessOutPrimitive() override
  {
    return eTessOutPrim;
  }

  // Bitfield for the shader stages this program is going to include (see PS_FLAG_*).
  // Needed so we can construct proper shader input and output names
  uint32_t ui32ProgramStages;
  virtual uint32_t getProgramStagesMask() override
  {
    return ui32ProgramStages;
  }

  virtual INTERPOLATION_MODE GetInterpolationMode(uint32_t regNo) override
  {
    if (regNo >= pixelInterpolation.size())
      return INTERPOLATION_UNDEFINED;
    else
      return pixelInterpolation[regNo];
  }

  virtual void SetInterpolationMode(uint32_t regNo, INTERPOLATION_MODE mode) override
  {
    if (regNo >= pixelInterpolation.size())
      pixelInterpolation.resize((regNo + 1) * 2, INTERPOLATION_UNDEFINED);

    pixelInterpolation[regNo] = mode;
  }

  virtual void ClearCrossDependencyData() override
  {
    pixelInterpolation.clear();
    for (int i = 0; i < MAX_NAMESPACES; i++)
    {
      varyingLocationsMap[i].clear();
      nextAvailableVaryingLocation[i] = 0;
    }
  }
};

struct GLSLShader
{
    int shaderType; //One of the GL enums.
    std::string sourceCode;
    ShaderInfo reflection;
    GLLang GLSLLanguage;
    TextureSamplerPairs textureSamplers;    // HLSLCC_FLAG_COMBINE_TEXTURE_SAMPLERS fills this out
};

// Interface for retrieving reflection and diagnostics data
class HLSLccReflection
{
public:
  HLSLccReflection() = default;
  virtual ~HLSLccReflection() = default;

  // Called on errors or diagnostic messages
  virtual void OnDiagnostics(const std::string &/*error*/, int /*line*/, bool /*isError*/) {}
  virtual void OnInputBinding(const std::string &/*name*/, int /*bindIndex*/) {}
  virtual void OnInputBindingEx(const std::string &/*name*/, int /*bindIndex*/, int /*vSize*/) {}
  virtual bool OnConstantBuffer(const std::string &/*name*/, size_t /*bufferSize*/, size_t /*memberCount*/) { return true; }
  virtual bool OnConstant(const std::string &/*name*/, int /*bindIndex*/, SHADER_VARIABLE_TYPE /*cType*/, int /*rows*/, int /*cols*/, bool /*isMatrix*/, int /*arraySize*/) { return true; }
  virtual void OnConstantBufferBinding(const std::string &/*name*/, int /*bindIndex*/) {}
  virtual void OnTextureBinding(const std::string &/*name*/, int /*bindIndex*/, HLSLCC_TEX_DIMENSION /*dim*/, bool /*isUAV*/) {}
  virtual void OnBufferBinding(const std::string &/*name*/, int /*bindIndex*/, bool /*isUAV*/) {}
  virtual void OnThreadGroupSize(unsigned int /*xSize*/, unsigned int /*ySize*/, unsigned int /*zSize*/) {}
};


/*HLSL constant buffers are treated as default-block unform arrays by default. This is done
  to support versions of GLSL which lack ARB_uniform_buffer_object functionality.
  Setting this flag causes each one to have its own uniform block.
  Note: Currently the nth const buffer will be named UnformBufferN. This is likey to change to the original HLSL name in the future.*/
static const unsigned int HLSLCC_FLAG_UNIFORM_BUFFER_OBJECT = 0x1;

static const unsigned int HLSLCC_FLAG_ORIGIN_UPPER_LEFT = 0x2;

static const unsigned int HLSLCC_FLAG_PIXEL_CENTER_INTEGER = 0x4;

static const unsigned int HLSLCC_FLAG_GLOBAL_CONSTS_NEVER_IN_UBO = 0x8;

//GS enabled?
//Affects vertex shader (i.e. need to compile vertex shader again to use with/without GS).
//This flag is needed in order for the interfaces between stages to match when GS is in use.
//PS inputs VtxGeoOutput
//GS outputs VtxGeoOutput
//Vs outputs VtxOutput if GS enabled. VtxGeoOutput otherwise.
static const unsigned int HLSLCC_FLAG_GS_ENABLED = 0x10;

static const unsigned int HLSLCC_FLAG_TESS_ENABLED = 0x20;

//Either use this flag or glBindFragDataLocationIndexed.
//When set the first pixel shader output is the first input to blend
//equation, the others go to the second input.
static const unsigned int HLSLCC_FLAG_DUAL_SOURCE_BLENDING = 0x40;

//If set, shader inputs and outputs are declared with their semantic name.
static const unsigned int HLSLCC_FLAG_INOUT_SEMANTIC_NAMES = 0x80;
//If set, shader inputs and outputs are declared with their semantic name appended.
static const unsigned int HLSLCC_FLAG_INOUT_APPEND_SEMANTIC_NAMES = 0x100;

//If set, combines texture/sampler pairs used together into samplers named "texturename_X_samplername".
static const unsigned int HLSLCC_FLAG_COMBINE_TEXTURE_SAMPLERS = 0x200;

//If set, attribute and uniform explicit location qualifiers are disabled (even if the language version supports that)
static const unsigned int HLSLCC_FLAG_DISABLE_EXPLICIT_LOCATIONS = 0x400;

//If set, global uniforms are not stored in a struct.
static const unsigned int HLSLCC_FLAG_DISABLE_GLOBALS_STRUCT = 0x800;

//If set, image declarations will always have binding and format qualifiers.
static const unsigned int HLSLCC_FLAG_GLES31_IMAGE_QUALIFIERS = 0x1000;

// If set, treats sampler names ending with _highp, _mediump, and _lowp as sampler precision qualifiers
// Also removes that prefix from generated output
static const unsigned int HLSLCC_FLAG_SAMPLER_PRECISION_ENCODED_IN_NAME = 0x2000;

// If set, adds location qualifiers to intra-shader varyings.
static const unsigned int HLSLCC_FLAG_SEPARABLE_SHADER_OBJECTS = 0x4000;

// If set, wraps all uniform buffer declarations in a preprocessor macro #ifndef HLSLCC_DISABLE_UNIFORM_BUFFERS
// so that if that macro is defined, all UBO declarations will become normal uniforms
static const unsigned int HLSLCC_FLAG_WRAP_UBO = 0x8000;

// If set, skips all members of the $Globals constant buffer struct that are not referenced in the shader code
static const unsigned int HLSLCC_FLAG_REMOVE_UNUSED_GLOBALS = 0x10000;

#define HLSLCC_TRANSLATE_MATRIX_FORMAT_STRING "hlslcc_mtx%dx%d"

// If set, translates all matrix declarations into vec4 arrays (as the DX bytecode treats them), and prefixes the name with 'hlslcc_mtx<rows>x<cols>'
static const unsigned int HLSLCC_FLAG_TRANSLATE_MATRICES = 0x20000;

// If set, emits Vulkan-style (set, binding) bindings, also captures that info from any declaration named "<Name>_hlslcc_set_X_bind_Y"
// Unless bindings are given explicitly, they are allocated into set 0 (map stored in GLSLCrossDependencyData)
static const unsigned int HLSLCC_FLAG_VULKAN_BINDINGS = 0x40000;

// If set, metal output will use linear sampler for shadow compares, otherwise point sampler.
static const unsigned int HLSLCC_FLAG_METAL_SHADOW_SAMPLER_LINEAR = 0x80000;

// write layout(offset = ...) for uniform members
static const unsigned int HLSLCC_FLAG_GLSL_EXPLICIT_UNIFORM_OFFSET = 0x100000;
// instead of putting it in a separate buffer slot, put the atomic counter inside
// the buffer at the beginning
static const unsigned int HLSLCC_FLAG_GLSL_EMBEDED_ATOMIC_COUNTER = 0x200000;
static const unsigned int HLSLCC_FLAG_GLSL_NO_FORMAT_FOR_IMAGE_WRITES = 0x400000;
static const unsigned int HLSLCC_FLAG_GLSL_CHECK_DATA_LAYOUT = 0x800000;
static const unsigned int HLSLCC_FLAG_MAP_CONST_BUFFER_TO_REGISTER_ARRAY = 0x1000000;

#ifdef __cplusplus
extern "C" {
#endif

HLSLCC_API int HLSLCC_APIENTRY TranslateHLSLFromFile(const char* filename,
                                                     unsigned int flags,
                                                     GLLang language,
                                                     const GlExtensions *extensions,
                                                     GLSLCrossDependencyDataInterface* dependencies,
                                                     HLSLccSamplerPrecisionInfo& samplerPrecisions,
                                                     HLSLccReflection& reflectionCallbacks,
                                                     std::stringstream& info_log,
                                                     GLSLShader* result);

HLSLCC_API int HLSLCC_APIENTRY TranslateHLSLFromMem(const char* shader,
                                                    unsigned int flags,
                                                    GLLang language,
                                                    const GlExtensions *extensions,
                                                    GLSLCrossDependencyDataInterface* dependencies,
                                                    HLSLccSamplerPrecisionInfo& samplerPrecisions,
                                                    HLSLccReflection& reflectionCallbacks,
                                                    std::stringstream& info_log,
                                                    GLSLShader* result);

#ifdef __cplusplus
}
#endif

#endif

