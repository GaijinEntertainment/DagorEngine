#pragma once

#include <set>
#include <unordered_set>
#include <unordered_map>
#include <map>
#include <limits>
#include <list>
#include <vector>
#include <algorithm>

#include <stdint.h>
#include <string.h>

/*
 * this crap should be renamed into *producer/*consumer to make it less confusing
 */

struct DefineUseChainEntry;
struct UseDefineChainEntry;

typedef std::unordered_set<DefineUseChainEntry *> DefineSet;
typedef std::unordered_set<UseDefineChainEntry *> UsageSet;

struct Instruction;
class Operand;
class ShaderInfo;
namespace HLSLcc
{
namespace ControlFlow
{
class ControlFlowGraph;
};
};


// Def-Use chain per temp component
struct DefineUseChainEntry
{
  Instruction *psInst = nullptr;			// The declaration (write to this temp component)
  Operand *psOp = nullptr;					// The operand within this instruction for the write target
  UsageSet usages;				// List of usages that are dependent on this write
  uint32_t writeMask = 0;				// Access mask; which all components were written to in the same op 
  uint32_t index = 0;					// For which component was this definition created for?
  uint32_t isStandalone = 0;			// A shortcut for analysis: if nonzero, all siblings of all usages for both this and all this siblings
  DefineUseChainEntry *psSiblings[4] = {};  // In case of vectorized op, contains pointer to this define's corresponding entries for the other components.

#if _DEBUG
  bool operator==(const DefineUseChainEntry &a) const
  {
    if (psInst != a.psInst)
      return false;
    if (psOp != a.psOp)
      return false;
    if (writeMask != a.writeMask)
      return false;
    if (index != a.index)
      return false;
    if (isStandalone != a.isStandalone)
      return false;

    // Just check that each one has the same amount of usages
    if (usages.size() != a.usages.size())
      return false;

    return true;
  }
#endif
};

typedef std::list<DefineUseChainEntry> DefineUseChain;

struct UseDefineChainEntry
{
  Instruction *psInst = nullptr;			// The use (read from this temp component)
  Operand *psOp = nullptr;					// The operand within this instruction for the read
  DefineSet defines;				// List of writes that are visible to this read
  uint32_t accessMask = 0;			// Which all components were read together with this one
  uint32_t index = 0;					// For which component was this usage created for?
  UseDefineChainEntry *psSiblings[4] = {};  // In case of vectorized op, contains pointer to this usage's corresponding entries for the other components.

#if _DEBUG
  bool operator==(const UseDefineChainEntry &a) const
  {
    if (psInst != a.psInst)
      return false;
    if (psOp != a.psOp)
      return false;
    if (accessMask != a.accessMask)
      return false;
    if (index != a.index)
      return false;

    // Just check that each one has the same amount of usages
    if (defines.size() != a.defines.size())
      return false;

    return true;
  }
#endif
};

struct SplitInfo
{
  uint16_t  originalRegister = std::numeric_limits<uint16_t>::max();
  uint8_t   componentCount = std::numeric_limits<uint8_t>::max();
  uint8_t   rebase = std::numeric_limits<uint8_t>::max();
};

inline bool operator==(SplitInfo l, SplitInfo r)
{
  return l.originalRegister == r.originalRegister && l.componentCount == r.componentCount && l.rebase == r.rebase;
}

inline bool operator!=(SplitInfo l, SplitInfo r)
{
  return !(l == r);
}

typedef std::list<UseDefineChainEntry> UseDefineChain;

typedef std::unordered_map<uint32_t, UseDefineChain> UseDefineChains;
typedef std::unordered_map<uint32_t, DefineUseChain> DefineUseChains;
typedef std::vector<DefineUseChainEntry *> ActiveDefinitions;

// Do flow control analysis on the instructions and build the define-use and use-define chains
void BuildUseDefineChains(uint32_t ui32NumTemps, DefineUseChains &psDUChains, UseDefineChains &psUDChains, HLSLcc::ControlFlow::ControlFlowGraph &cfg);

// Do temp splitting based on use-define chains
void UDSplitTemps(uint32_t *psNumTemps, DefineUseChains &psDUChains, UseDefineChains &psUDChains, std::vector<SplitInfo> &pui32SplitTable);

// Based on the sampler precisions, downgrade the definitions if possible.
void UpdateSamplerPrecisions(const ShaderInfo &psContext, DefineUseChains &psDUChains, uint32_t ui32NumTemps);

// Optimization pass for successive passes: Mark Operand->isStandalone for definitions that are "standalone": all usages (and all their sibligns) of this and all its siblings only see this definition.
void CalculateStandaloneDefinitions(DefineUseChains &psDUChains, uint32_t ui32NumTemps);

// Write the uses and defines back to Instruction and Operand member lists.
void WriteBackUsesAndDefines(DefineUseChains &psDUChains);

