
#include "internal_includes/UseDefineChains.h"
#include "internal_includes/debug.h"
#include "internal_includes/Instruction.h"

#include "internal_includes/ControlFlowGraph.h"
#include "internal_includes/debug.h"
#include "internal_includes/HLSLccToolkit.h"
#include <algorithm>

using HLSLcc::ForEachOperand;

#define DEBUG_UDCHAINS 0

#if DEBUG_UDCHAINS
// Debug mode
static void UDCheckConsistencyDUChain(uint32_t idx, DefineUseChains &psDUChains, UseDefineChains &psUDChains, ActiveDefinitions &activeDefinitions)
{
	DefineUseChain::iterator du = psDUChains[idx].begin();
	UseDefineChain::iterator ud = psUDChains[idx].begin();
	while (du != psDUChains[idx].end())
	{
		ASSERT(du->index == idx % 4);
		// Check that the definition actually writes to idx
		{
			uint32_t tempReg = idx / 4;
			uint32_t offs = idx - (tempReg * 4);
			uint32_t accessMask = 1 << offs;
			uint32_t i;
			int found = 0;
			for (i = 0; i < du->psInst->ui32FirstSrc; i++)
			{
				if (du->psInst->asOperands[i].eType == OPERAND_TYPE_TEMP)
				{
					if (du->psInst->asOperands[i].ui32RegisterNumber == tempReg)
					{
						uint32_t writeMask = du->psInst->asOperands[i].GetAccessMask();
						if (writeMask & accessMask)
						{
							ASSERT(writeMask == du->writeMask);
							found = 1;
							break;
						}
					}
				}
			}
			ASSERT(found);
		}

		// Check that each usage of each definition also is found in the use-define chain
		UsageSet::iterator ul = du->usages.begin();
		while (ul != du->usages.end())
		{
			// Search for the usage in the chain
			UseDefineChain::iterator use = ud;
			while (use != psUDChains[idx].end() && &*use != *ul)
				use++;
			ASSERT(use != psUDChains[idx].end());
			ASSERT(&*use == *ul);

			// Check that the mapping back is also found
			ASSERT(std::find(use->defines.begin(), use->defines.end(), &*du) != use->defines.end());

			ul++;
		}

		du++;
	}
}

static void UDCheckConsistencyUDChain(uint32_t idx, DefineUseChains &psDUChains, UseDefineChains &psUDChains, ActiveDefinitions &activeDefinitions)
{
	DefineUseChain::iterator du = psDUChains[idx].begin();
	UseDefineChain::iterator ud = psUDChains[idx].begin();
	while (ud != psUDChains[idx].end())
	{
		// Check that each definition of each usage also is found in the define-use chain
		DefineSet::iterator dl = ud->defines.begin();
		ASSERT(ud->psOp->ui32RegisterNumber == idx / 4);
		ASSERT(ud->index == idx % 4);
		while (dl != ud->defines.end())
		{
			// Search for the definition in the chain
			DefineUseChain::iterator def = du;
			while (def != psDUChains[idx].end() && &*def != *dl)
				def++;
			ASSERT(def != psDUChains[idx].end());
			ASSERT(&*def == *dl);

			// Check that the mapping back is also found
			ASSERT(std::find(def->usages.begin(), def->usages.end(), &*ud) != def->usages.end());

			dl++;
		}
		ud++;
	}

}

static void UDCheckConsistency(uint32_t tempRegs, DefineUseChains &psDUChains, UseDefineChains &psUDChains, ActiveDefinitions &activeDefinitions)
{
	uint32_t i;
	for (i = 0; i < tempRegs * 4; i++)
	{
		UDCheckConsistencyDUChain(i, psDUChains, psUDChains, activeDefinitions);
		UDCheckConsistencyUDChain(i, psDUChains, psUDChains, activeDefinitions);
	}
}

#define printf_console printf

#endif

using namespace HLSLcc::ControlFlow;
using std::for_each;

static DefineUseChainEntry *GetOrCreateDefinition(const BasicBlock::Definition &def, DefineUseChain &psDUChain, uint32_t index)
{
  // Try to find an existing entry
  auto itr = std::find_if(psDUChain.begin(), psDUChain.end(), [&] (const DefineUseChainEntry &de)
  {
    return de.psInst == def.m_Instruction && de.psOp == def.m_Operand;
  });

  if (itr != psDUChain.end())
  {
    return &(*itr);
  }

  // Not found, create
  psDUChain.push_front(DefineUseChainEntry());
  DefineUseChainEntry &de = psDUChain.front();

  de.psInst = (Instruction *)def.m_Instruction;
  de.psOp = (Operand *)def.m_Operand;
  de.index = index;
  de.writeMask = def.m_Operand->GetAccessMask();
  de.psSiblings[index] = &de;

  return &de;
}

// Do flow control analysis on the instructions and build the define-use and use-define chains
void BuildUseDefineChains(uint32_t ui32NumTemps, DefineUseChains &psDUChain, UseDefineChains &psUDChain, HLSLcc::ControlFlow::ControlFlowGraph &cfg)
{
  ActiveDefinitions lastSeenDefinitions(ui32NumTemps * 4, NULL); // Array of pointers to the currently active definition for each temp


  for (uint32_t i = 0; i < ui32NumTemps * 4; i++)
  {
    psUDChain.insert(std::make_pair(i, UseDefineChain()));
    psDUChain.insert(std::make_pair(i, DefineUseChain()));
  }

  const ControlFlowGraph::BasicBlockStorage &blocks = cfg.AllBlocks();

  // Loop through each block, first calculate the union of all the reachables of all preceding blocks
  // and then build on that as we go along the basic block instructions
  for (auto&& bptr : blocks)
  {
    const BasicBlock &b = *bptr.get();
    BasicBlock::ReachableVariables rvars;
    for (auto&& precBlock : b.Preceding())
      BasicBlock::RVarUnion(rvars, cfg.GetBasicBlockForInstruction(precBlock)->Reachable());

    // Now we have a Reachable set for the beginning of this block in rvars. Loop through all instructions and their operands and pick up uses and definitions
    for (const Instruction *inst = b.First(); inst <= b.Last(); ++inst)
    {
      // Process sources first
      ForEachOperand(inst,
                     inst + 1,
                     FEO_FLAG_SRC_OPERAND | FEO_FLAG_SUBOPERAND,
                     [&](const Instruction *psInst, const Operand *psOperand, uint32_t /*ui32OperandType*/)
      {
        if (psOperand->eType != OPERAND_TYPE_TEMP)
          return;

        const uint32_t tempReg = psOperand->ui32RegisterNumber;
        const uint32_t accessMask = psOperand->GetAccessMask();

        // Go through each component
        for (int k = 0; k < 4; k++)
        {
          if (!(accessMask & (1 << k)))
            continue;

          const uint32_t regIdx = tempReg * 4 + k;

          // Add an use for all visible definitions
          psUDChain[regIdx].push_front(UseDefineChainEntry());
          UseDefineChainEntry &ue = psUDChain[regIdx].front();
          ue.psInst = (Instruction *)psInst;
          ue.psOp = (Operand *)psOperand;
          ue.accessMask = accessMask;
          ue.index = k;
          ue.psSiblings[k] = &ue;
          // ue.siblings will be filled out later.

          BasicBlock::ReachableDefinitionsPerVariable& rpv = rvars[regIdx];
          for (auto&& def : rpv)
          {
            DefineUseChainEntry *duentry = GetOrCreateDefinition(def, psDUChain[regIdx], k);
            ue.defines.insert(duentry);
            duentry->usages.insert(&ue);
          }
        }
      });

      // Then the destination operands
      ForEachOperand(inst, inst + 1, FEO_FLAG_DEST_OPERAND,
                     [&](const Instruction *psInst, const Operand *psOperand, uint32_t /*ui32OperandType*/)
      {
        if (psOperand->eType != OPERAND_TYPE_TEMP)
          return;

        uint32_t tempReg = psOperand->ui32RegisterNumber;
        uint32_t accessMask = psOperand->GetAccessMask();

        // Go through each component
        for (int k = 0; k < 4; k++)
        {
          if (!(accessMask & (1 << k)))
            continue;

          uint32_t regIdx = tempReg * 4 + k;

          // Overwrite whatever's in rvars; they are killed by this
          rvars[regIdx].clear();
          rvars[regIdx].insert(BasicBlock::Definition(psInst, psOperand));

          // Make sure the definition gets created even though it doesn't have any uses at all
          // (happens when sampling a texture but not all channels are used etc).
          GetOrCreateDefinition(BasicBlock::Definition(psInst, psOperand), psDUChain[regIdx], k);
        }
      });
    }
  }

  // Connect the siblings for all uses and definitions
  for (auto&& udpair : psUDChain)
  {
    UseDefineChain &ud = udpair.second;
    // Clear out the bottom 2 bits to get the actual base reg
    const uint32_t baseReg = udpair.first & ~(3);

    for (auto&& ue : ud)
    {
      ASSERT(baseReg / 4 == ue.psOp->ui32RegisterNumber);

      // Go through each component
      for (uint32_t k = 0; k < 4; k++)
      {
        // Skip components that we don't access, or the one that's our own
        if (!(ue.accessMask & (1 << k)) || ue.index == k)
          continue;

        // Find the corresponding sibling. We can uniquely identify it by the operand pointer alone.
        auto siblItr = find_if(psUDChain[baseReg + k].begin(), psUDChain[baseReg + k].end(), [&](const UseDefineChainEntry &_sibl) { return _sibl.psOp == ue.psOp; });
        ASSERT(siblItr != psUDChain[baseReg + k].end());
        ue.psSiblings[k] = &*siblItr;
      }
    }
  }

  // Same for definitions
  for (auto&& dupair : psDUChain)
  {
    DefineUseChain &du = dupair.second;
    // Clear out the bottom 2 bits to get the actual base reg
    const uint32_t baseReg = dupair.first & ~(3);

    for (auto&& de : du)
    {
      ASSERT(baseReg / 4 == de.psOp->ui32RegisterNumber);

      // Go through each component
      for (uint32_t k = 0; k < 4; k++)
      {
        // Skip components that we don't access, or the one that's our own
        if (!(de.writeMask & (1 << k)) || de.index == k)
          continue;

        // Find the corresponding sibling. We can uniquely identify it by the operand pointer alone.
        auto siblItr = find_if(psDUChain[baseReg + k].begin(), psDUChain[baseReg + k].end(), [&](const DefineUseChainEntry &_sibl) { return _sibl.psOp == de.psOp; });
        ASSERT(siblItr != psDUChain[baseReg + k].end());
        de.psSiblings[k] = &*siblItr;
      }
    }
  }

#if DEBUG_UDCHAINS
  UDCheckConsistency(ui32NumTemps, psDUChain, psUDChain, lastSeenDefinitions);
#endif
}


typedef std::vector<DefineUseChainEntry *> SplitDefinitions;

// Split out a define to use a new temp register
static void UDDoSplit(SplitDefinitions &defs, uint32_t *psNumTemps, DefineUseChains &psDUChains, UseDefineChains &psUDChains, std::vector<SplitInfo> &pui32SplitTable)
{
	uint32_t newReg = *psNumTemps;
	uint32_t oldReg = defs[0]->psOp->ui32RegisterNumber;
	uint32_t accessMask = defs[0]->writeMask;
	uint32_t i, u32def;

	ASSERT(defs.size() > 0);
	for (i = 1; i < defs.size(); i++)
	{
		ASSERT(defs[i]->psOp->ui32RegisterNumber == oldReg);
		accessMask |= defs[i]->writeMask;
	}


	(*psNumTemps)++;


#if DEBUG_UDCHAINS
	UDCheckConsistency((*psNumTemps) - 1, psDUChains, psUDChains, ActiveDefinitions());
#endif
	ASSERT(accessMask != 0 && accessMask <= 0xf);
	// Calculate rebase value and component count
  uint8_t rebase = 0;
  uint8_t count = 0;
	i = accessMask;
	while ((i & 1) == 0)
	{
		rebase++;
		i = i >> 1;
	}
	while (i != 0)
	{
		count++;
		i = i >> 1;
	}

	// Make sure there's enough room in the split table
	if (pui32SplitTable.size() <= newReg)
	{
		size_t newSize = pui32SplitTable.size() * 2;
		pui32SplitTable.resize(newSize);
	}

	// Set the original temp of the new register
	{
		uint32_t origTemp = oldReg;
    while (pui32SplitTable[origTemp] != SplitInfo{})
      origTemp = pui32SplitTable[origTemp].originalRegister;

    pui32SplitTable[newReg].componentCount = count;
    pui32SplitTable[newReg].rebase = rebase;
    pui32SplitTable[newReg].originalRegister = (uint16_t)origTemp;
	}

	// Insert the new temps to the map
	for (i = newReg * 4; i < newReg * 4 + 4; i++)
	{
		psUDChains.insert(std::make_pair(i, UseDefineChain()));
		psDUChains.insert(std::make_pair(i, DefineUseChain()));
	}

	for (u32def = 0; u32def < defs.size(); u32def++)
	{
		DefineUseChainEntry *defineToSplit = defs[u32def];
		uint32_t oldIdx = defineToSplit->index;
#if DEBUG_UDCHAINS
		printf("Split def at instruction %p (reg %d -> %d, access %X, rebase %d, count: %d)\n", defineToSplit->psInst, oldReg, newReg, accessMask, rebase, count);
#endif

		// We may have moved the opcodes already because of multiple defines pointing to the same op
		if (defineToSplit->psOp->ui32RegisterNumber != newReg)
		{
			ASSERT(defineToSplit->psOp->ui32RegisterNumber == oldReg);
			// Update the declaration operand
			// Don't change possible suboperands as they are sources
			defineToSplit->psInst->ChangeOperandTempRegister(defineToSplit->psOp, oldReg, newReg, accessMask, UD_CHANGE_MAIN_OPERAND, rebase);
		}

		defineToSplit->writeMask >>= rebase;
		defineToSplit->index -= rebase;
		// Change the temp register number for all usages
		UsageSet::iterator ul = defineToSplit->usages.begin();
		while (ul != defineToSplit->usages.end())
		{
			// Already updated by one of the siblings? Skip.
			if ((*ul)->psOp->ui32RegisterNumber != newReg)
			{
				ASSERT((*ul)->psOp->ui32RegisterNumber == oldReg);
				(*ul)->psInst->ChangeOperandTempRegister((*ul)->psOp, oldReg, newReg, accessMask, UD_CHANGE_MAIN_OPERAND, rebase);
			}

			// Update the UD chain
			{
				UseDefineChain::iterator udLoc = psUDChains[oldReg * 4 + oldIdx].begin();
				while (udLoc != psUDChains[oldReg * 4 + oldIdx].end())
				{
					if (&*udLoc == *ul)
					{
						// Move to new list
						psUDChains[newReg * 4 + oldIdx - rebase].splice(psUDChains[newReg * 4 + oldIdx - rebase].begin(), psUDChains[oldReg * 4 + oldIdx], udLoc);

						if (rebase > 0)
						{
							(*ul)->accessMask >>= rebase;
							(*ul)->index -= rebase;
							memmove((*ul)->psSiblings, (*ul)->psSiblings + rebase, (4 - rebase) * sizeof(UseDefineChain *));
						}
						break;
					}
					udLoc++;
				}
			}

			ul++;
		}

		// Move the define out of the old chain (if its still there)
		{
			// Find the define in the old chain
			DefineUseChain::iterator duLoc = psDUChains[oldReg * 4 + oldIdx].begin();
			while (duLoc != psDUChains[oldReg * 4 + oldIdx].end() && ((&*duLoc) != defineToSplit))
			{
				duLoc++;
			}
			ASSERT(duLoc != psDUChains[oldReg * 4 + oldIdx].end());
			{
				// Move directly to new chain
				psDUChains[newReg * 4 + oldIdx - rebase].splice(psDUChains[newReg * 4 + oldIdx - rebase].begin(), psDUChains[oldReg * 4 + oldIdx], duLoc);
				if (rebase != 0)
				{
					memmove(defineToSplit->psSiblings, defineToSplit->psSiblings + rebase, (4 - rebase) * sizeof(DefineUseChain *));
				}
			}

		}

	}

#if DEBUG_UDCHAINS
	UDCheckConsistency(*psNumTemps, psDUChains, psUDChains, ActiveDefinitions());
#endif
}

template<typename C, typename V>
void push_back_unique(C& c, const V& v)
{
  auto ref = find(c.begin(), c.end(), v);
  if (ref == c.end())
    c.push_back(v);
}

// Adds a define and all its siblings to the list, checking duplicates
static void AddDefineToList(SplitDefinitions &defs, DefineUseChainEntry *newDef)
{
  for (auto&& defToAdd : newDef->psSiblings)
    if (defToAdd)
      push_back_unique(defs, defToAdd);
}

// Check if a set of definitions can be split and does the split. Returns nonzero if a split took place
static int AttemptSplitDefinitions(SplitDefinitions &defs, uint32_t *psNumTemps, DefineUseChains &psDUChains, UseDefineChains &psUDChains, std::vector<SplitInfo> &pui32SplitTable)
{
	uint32_t i, k, u32def;
	DefineUseChain::iterator du;
	// Initial checks: all definitions must:
	// Access the same register
	// Have at least one definition in any of the 4 register slots that isn't included
	if (defs.empty())
		return 0;

	uint32_t reg = defs[0]->psOp->ui32RegisterNumber;
	uint32_t combinedMask = defs[0]->writeMask;
	for (i = 1; i < defs.size(); i++)
	{
		if (reg != defs[i]->psOp->ui32RegisterNumber)
			return 0;

		combinedMask |= defs[i]->writeMask;
	}

  bool hasLeftoverDefinitions = false;
	for (i = 0; i < 4; i++)
	{
		du = psDUChains[reg * 4 + i].begin();
		while (du != psDUChains[reg * 4 + i].end())
		{
			int defFound = 0;
			for (k = 0; k < defs.size(); k++)
			{
				if (&*du == defs[k])
				{
					defFound = 1;
					break;
				}
			}
			if (defFound == 0)
			{
				hasLeftoverDefinitions = true;
				break;
			}
			du++;
		}
		if (hasLeftoverDefinitions)
			break;
	}
	// We'd be splitting the entire register and all its definitions, no point in that.
	if (hasLeftoverDefinitions == false)
		return 0;

  bool canSplit = true;
	// Check all the definitions. Any of them must not have any usages that see any definitions not in our defs array.
	for (u32def = 0; u32def < defs.size(); u32def++)
	{
		DefineUseChainEntry *def = defs[u32def];

		UsageSet::iterator ul = def->usages.begin();
		while (ul != def->usages.end())
		{
			uint32_t j;

			// Check that we only read a subset of the combined writemask
			if (((*ul)->accessMask & (~combinedMask)) != 0)
			{
				// Do an additional attempt, pick up all the sibling definitions as well
				// Only do this if we have the space in the definitions table
				for (j = 0; j < 4; j++)
				{
					if (((*ul)->accessMask & (1 << j)) == 0)
						continue;
          ASSERT(!(*ul)->psSiblings[j]->defines.empty());
					AddDefineToList(defs, *(*ul)->psSiblings[j]->defines.begin());
				}
				return AttemptSplitDefinitions(defs, psNumTemps, psDUChains, psUDChains, pui32SplitTable);

			}

			// It must have at least one declaration
			ASSERT(!(*ul)->defines.empty());

			// Check that all siblings for the usage use one of the definitions
			for (j = 0; j < 4; j++)
			{
				uint32_t m;
				int defineFound = 0;
				if (((*ul)->accessMask & (1 << j)) == 0)
					continue;

				ASSERT((*ul)->psSiblings[j] != NULL);
				ASSERT(!(*ul)->psSiblings[j]->defines.empty());

				// Check that all definitions for this usage are found from the definitions table
				DefineSet::iterator dl = (*ul)->psSiblings[j]->defines.begin();
				while (dl != (*ul)->psSiblings[j]->defines.end())
				{
					defineFound = 0;
					for (m = 0; m < defs.size(); m++)
					{
						if (*dl == defs[m])
						{
							defineFound = 1;
							break;
						}
					}
					if (defineFound == 0)
					{
						// Add this define and all its siblings to the table and try again
						AddDefineToList(defs, *dl);
						return AttemptSplitDefinitions(defs, psNumTemps, psDUChains, psUDChains, pui32SplitTable);
					}

					dl++;
				}

				if (defineFound == 0)
				{
					canSplit = false;
					break;
				}
			}
			if (canSplit == false)
				break;

			// This'll do, check next usage
			ul++;
		}
		if (canSplit == false)
			break;

	}
	if (canSplit)
	{
		UDDoSplit(defs, psNumTemps, psDUChains, psUDChains, pui32SplitTable);
		return 1;
	}
	return 0;
}

// Do temp splitting based on use-define chains
void UDSplitTemps(uint32_t *psNumTemps, DefineUseChains &psDUChains, UseDefineChains &psUDChains, std::vector<SplitInfo> &pui32SplitTable)
{
	// Algorithm overview:
	// Take each definition and look at all its usages. If all usages only see this definition (and this is not the only definition for this variable),
	// split it out.
	uint32_t i;
	uint32_t tempsAtStart = *psNumTemps; // We don't need to try to analyze the newly created ones, they're unsplittable by definition
	for (i = 0; i < tempsAtStart * 4; i++)
	{
		// No definitions?
		if (psDUChains[i].empty())
			continue;

		DefineUseChain::iterator du = psDUChains[i].begin();
		// Ok we have multiple definitions for a temp, check them through
		while (du != psDUChains[i].end())
		{
			SplitDefinitions sd;
			AddDefineToList(sd, &*du);
			du++;
			// If we split, we'll have to start from the beginning of this chain because du might no longer be in this chain
			if (AttemptSplitDefinitions(sd, psNumTemps, psDUChains, psUDChains, pui32SplitTable))
			{
				du = psDUChains[i].begin();
			}
		}
	}
}

// Returns nonzero if all the operands have partial precision and at least one of them has been downgraded as part of shader downgrading process.
// Sampler ops, bitwise ops and comparisons are ignored.
static int CanDowngradeDefinitionPrecision(DefineUseChain::iterator du, OPERAND_MIN_PRECISION *pType)
{
	Instruction *psInst = du->psInst;
	int hasFullPrecOperands = 0;
	uint32_t i;

	if (du->psOp->eMinPrecision != OPERAND_MIN_PRECISION_DEFAULT)
		return 0;

	switch (psInst->eOpcode)
	{
	case OPCODE_ADD:
	case OPCODE_MUL:
	case OPCODE_MOV:
	case OPCODE_MAD:
	case OPCODE_DIV:
	case OPCODE_LOG:
	case OPCODE_EXP:
	case OPCODE_MAX:
	case OPCODE_MIN:
	case OPCODE_DP2:
	case OPCODE_DP2ADD:
	case OPCODE_DP3:
	case OPCODE_DP4:
	case OPCODE_RSQ:
	case OPCODE_SQRT:
		break;
	default:
		return 0;
	}

	for (i = psInst->ui32FirstSrc; i < psInst->ui32NumOperands; i++)
	{
		Operand *op = &psInst->asOperands[i];
		if (op->eType == OPERAND_TYPE_IMMEDIATE32)
			continue; // Immediate values are ignored

		if (op->eMinPrecision == OPERAND_MIN_PRECISION_DEFAULT)
		{
			hasFullPrecOperands = 1;
			break;
		}
	}

	if (hasFullPrecOperands)
		return 0;

	if (pType)
		*pType = OPERAND_MIN_PRECISION_FLOAT_16; // Don't go lower than mediump

	return 1;
}

// Returns true if all the usages of this definitions are instructions that deal with floating point data
static bool HasOnlyFloatUsages(DefineUseChain::iterator du)
{
	UsageSet::iterator itr = du->usages.begin();
	for (; itr != du->usages.end(); itr++)
	{
		Instruction *psInst = (*itr)->psInst;
	
		if ((*itr)->psOp->eMinPrecision != OPERAND_MIN_PRECISION_DEFAULT)
			return false;

		switch (psInst->eOpcode)
		{
		case OPCODE_ADD:
		case OPCODE_MUL:
		case OPCODE_MOV:
		case OPCODE_MAD:
		case OPCODE_DIV:
		case OPCODE_LOG:
		case OPCODE_EXP:
		case OPCODE_MAX:
		case OPCODE_MIN:
		case OPCODE_DP2:
		case OPCODE_DP2ADD:
		case OPCODE_DP3:
		case OPCODE_DP4:
		case OPCODE_RSQ:
		case OPCODE_SQRT:
			break;
		default:
			return false;
		}
	}
	return true;
}

// Based on the sampler precisions, downgrade the definitions if possible.
void UpdateSamplerPrecisions(const ShaderInfo &info, DefineUseChains &psDUChains, uint32_t ui32NumTemps)
{
	uint32_t madeProgress = 0;
	do
	{
		uint32_t i;
		madeProgress = 0;
		for (i = 0; i < ui32NumTemps * 4; i++)
		{
			DefineUseChain::iterator du = psDUChains[i].begin();
			while (du != psDUChains[i].end())
			{
				OPERAND_MIN_PRECISION sType = OPERAND_MIN_PRECISION_DEFAULT;
				if ((du->psInst->IsPartialPrecisionSamplerInstruction(info, &sType)
					|| CanDowngradeDefinitionPrecision(du, &sType))
					&& du->psInst->asOperands[0].eType == OPERAND_TYPE_TEMP
					&& du->psInst->asOperands[0].eMinPrecision == OPERAND_MIN_PRECISION_DEFAULT
					&& du->isStandalone
					&& HasOnlyFloatUsages(du))
				{
					uint32_t sibl;
					// Ok we can change the precision.
					ASSERT(du->psOp->eType == OPERAND_TYPE_TEMP);
					ASSERT(sType != OPERAND_MIN_PRECISION_DEFAULT);
					du->psOp->eMinPrecision = sType;

					// Update all the uses of all the siblings
					for (sibl = 0; sibl < 4; sibl++)
					{
						if (!du->psSiblings[sibl])
							continue;

						UsageSet::iterator ul = du->psSiblings[sibl]->usages.begin();
						while (ul != du->psSiblings[sibl]->usages.end())
						{
							ASSERT((*ul)->psOp->eMinPrecision == OPERAND_MIN_PRECISION_DEFAULT ||
								(*ul)->psOp->eMinPrecision == sType);
							// We may well write this multiple times to the same op but that's fine.
							(*ul)->psOp->eMinPrecision = sType;

							ul++;
						}
					}
					madeProgress = 1;
				}
				du++;
			}
		}
	} while (madeProgress != 0);

}

static bool IsStandalone(DefineUseChainEntry& du)
{
  if (du.isStandalone)
    return true;

  for (auto&& sibl : du.psSiblings)
  {
    if (!sibl)
      continue;

    for (auto&& ul : sibl->usages)
    {
      ASSERT(!ul->defines.empty());

      // Need to check that all the siblings of this usage only see this definition's corresponding sibling
      for (uint32_t k = 0; k < 4; k++)
      {
        if (!ul->psSiblings[k])
          continue;

        if (ul->psSiblings[k]->defines.size() > 1 ||
            *ul->psSiblings[k]->defines.begin() != du.psSiblings[k])
        {
          return false;
        }
      }
    }
  }
  return true;
}

void CalculateStandaloneDefinitions(DefineUseChains &psDUChains, uint32_t ui32NumTemps)
{
  (void)ui32NumTemps;
  for(auto&& chain : psDUChains)  // may use ui32NumTemps again
    for(auto&& du : chain.second)
      if (IsStandalone(du))
        for(auto&& sibl : du.psSiblings)
          if (sibl)
            sibl->isStandalone = 1;
}

// Write the uses and defines back to Instruction and Operand member lists.
void WriteBackUsesAndDefines(DefineUseChains &psDUChains)
{
  // Loop through the whole data structure, and write usages and defines to Instructions and Operands as we see them
  for(auto&& itr : psDUChains)
  {
    for(auto&& du : itr.second)
    {
      for (auto&& usage : du.usages)
      {
        // Update instruction use list
        du.psInst->m_Uses.push_back(Instruction::Use(usage->psInst, usage->psOp));
        // And the usage's definition
        usage->psOp->m_Defines.push_back(Operand::Define(du.psInst, du.psOp));

      }
    }
  }
}
