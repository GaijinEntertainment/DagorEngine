
#pragma once

#include <vector>
#include <string>
#include <map>
#include <array>
#include <limits>
#include <bitset>

#include "growing_array.h"
#include "internal_includes/tokens.h"
#include "internal_includes/reflect.h"
#include "include/ShaderInfo.h"
#include "internal_includes/Instruction.h"
#include "internal_includes/Declaration.h"
#include "internal_includes/ControlFlowGraph.h"
#include "internal_includes/UseDefineChains.h"
#include "bstrlib.h"


struct ConstantArrayChunk
{
  ConstantArrayChunk() = default;
  ConstantArrayChunk(uint32_t sz, uint32_t mask, Operand *firstUse)
    : m_Size(sz), m_AccessMask(mask)
  {
    m_UseSites.push_back(firstUse);
  }

  uint32_t m_Size = 0;
  uint32_t m_AccessMask = 0;
  uint32_t m_Rebase = 0;
  uint32_t m_ComponentCount = 0;

  std::vector<Operand *> m_UseSites;
};
typedef std::multimap<uint32_t, ConstantArrayChunk> ChunkMap;

struct ConstantArrayInfo
{
  ConstantArrayInfo() = default;

  Declaration *m_OrigDeclaration = nullptr; // Pointer to the original declaration of the const array
  ChunkMap m_Chunks; // map of <starting offset, chunk info>, same start offset might have multiple entries for different access masks
};

class ShaderPhase
{
public:
  ShaderPhase() = default;

  void ResolveUAVProperties();

  void UnvectorizeImmMoves(); // Transform MOV tX.xyz, (0, 1, 2) into MOV tX.x, 0; MOV tX.y, 1; MOV tX.z, 2 to make datatype analysis easier

  void PruneConstArrays(); // Walk through everything that accesses a const array to see if we could make it smaller

  void ExpandSWAPCs(); // Expand all SWAPC opcodes into a bunch of MOVCs. Must be done first!

  ConstantArrayInfo m_ConstantArrayInfo;

  std::vector<Declaration> psDecl;
  std::vector<Instruction> psInst;

  SHADER_PHASE_TYPE ePhase = MAIN_PHASE;
  uint32_t ui32InstanceCount = 0; // In case of hull shaders, how many instances this phase needs to have. Defaults to 1.
  bstring postShaderCode = nullptr;//End of main or before emit()
  int hasPostShaderCode = 0;

  bstring earlyMain = nullptr;//Code to be inserted at the start of phase

  uint32_t ui32OrigTemps = 0; // The number of temporaries this phase originally declared
  uint32_t ui32TotalTemps = 0; // The number of temporaries this phase has now
  std::vector<Declaration>::iterator psTempDeclaration; // Shortcut to the OPCODE_DCL_TEMPS opcode

  std::vector<SplitInfo> pui32SplitInfo;
  std::vector<SHADER_VARIABLE_TYPE> peTempTypes;

  // These are needed in cases we have 2 vec2 texcoords combined into one vec4 and they are accessed together.
  std::vector<unsigned char> acInputNeedsRedirect; // If 0xff, requires re-routing all reads via a combined vec4. If 0xfe, the same but the vec4 has already been declared.
  std::vector<unsigned char> acOutputNeedsRedirect; // Same for outputs
  std::vector<unsigned char> acPatchConstantsNeedsRedirect; // Same for patch constants

  // Get the Control Flow Graph for this phase, build it if necessary.
  HLSLcc::ControlFlow::ControlFlowGraph &GetCFG();

  uint32_t m_NextFreeTempRegister = 1; // A counter for creating new temporaries for for-loops.
  uint32_t m_NextTexCoordTemp = 0; // A counter for creating tex coord temps for driver issue workarounds

private:
  bool m_CFGInitialized = false;
  HLSLcc::ControlFlow::ControlFlowGraph m_CFG;
};

class Shader
{
public:

  Shader() = default;

  // Retrieve the number of components the temp register has.
  uint32_t GetTempComponentCount(SHADER_VARIABLE_TYPE eType, uint32_t ui32Reg) const;

  //Hull shaders have multiple phases.
  //Each phase has its own temps.
  //Convert from per-phase temps to global temps.
  void ConsolidateHullTempVars();

  // Go through all declarations and remove UAV occupied binding points from the aui32StructuredBufferBindingPoints list
  void ResolveStructuredBufferBindingSlots(ShaderPhase *psPhase);

  // HLSL has separate register spaces for UAV and structured buffers. GLSL has shared register space for all buffers.
  // The aim here is to preserve the UAV buffer bindings as they are and use remaining binding points for structured buffers.
  // In this step make aui32StructuredBufferBindingPoints contain increasingly ordered uints starting from zero.
  void PrepareStructuredBufferBindingSlots();

  // Detect temp registers per data type that are actually used.
  void PruneTempRegisters();

  // Check if inputs and outputs are accessed across semantic boundaries
  // as in, 2x texcoord vec2's are packed together as vec4 but still accessed together.
  void AnalyzeIOOverlap();

  // Change all references to vertex position to always be highp, having them be mediump causes problems on Metal and Vivante GPUs.
  void ForcePositionToHighp();

  void FindUnusedGlobals(uint32_t flags); // Finds the DCL_CONSTANT_BUFFER with name "$Globals" and searches through all usages for each member of it and mark if they're actually ever used.

  void ExpandSWAPCs();

  uint32_t ui32MajorVersion = 0;
  uint32_t ui32MinorVersion = 0;
  SHADER_TYPE eShaderType = INVALID_SHADER;

  GLLang eTargetLanguage = LANG_DEFAULT;
  const struct GlExtensions *extensions = nullptr;

  int fp64 = 0;

  //DWORDs in program code, including version and length tokens.
  uint32_t ui32ShaderLength = 0;


  //Instruction* functions;//non-main subroutines
  HLSLcc::growing_vector<uint32_t> aui32FuncTableToFuncPointer; // dynamic alloc?
  HLSLcc::growing_vector<uint32_t> aui32FuncBodyToFuncTable;

  struct FuncTableEntry {
    HLSLcc::growing_vector<uint32_t> aui32FuncBodies;
  };
  HLSLcc::growing_vector<FuncTableEntry> funcTable;

  struct FuncPointerEntry {
    HLSLcc::growing_vector<uint32_t> aui32FuncTables;
    uint32_t ui32NumBodiesPerTable = 0;
  };

  HLSLcc::growing_vector<FuncPointerEntry> funcPointer;

  HLSLcc::growing_vector<uint32_t> ui32NextClassFuncName;

  const uint32_t* pui32FirstToken = nullptr;//Reference for calculating current position in token stream.

  std::vector<ShaderPhase> asPhases;

  ShaderInfo sInfo;

  // There are 2 input/output register spaces in DX bytecode: one for per-patch data and one for per-vertex.
  // Which one is used depends on the context:
  // per-vertex space is used in vertex/pixel/geom shaders always
  // hull shader control point phase uses per-vertex by default, other phases are per-patch by default (can access per-vertex with OPERAND_TYPE_I/O_CONTROL_POINT)
  // domain shader is per-patch by default, can access per-vertex with OPERAND_TYPE_I/O_CONTROL_POINT

  // Below, the [2] is accessed with 0 == per-vertex, 1 == per-patch
  // Note that these ints are component masks
  HLSLcc::growing_vector<int> abScalarInput[2];
  HLSLcc::growing_vector<int> abScalarOutput[2];

  HLSLcc::growing_vector<int> aIndexedInput[2];
  HLSLcc::growing_vector<bool> aIndexedOutput[2];

  HLSLcc::growing_vector<int> aIndexedInputParents[2];

  HLSLcc::growing_vector<RESOURCE_DIMENSION> aeResourceDims;

  HLSLcc::growing_vector<char> acInputDeclared[2];
  HLSLcc::growing_vector<char> acOutputDeclared[2];

  std::bitset<NUM_OPCODES> aiOpcodeUsed = {0}; // Initialized to NUM_OPCODES elements above.

  uint32_t ui32CurrentVertexOutputStream = 0;

  TextureSamplerPairs textureSamplers;

  std::array<uint32_t, MAX_RESOURCE_BINDINGS> aui32StructuredBufferBindingPoints = {};
  uint32_t ui32CurrentStructuredBufferIndex = 0;

  std::vector<char> psIntTempSizes; // Array for whether this temp register needs declaration as int temp
  std::vector<char> psInt16TempSizes; // min16ints
  std::vector<char> psInt12TempSizes; // min12ints
  std::vector<char> psUIntTempSizes; // Same for uints
  std::vector<char> psUInt16TempSizes; // ... and for uint16's
  std::vector<char> psFloatTempSizes; // ...and for floats
  std::vector<char> psFloat16TempSizes; // ...and for min16floats
  std::vector<char> psFloat10TempSizes; // ...and for min10floats
  std::vector<char> psDoubleTempSizes; // ...and for doubles
  std::vector<char> psBoolTempSizes; // ... and for bools

private:
  void DoIOOverlapOperand(ShaderPhase *psPhase, Operand *psOperand);

};
