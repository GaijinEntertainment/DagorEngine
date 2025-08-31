/*
* Copyright (c) 2014-2021, NVIDIA CORPORATION. All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files (the "Software"),
* to deal in the Software without restriction, including without limitation
* the rights to use, copy, modify, merge, publish, distribute, sublicense,
* and/or sell copies of the Software, and to permit persons to whom the
* Software is furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
* THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
* DEALINGS IN THE SOFTWARE.
*/

#define OMM_DECLARE_INPUT_RESOURCES \
	OMM_INPUT_RESOURCE(Texture2D<float4>,	t_alphaTexture,		t, 0) \
	OMM_INPUT_RESOURCE(Buffer<uint>,		t_indexBuffer,		t, 1) \
	OMM_INPUT_RESOURCE(ByteAddressBuffer,	t_texCoordBuffer,	t, 2) \
	OMM_INPUT_RESOURCE(ByteAddressBuffer,	t_heap0,			t, 3) \
	OMM_INPUT_RESOURCE(ByteAddressBuffer,	t_heap1,			t, 4)

#define OMM_DECLARE_OUTPUT_RESOURCES \
    OMM_OUTPUT_RESOURCE( RWByteAddressBuffer, u_heap0, u, 0 ) \
    OMM_OUTPUT_RESOURCE( RWByteAddressBuffer, u_vmArrayBuffer, u, 1 )

#define OMM_DECLARE_SUBRESOURCES \
    OMM_SUBRESOURCE(ByteAddressBuffer, RasterItemsBuffer, t_heap0) \
	OMM_SUBRESOURCE(ByteAddressBuffer, IEBakeCsThreadCountBuffer, t_heap1)\
    OMM_SUBRESOURCE(RWByteAddressBuffer, SpecialIndicesStateBuffer, u_heap0)

#define OMM_DECLARE_LOCAL_CONSTANT_BUFFER						\
OMM_PUSH_CONSTANTS_START(LocalConstants)						\
																\
	OMM_PUSH_CONSTANT(uint, SubdivisionLevel)					\
	OMM_PUSH_CONSTANT(uint, VmResultBufferStride)				\
	OMM_PUSH_CONSTANT(uint, BatchIndex)							\
	OMM_PUSH_CONSTANT(uint, PrimitiveIdOffset)					\
																\
	OMM_PUSH_CONSTANT(uint, gPad0)								\
	OMM_PUSH_CONSTANT(uint, gPad1)								\
	OMM_PUSH_CONSTANT(uint, gPad2)								\
	OMM_PUSH_CONSTANT(uint, gPad4)								\
																\
OMM_PUSH_CONSTANTS_END(LocalConstants, 1)
