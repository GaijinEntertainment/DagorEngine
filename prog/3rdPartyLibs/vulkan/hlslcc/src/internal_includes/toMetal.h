
#pragma once
#include "internal_includes/Translator.h"
#include <map>
#include <vector>

// We store struct definition contents inside a vector of strings
struct StructDefinition
{
	StructDefinition() : m_Members(), m_Dependencies(), m_IsPrinted(false) {}

	std::vector<std::string> m_Members; // A vector of strings with the struct members
	std::vector<std::string> m_Dependencies; // A vector of struct names this struct depends on.
	bool m_IsPrinted; // Has this struct been printed out yet?
};

typedef std::map<std::string, StructDefinition> StructDefinitions;

// Map of extra function definitions we need to add before the shader body but after the declarations.
typedef std::map<std::string, std::string> FunctionDefinitions;

// A helper class for allocating binding slots
// (because both UAVs and textures use the same slots in Metal, also constant buffers and other buffers etc)
class BindingSlotAllocator
{
	typedef std::map<uint32_t, uint32_t> SlotMap;
	SlotMap m_Allocations;
public:
	BindingSlotAllocator() : m_Allocations(), m_NextFreeSlot(0) {}

	enum BindType
	{
		ConstantBuffer = 0,
		RWBuffer,
		Texture,
		UAV
	};

	// isUAV is only meaningful for texture slots

	uint32_t GetBindingSlot(uint32_t regNo, BindType type)
	{
		// The key is regNumber with the bindtype stored to highest 16 bits
		uint32_t key = regNo | (uint32_t(type) << 16);
		SlotMap::iterator itr = m_Allocations.find(key);
		if (itr == m_Allocations.end())
		{
			m_Allocations.insert(std::make_pair(key, m_NextFreeSlot));
			return m_NextFreeSlot++;
		}
		return itr->second;
	}

private:
	uint32_t m_NextFreeSlot;
};


class ToMetal : public Translator
{
protected:
	GLLang language;
public:
	explicit ToMetal(HLSLCrossCompilerContext *ctx) : Translator(ctx), m_ShadowSamplerDeclared(false) {}

	virtual bool Translate();
	virtual void TranslateDeclaration(const Declaration *psDecl);
	virtual bool TranslateSystemValue(const Operand *psOperand, const ShaderInfo::InOutSignature *sig, std::string &result, uint32_t *pui32IgnoreSwizzle, bool isIndexed, bool isInput, bool *outSkipPrefix = NULL);
	std::string TranslateOperand(const Operand *psOp, uint32_t flags, uint32_t ui32ComponentMask = OPERAND_4_COMPONENT_MASK_ALL);

	virtual void SetIOPrefixes();

private:
	void TranslateInstruction(Instruction* psInst);

	void DeclareBuiltinInput(const Declaration *psDecl);
	void DeclareBuiltinOutput(const Declaration *psDecl);

	// Retrieve the name of the output struct for this shader
	std::string GetOutputStructName() const;
	std::string GetInputStructName() const;

	void HandleInputRedirect(const Declaration *psDecl, const std::string &typeName);
	void HandleOutputRedirect(const Declaration *psDecl, const std::string &typeName);

	void DeclareConstantBuffer(const ConstantBuffer *psCBuf, uint32_t ui32BindingPoint);
	void DeclareStructType(const std::string &name, const std::vector<ShaderVar> &contents, bool withinCB = false, uint32_t cumulativeOffset = 0, bool stripUnused = false);
	void DeclareStructType(const std::string &name, const std::vector<ShaderVarType> &contents, bool withinCB = false, uint32_t cumulativeOffset = 0);
	void DeclareStructVariable(const std::string &parentName, const ShaderVar &var, bool withinCB = false, uint32_t cumulativeOffset = 0);
	void DeclareStructVariable(const std::string &parentName, const ShaderVarType &var, bool withinCB = false, uint32_t cumulativeOffset = 0);
	void DeclareBufferVariable(const Declaration *psDecl, const bool isRaw, const bool isUAV);

	void DeclareResource(const Declaration *psDecl);
	void TranslateResourceTexture(const Declaration* psDecl, uint32_t samplerCanDoShadowCmp, HLSLCC_TEX_DIMENSION texDim);

	void DeclareOutput(const Declaration *decl);

	void PrintStructDeclarations(StructDefinitions &defs);

	std::string ResourceName(ResourceGroup group, const uint32_t ui32RegisterNumber);

	// ToMetalOperand.cpp
	std::string TranslateOperandSwizzle(const Operand* psOperand, uint32_t ui32ComponentMask, int iRebase, bool includeDot = true);
	std::string TranslateOperandIndex(const Operand* psOperand, int index);
	std::string TranslateVariableName(const Operand* psOperand, uint32_t ui32TOFlag, uint32_t* pui32IgnoreSwizzle, uint32_t ui32CompMask, int *piRebase);

  void TranslateUDiv(Instruction* psInst, int qoutient, int remainder, int divident, int divisor);

	// ToMetalInstruction.cpp

	void AddOpAssignToDestWithMask(const Operand* psDest,
		SHADER_VARIABLE_TYPE eSrcType, uint32_t ui32SrcElementCount, const char *szAssignmentOp, int *pNeedsParenthesis, uint32_t ui32CompMask);
	void AddAssignToDest(const Operand* psDest,
		SHADER_VARIABLE_TYPE eSrcType, uint32_t ui32SrcElementCount, int* pNeedsParenthesis);
	void AddAssignPrologue(int numParenthesis);

	typedef enum
	{
		CMP_EQ,
		CMP_LT,
		CMP_GE,
		CMP_NE,
	} ComparisonType;

	void AddComparison(Instruction* psInst, ComparisonType eType,
		uint32_t typeFlag);

	void AddMOVBinaryOp(const Operand *pDest, Operand *pSrc);
	void AddMOVCBinaryOp(const Operand *pDest, const Operand *src0, Operand *src1, Operand *src2);
	void CallBinaryOp(const char* name, Instruction* psInst,
		int dest, int src0, int src1, SHADER_VARIABLE_TYPE eDataType);
	void CallTernaryOp(const char* op1, const char* op2, Instruction* psInst,
		int dest, int src0, int src1, int src2, uint32_t dataType);
	void CallHelper3(const char* name, Instruction* psInst,
		int dest, int src0, int src1, int src2, int paramsShouldFollowWriteMask);
	void CallHelper2(const char* name, Instruction* psInst,
		int dest, int src0, int src1, int paramsShouldFollowWriteMask);
	void CallHelper2Int(const char* name, Instruction* psInst,
		int dest, int src0, int src1, int paramsShouldFollowWriteMask);
	void CallHelper2UInt(const char* name, Instruction* psInst,
		int dest, int src0, int src1, int paramsShouldFollowWriteMask);
	void CallHelper1(const char* name, Instruction* psInst,
		int dest, int src0, int paramsShouldFollowWriteMask);
	void CallHelper1Int(
		const char* name,
		Instruction* psInst,
		const int dest,
		const int src0,
		int paramsShouldFollowWriteMask);
	void TranslateTexelFetch(
		Instruction* psInst,
		const ResourceBinding* psBinding,
		bstring glsl);
	void TranslateTexelFetchOffset(
		Instruction* psInst,
		const ResourceBinding* psBinding,
		bstring glsl);
	void TranslateTexCoord(
		const RESOURCE_DIMENSION eResDim,
		Operand* psTexCoordOperand);
	void GetResInfoData(Instruction* psInst, int index, int destElem);
	void TranslateTextureSample(Instruction* psInst,
		uint32_t ui32Flags);
	void TranslateDynamicComponentSelection(const ShaderVarType* psVarType,
		const Operand* psByteAddr, uint32_t offset, uint32_t mask);
	void TranslateShaderStorageStore(Instruction* psInst);
	void TranslateShaderStorageLoad(Instruction* psInst);
	void TranslateAtomicMemOp(Instruction* psInst);
	void TranslateConditional(
		Instruction* psInst,
		bstring glsl);

	// The map is keyed by struct name. The special name "" (empty string) is reserved for entry point function parameters 
	StructDefinitions m_StructDefinitions;

	// A <function name, body text> map of extra helper functions we'll need.
	FunctionDefinitions m_FunctionDefinitions;

	BindingSlotAllocator m_TextureSlots;
	BindingSlotAllocator m_BufferSlots;

	std::string m_ExtraGlobalDefinitions;

	bool m_ShadowSamplerDeclared;

	void EnsureShadowSamplerDeclared();

	// Add an extra function to the m_FunctionDefinitions list, unless it's already there.
	void DeclareExtraFunction(const std::string &name, const std::string &body);

	// Move all lowp -> mediump
	void ClampPartialPrecisions();
};


