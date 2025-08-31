/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/
#define OMM_DECLARE_GLOBAL_CONSTANT_BUFFER						\
OMM_CONSTANTS_START(GlobalConstants)							\
	OMM_CONSTANT(uint, IndexCount)								\
	OMM_CONSTANT(uint, PrimitiveCount)							\
	OMM_CONSTANT(uint, MaxBatchCount)							\
	OMM_CONSTANT(uint, MaxOutOmmArraySize)						\
																\
	OMM_CONSTANT(uint, IsOmmIndexFormat16bit)					\
	OMM_CONSTANT(uint, EnableSpecialIndices)					\
	OMM_CONSTANT(uint, EnableTexCoordDeduplication)				\
	OMM_CONSTANT(uint, DoSetup)									\
																\
	OMM_CONSTANT(uint, SamplerIndex)							\
	OMM_CONSTANT(uint, BakeResultBufferSize)					\
	OMM_CONSTANT(float2, ViewportSize)							\
																\
	OMM_CONSTANT(float2, InvViewportSize)						\
	OMM_CONSTANT(float2, ViewportOffset)						\
																\
	OMM_CONSTANT(uint, MaxNumSubdivisionLevels)					\
	OMM_CONSTANT(uint, MaxSubdivisionLevel)						\
	OMM_CONSTANT(uint, OMMFormat)								\
	OMM_CONSTANT(uint, TexCoordHashTableEntryCount)				\
																\
	OMM_CONSTANT(float, DynamicSubdivisionScale)				\
	OMM_CONSTANT(uint, TexCoordFormat)							\
	OMM_CONSTANT(uint, TexCoordOffset)							\
	OMM_CONSTANT(uint, TexCoordStride)							\
																\
	OMM_CONSTANT(float, AlphaCutoff)							\
	OMM_CONSTANT(uint, AlphaCutoffLessEqual)					\
	OMM_CONSTANT(uint, AlphaCutoffGreater)						\
	OMM_CONSTANT(uint, AlphaTextureChannel)						\
																\
	OMM_CONSTANT(uint, FilterType) /* TextureFilterMode */ 		\
	OMM_CONSTANT(uint, EnableLevelLine)							\
	OMM_CONSTANT(uint, EnablePostDispatchInfoStats)				\
	OMM_CONSTANT(uint, IndirectDispatchEntryStride)				\
																\
	OMM_CONSTANT(float2, TexSize)								\
	OMM_CONSTANT(float2, InvTexSize)							\
/*	---- Buffer offsets go here								  */\
																\
	OMM_CONSTANT(uint, IEBakeBufferOffset)						\
	OMM_CONSTANT(uint, IEBakeCsBufferOffset)					\
	OMM_CONSTANT(uint, IECompressCsBufferOffset)				\
	OMM_CONSTANT(uint, OmmArrayAllocatorCounterBufferOffset)	\
																\
	OMM_CONSTANT(uint, OmmDescAllocatorCounterBufferOffset)		\
	OMM_CONSTANT(uint, BakeResultBufferCounterBufferOffset)		\
	OMM_CONSTANT(uint, DispatchIndirectThreadCountBufferOffset)	\
	OMM_CONSTANT(uint, IEBakeCsThreadCountBufferOffset)			\
																\
	OMM_CONSTANT(uint, RasterItemsBufferOffset)					\
	OMM_CONSTANT(uint, TempOmmIndexBufferOffset)				\
	OMM_CONSTANT(uint, HashTableBufferOffset)					\
	OMM_CONSTANT(uint, BakeResultBufferOffset)					\
																\
	OMM_CONSTANT(uint, SpecialIndicesStateBufferOffset)			\
	OMM_CONSTANT(uint, TempOmmBakeScheduleTrackerBufferOffset)	\
	OMM_CONSTANT(uint, AssertBufferOffset)						\
	OMM_CONSTANT(uint, Pad3)									\
																\
OMM_CONSTANTS_END(GlobalConstants, 0)
