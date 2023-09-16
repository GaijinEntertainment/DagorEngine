
#pragma once

#include <vector>
#include <unordered_set>
#include "internal_includes/tokens.h"
#include "internal_includes/Operand.h"

struct ICBVec4
{
	uint32_t a;
	uint32_t b;
	uint32_t c;
	uint32_t d;
};

#define ACCESS_FLAG_READ       0x1
#define ACCESS_FLAG_WRITE      0x2

struct Declaration
{
	OPCODE_TYPE eOpcode = OPCODE_INVALID;

	uint32_t ui32NumOperands = 0;

  Operand asOperands[2] = {};

	std::vector<ICBVec4> asImmediateConstBuffer;
	//The declaration can set one of these
	//values depending on the opcode.
	union {
		uint32_t ui32GlobalFlags;
		uint32_t ui32NumTemps;
		RESOURCE_DIMENSION eResourceDimension;
		INTERPOLATION_MODE eInterpolation;
		PRIMITIVE_TOPOLOGY eOutputPrimitiveTopology;
		PRIMITIVE eInputPrimitive;
		uint32_t ui32MaxOutputVertexCount;
		TESSELLATOR_DOMAIN eTessDomain;
		TESSELLATOR_PARTITIONING eTessPartitioning;
		TESSELLATOR_OUTPUT_PRIMITIVE eTessOutPrim;
		uint32_t aui32WorkGroupSize[3];
		uint32_t ui32HullPhaseInstanceCount;
		float fMaxTessFactor;
		uint32_t ui32IndexRange;
		uint32_t ui32GSInstanceCount;

		struct Interface_TAG
		{
			uint32_t ui32InterfaceID;
			uint32_t ui32NumFuncTables;
			uint32_t ui32ArraySize;
		} interface;
	} value;

	uint32_t ui32BufferStride = 0;

	struct
	{
		uint32_t ui32GloballyCoherentAccess = 0;
		uint8_t bCounter = 0;
		RESOURCE_RETURN_TYPE Type = RETURN_TYPE_UNORM;
		uint32_t ui32NumComponents = 0;
		uint32_t ui32AccessFlags = 0;
	} sUAV;

	struct
	{
		uint32_t ui32Stride = 0;
		uint32_t ui32Count = 0;
	} sTGSM;

	struct
	{
		uint32_t ui32RegIndex = 0;
		uint32_t ui32RegCount = 0;
		uint32_t ui32RegComponentSize = 0;
	} sIdxTemp;

	uint32_t ui32TableLength = 0;

	uint32_t ui32IsShadowTex = 0;

	// Set indexed by sampler register number.
	std::unordered_set<uint32_t> samplersUsed;
};

