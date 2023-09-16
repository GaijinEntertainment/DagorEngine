#pragma once

#include "hlslcc.h"
#include "internal_includes/Translator.h"
#include <tuple>

class HLSLCrossCompilerContext;

class ToGLSL : public Translator
{
protected:
	GLLang language;
public:
	explicit ToGLSL(HLSLCrossCompilerContext *ctx) : Translator(ctx), language(LANG_DEFAULT) {}
	// Sets the target language according to given input. if LANG_DEFAULT, does autodetect and returns the selected language
	GLLang SetLanguage(GLLang suggestedLanguage);

	virtual bool Translate();
	virtual void TranslateDeclaration(const Declaration* psDecl);
	virtual bool TranslateSystemValue(const Operand *psOperand, const ShaderInfo::InOutSignature *sig, std::string &result, uint32_t *pui32IgnoreSwizzle, bool isIndexed, bool isInput, bool *outSkipPrefix = NULL);
	virtual void SetIOPrefixes();

private:
  bool validateStructBufferDataLayout(const Declaration& psDecl);
  bool validateConstBufferDataLayout(const Declaration& psDecl);
  bool validateDeclarationDataLayout(const Declaration& psDecl);
  bool validateDataLayout();

	void TranslateOperand(const Operand *psOp, uint32_t flags, uint32_t ui32ComponentMask = OPERAND_4_COMPONENT_MASK_ALL, const char* temp_override = NULL);
	void TranslateInstruction(Instruction* psInst, bool isEmbedded = false);

	void TranslateVariableNameWithMask(const Operand* psOperand, uint32_t ui32TOFlag, uint32_t* pui32IgnoreSwizzle, uint32_t ui32CompMask, int *piRebase, const char* temp_name);

	void TranslateOperandIndex(const Operand* psOperand, int index);
	void TranslateOperandIndexMAD(const Operand* psOperand, int index, uint32_t multiply, uint32_t add);

	void AddOpAssignToDestWithMask(const Operand* psDest,
		SHADER_VARIABLE_TYPE eSrcType, uint32_t ui32SrcElementCount, const char *szAssignmentOp, int *pNeedsParenthesis, uint32_t ui32CompMask);
	void AddAssignToDest(const Operand* psDest,
		SHADER_VARIABLE_TYPE eSrcType, uint32_t ui32SrcElementCount, int* pNeedsParenthesis);
	void AddAssignPrologue(int numParenthesis, bool isEmbedded = false);

	void AddBuiltinOutput(const Declaration* psDecl, int arrayElements, const char* builtinName);
	void HandleOutputRedirect(const Declaration *psDecl, const char *Precision);
	void HandleInputRedirect(const Declaration *psDecl, const char *Precision);

	void AddUserOutput(const Declaration* psDecl);
	void DeclareStructConstants(const uint32_t ui32BindingPoint,
		const ConstantBuffer* psCBuf, const Operand* psOperand,
		bstring glsl);

	typedef enum
	{
		CMP_EQ,
		CMP_LT,
		CMP_GE,
		CMP_NE,
	} ComparisonType;
	
	void AddComparison(Instruction* psInst, ComparisonType eType,
		uint32_t typeFlag);
  void FormatAndAddAccessMask(int component_mask);
  void CopyOperandToTemp(Instruction* psInst,
                         int index);
  void TranslateUDiv(Instruction* psInst,
                     int qoutient,
                     int remainder,
                     int divident,
                     int divisor);
  void TranslateCondSwap(Instruction* psInst, int dst0, int dst1, int cmp, int src0, int src1);
	void AddMOVBinaryOp(const Operand *pDest, Operand *pSrc, bool isEmbedded = false);
	void AddMOVCBinaryOp(const Operand *pDest, const Operand *src0, Operand *src1, Operand *src2, int tempMask, const char* tempName = nullptr);
	void CallBinaryOp(const char* name, Instruction* psInst,
		int dest, int src0, int src1, SHADER_VARIABLE_TYPE eDataType, bool isEmbedded = false, int temp_override = -1);
	void CallTernaryOp(const char* op1, const char* op2, Instruction* psInst,
		int dest, int src0, int src1, int src2, uint32_t dataType);
	void CallHelper3(const char* name, Instruction* psInst,
		int dest, int src0, int src1, int src2, int paramsShouldFollowWriteMask, uint32_t ui32Flags = 0);
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
		Operand* psTexCoordOperand,
    bool needs_array_index);
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

};




