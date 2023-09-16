#pragma once

#include "internal_includes/Operand.h"
#include "internal_includes/tokens.h"
#include "include/ShaderInfo.h"
#include <memory>

#define ATOMIC_ADDRESS_BASIC 0
#define ATOMIC_ADDRESS_ARRAY_DYNAMIC 1
#define ATOMIC_ADDRESS_STRUCT_DYNAMIC 2

#define TEXSMP_FLAG_NONE 0x0
#define TEXSMP_FLAG_LOD 0x1 //LOD comes from operand
#define TEXSMP_FLAG_DEPTHCOMPARE 0x2
#define TEXSMP_FLAG_FIRSTLOD 0x4 //LOD is 0
#define TEXSMP_FLAG_BIAS 0x8
#define TEXSMP_FLAG_GRAD 0x10
//Gather specific flags
#define TEXSMP_FLAG_GATHER 0x20
#define TEXSMP_FLAG_PARAMOFFSET 0x40 //Offset comes from operand

struct Instruction
{
  Instruction() = default;

	// For creating unit tests only. Create an instruction with temps (unless reg is 0xffffffff in which case use OPERAND_TYPE_INPUT/OUTPUT)
	Instruction(OPCODE_TYPE opcode, uint32_t reg1 = 0, uint32_t reg1Mask = 0, uint32_t reg2 = 0, uint32_t reg2Mask = 0, uint32_t reg3 = 0, uint32_t reg3Mask = 0, uint32_t reg4 = 0, uint32_t reg4Mask = 0)
	{
		eOpcode = opcode;
		eBooleanTestType = INSTRUCTION_TEST_ZERO;
        ui32FirstSrc = 0;
		ui32NumOperands = 0;
		m_LoopInductors[0] = m_LoopInductors[1] = m_LoopInductors[2] = m_LoopInductors[3] = 0;
		m_SkipTranslation = false;
		m_InductorRegister = 0;

		if (reg1Mask == 0)
			return;

		ui32NumOperands++;
		asOperands[0].eType = reg1 == 0xffffffff ? OPERAND_TYPE_OUTPUT : OPERAND_TYPE_TEMP;
		asOperands[0].ui32RegisterNumber = reg1 == 0xffffffff ? 0 : reg1;
		asOperands[0].ui32CompMask = reg1Mask;
		asOperands[0].eSelMode = OPERAND_4_COMPONENT_MASK_MODE;

		if (reg2Mask == 0)
			return;

		ui32FirstSrc = 1;
		ui32NumOperands++;

		asOperands[1].eType = reg2 == 0xffffffff ? OPERAND_TYPE_INPUT : OPERAND_TYPE_TEMP;
		asOperands[1].ui32RegisterNumber = reg2 == 0xffffffff ? 0 : reg2;
		asOperands[1].ui32CompMask = reg2Mask;
		asOperands[1].eSelMode = OPERAND_4_COMPONENT_MASK_MODE;

		if (reg3Mask == 0)
			return;
		ui32NumOperands++;

		asOperands[2].eType = reg3 == 0xffffffff ? OPERAND_TYPE_INPUT : OPERAND_TYPE_TEMP;
		asOperands[2].ui32RegisterNumber = reg3 == 0xffffffff ? 0 : reg3;
		asOperands[2].ui32CompMask = reg3Mask;
		asOperands[2].eSelMode = OPERAND_4_COMPONENT_MASK_MODE;

		if (reg4Mask == 0)
			return;
		ui32NumOperands++;

		asOperands[3].eType = reg4 == 0xffffffff ? OPERAND_TYPE_INPUT : OPERAND_TYPE_TEMP;
		asOperands[3].ui32RegisterNumber = reg4 == 0xffffffff ? 0 : reg4;
		asOperands[3].ui32CompMask = reg4Mask;
		asOperands[3].eSelMode = OPERAND_4_COMPONENT_MASK_MODE;
	}


	bool IsPartialPrecisionSamplerInstruction(const ShaderInfo &info, OPERAND_MIN_PRECISION *pType) const;

	// Flags for ChangeOperandTempRegister
#define UD_CHANGE_SUBOPERANDS 1
#define UD_CHANGE_MAIN_OPERAND 2
#define UD_CHANGE_ALL 3

	void ChangeOperandTempRegister(Operand *psOperand, uint32_t oldReg, uint32_t newReg, uint32_t compMask, uint32_t flags, uint32_t rebase);


	OPCODE_TYPE eOpcode = OPCODE_NOP;
	INSTRUCTION_TEST_BOOLEAN eBooleanTestType = INSTRUCTION_TEST_ZERO;
	uint32_t ui32SyncFlags = 0;
	uint32_t ui32NumOperands = 0;
	uint32_t ui32FirstSrc = 0;
	Operand asOperands[6];
	uint32_t bSaturate = 0;
	uint32_t ui32FuncIndexWithinInterface = 0;
	RESINFO_RETURN_TYPE eResInfoReturnType = RESINFO_INSTRUCTION_RETURN_FLOAT;

	int bAddressOffset = 0;
	int8_t iUAddrOffset = 0;
	int8_t iVAddrOffset = 0;
	int8_t iWAddrOffset = 0;
  RESOURCE_RETURN_TYPE xType = RETURN_TYPE_FLOAT;
  RESOURCE_RETURN_TYPE yType = RETURN_TYPE_FLOAT;
  RESOURCE_RETURN_TYPE zType = RETURN_TYPE_FLOAT;
  RESOURCE_RETURN_TYPE wType = RETURN_TYPE_FLOAT;
	RESOURCE_DIMENSION eResDim = RESOURCE_DIMENSION_UNKNOWN;
	int8_t iCausedSplit = 0; // Nonzero if has caused a temp split. Later used by sampler datatype tweaking

	struct Use
	{
		Use() : m_Inst(0), m_Op(0) {}
		Use(const Use &a) : m_Inst(a.m_Inst), m_Op(a.m_Op) {}
		Use(Instruction *inst, Operand *op) : m_Inst(inst), m_Op(op) {}

		Instruction *m_Inst = nullptr; // The instruction that references the result of this instruction
		Operand		*m_Op = nullptr;   // The operand within the instruction above. Note: can also be suboperand.
	};

	std::vector<Use> m_Uses; // Array of use sites for the result(s) of this instruction, if any of the results is a temp reg.

  Instruction *m_LoopInductors[4] = { nullptr, nullptr, nullptr, nullptr }; // If OPCODE_LOOP and is suitable for transforming into for-loop, contains pointers to for initializer, end condition, breakc,  and increment. 
	bool m_SkipTranslation = false; // If true, don't emit this instruction (currently used by the for loop translation)
	uint32_t m_InductorRegister = 0; // If non-zero, the inductor variable can be declared in the for statement, and this register number has been allocated for it

	//uint64_t id;
};
