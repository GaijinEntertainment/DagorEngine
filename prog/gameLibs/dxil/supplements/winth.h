#pragma once

typedef struct _D3D12_SHADER_INPUT_BIND_DESC
{
  LPCSTR Name;                // Name of the resource
  D3D_SHADER_INPUT_TYPE Type; // Type of resource (e.g. texture, cbuffer, etc.)
  UINT BindPoint;             // Starting bind point
  UINT BindCount;             // Number of contiguous bind points (for arrays)

  UINT uFlags;                         // Input binding flags
  D3D_RESOURCE_RETURN_TYPE ReturnType; // Return type (if texture)
  D3D_SRV_DIMENSION Dimension;         // Dimension (if texture)
  UINT NumSamples;                     // Number of samples (0 if not MS texture)
  UINT Space;                          // Register space
  UINT uID;                            // Range ID in the bytecode
} D3D12_SHADER_INPUT_BIND_DESC;

typedef struct _D3D12_SIGNATURE_PARAMETER_DESC
{
  LPCSTR SemanticName;                       // Name of the semantic
  UINT SemanticIndex;                        // Index of the semantic
  UINT Register;                             // Number of member variables
  D3D_NAME SystemValueType;                  // A predefined system value, or D3D_NAME_UNDEFINED if not applicable
  D3D_REGISTER_COMPONENT_TYPE ComponentType; // Scalar type (e.g. uint, float, etc.)
  BYTE Mask;                                 // Mask to indicate which components of the register
                                             // are used (combination of D3D10_COMPONENT_MASK values)
  BYTE ReadWriteMask;                        // Mask to indicate whether a given component is
                                             // never written (if this is an output signature) or
                                             // always read (if this is an input signature).
                                             // (combination of D3D_MASK_* values)
  UINT Stream;                               // Stream index
  D3D_MIN_PRECISION MinPrecision;            // Minimum desired interpolation precision
} D3D12_SIGNATURE_PARAMETER_DESC;

typedef struct _D3D12_SHADER_DESC
{
  UINT Version;   // Shader version
  LPCSTR Creator; // Creator string
  UINT Flags;     // Shader compilation/parse flags

  UINT ConstantBuffers;  // Number of constant buffers
  UINT BoundResources;   // Number of bound resources
  UINT InputParameters;  // Number of parameters in the input signature
  UINT OutputParameters; // Number of parameters in the output signature

  UINT InstructionCount;                              // Number of emitted instructions
  UINT TempRegisterCount;                             // Number of temporary registers used
  UINT TempArrayCount;                                // Number of temporary arrays used
  UINT DefCount;                                      // Number of constant defines
  UINT DclCount;                                      // Number of declarations (input + output)
  UINT TextureNormalInstructions;                     // Number of non-categorized texture instructions
  UINT TextureLoadInstructions;                       // Number of texture load instructions
  UINT TextureCompInstructions;                       // Number of texture comparison instructions
  UINT TextureBiasInstructions;                       // Number of texture bias instructions
  UINT TextureGradientInstructions;                   // Number of texture gradient instructions
  UINT FloatInstructionCount;                         // Number of floating point arithmetic instructions used
  UINT IntInstructionCount;                           // Number of signed integer arithmetic instructions used
  UINT UintInstructionCount;                          // Number of unsigned integer arithmetic instructions used
  UINT StaticFlowControlCount;                        // Number of static flow control instructions used
  UINT DynamicFlowControlCount;                       // Number of dynamic flow control instructions used
  UINT MacroInstructionCount;                         // Number of macro instructions used
  UINT ArrayInstructionCount;                         // Number of array instructions used
  UINT CutInstructionCount;                           // Number of cut instructions used
  UINT EmitInstructionCount;                          // Number of emit instructions used
  D3D_PRIMITIVE_TOPOLOGY GSOutputTopology;            // Geometry shader output topology
  UINT GSMaxOutputVertexCount;                        // Geometry shader maximum output vertex count
  D3D_PRIMITIVE InputPrimitive;                       // GS/HS input primitive
  UINT PatchConstantParameters;                       // Number of parameters in the patch constant signature
  UINT cGSInstanceCount;                              // Number of Geometry shader instances
  UINT cControlPoints;                                // Number of control points in the HS->DS stage
  D3D_TESSELLATOR_OUTPUT_PRIMITIVE HSOutputPrimitive; // Primitive output by the tessellator
  D3D_TESSELLATOR_PARTITIONING HSPartitioning;        // Partitioning mode of the tessellator
  D3D_TESSELLATOR_DOMAIN TessellatorDomain;           // Domain of the tessellator (quad, tri, isoline)
  // instruction counts
  UINT cBarrierInstructions;      // Number of barrier instructions in a compute shader
  UINT cInterlockedInstructions;  // Number of interlocked instructions
  UINT cTextureStoreInstructions; // Number of texture writes
} D3D12_SHADER_DESC;

typedef struct _D3D12_SHADER_BUFFER_DESC
{
  LPCSTR Name;           // Name of the constant buffer
  D3D_CBUFFER_TYPE Type; // Indicates type of buffer content
  UINT Variables;        // Number of member variables
  UINT Size;             // Size of CB (in bytes)
  UINT uFlags;           // Buffer description flags
} D3D12_SHADER_BUFFER_DESC;

typedef struct _D3D12_SHADER_VARIABLE_DESC
{
  LPCSTR Name;         // Name of the variable
  UINT StartOffset;    // Offset in constant buffer's backing store
  UINT Size;           // Size of variable (in bytes)
  UINT uFlags;         // Variable flags
  LPVOID DefaultValue; // Raw pointer to default value
  UINT StartTexture;   // First texture index (or -1 if no textures used)
  UINT TextureSize;    // Number of texture slots possibly used.
  UINT StartSampler;   // First sampler index (or -1 if no textures used)
  UINT SamplerSize;    // Number of sampler slots possibly used.
} D3D12_SHADER_VARIABLE_DESC;

typedef struct _D3D12_SHADER_TYPE_DESC
{
  D3D_SHADER_VARIABLE_CLASS Class; // Variable class (e.g. object, matrix, etc.)
  D3D_SHADER_VARIABLE_TYPE Type;   // Variable type (e.g. float, sampler, etc.)
  UINT Rows;                       // Number of rows (for matrices, 1 for other numeric, 0 if not applicable)
  UINT Columns;                    // Number of columns (for vectors & matrices, 1 for other numeric, 0 if not
                                   // applicable)
  UINT Elements;                   // Number of elements (0 if not an array)
  UINT Members;                    // Number of members (0 if not a structure)
  UINT Offset;                     // Offset from the start of structure (0 if not a structure member)
  LPCSTR Name;                     // Name of type, can be NULL
} D3D12_SHADER_TYPE_DESC;

typedef interface ID3D12ShaderReflectionConstantBuffer ID3D12ShaderReflectionConstantBuffer;
typedef interface ID3D12ShaderReflection ID3D12ShaderReflection;
typedef interface ID3D12ShaderReflectionVariable ID3D12ShaderReflectionVariable;
typedef interface ID3D12ShaderReflectionType ID3D12ShaderReflectionType;

interface DECLSPEC_UUID("5A58797D-A72C-478D-8BA2-EFC6B0EFE88E") ID3D12ShaderReflection;

#define INTERFACE ID3D12ShaderReflection

DECLARE_INTERFACE_(ID3D12ShaderReflection, IUnknown)
{
  STDMETHOD(QueryInterface)(THIS_ _In_ REFIID iid, _Out_ LPVOID * ppv) PURE;
  STDMETHOD_(ULONG, AddRef)(THIS) PURE;
  STDMETHOD_(ULONG, Release)(THIS) PURE;

  STDMETHOD(GetDesc)(THIS_ _Out_ D3D12_SHADER_DESC * pDesc) PURE;

  STDMETHOD_(ID3D12ShaderReflectionConstantBuffer *, GetConstantBufferByIndex)
  (THIS_ _In_ UINT Index) PURE;
  STDMETHOD_(ID3D12ShaderReflectionConstantBuffer *, GetConstantBufferByName)
  (THIS_ _In_ LPCSTR Name) PURE;

  STDMETHOD(GetResourceBindingDesc)
  (THIS_ _In_ UINT ResourceIndex, _Out_ D3D12_SHADER_INPUT_BIND_DESC * pDesc) PURE;

  STDMETHOD(GetInputParameterDesc)
  (THIS_ _In_ UINT ParameterIndex, _Out_ D3D12_SIGNATURE_PARAMETER_DESC * pDesc) PURE;
  STDMETHOD(GetOutputParameterDesc)
  (THIS_ _In_ UINT ParameterIndex, _Out_ D3D12_SIGNATURE_PARAMETER_DESC * pDesc) PURE;
  STDMETHOD(GetPatchConstantParameterDesc)
  (THIS_ _In_ UINT ParameterIndex, _Out_ D3D12_SIGNATURE_PARAMETER_DESC * pDesc) PURE;

  STDMETHOD_(ID3D12ShaderReflectionVariable *, GetVariableByName)(THIS_ _In_ LPCSTR Name) PURE;

  STDMETHOD(GetResourceBindingDescByName)
  (THIS_ _In_ LPCSTR Name, _Out_ D3D12_SHADER_INPUT_BIND_DESC * pDesc) PURE;

  STDMETHOD_(UINT, GetMovInstructionCount)(THIS) PURE;
  STDMETHOD_(UINT, GetMovcInstructionCount)(THIS) PURE;
  STDMETHOD_(UINT, GetConversionInstructionCount)(THIS) PURE;
  STDMETHOD_(UINT, GetBitwiseInstructionCount)(THIS) PURE;

  STDMETHOD_(D3D_PRIMITIVE, GetGSInputPrimitive)(THIS) PURE;
  STDMETHOD_(BOOL, IsSampleFrequencyShader)(THIS) PURE;

  STDMETHOD_(UINT, GetNumInterfaceSlots)(THIS) PURE;
  STDMETHOD(GetMinFeatureLevel)(THIS_ _Out_ enum D3D_FEATURE_LEVEL * pLevel) PURE;

  STDMETHOD_(UINT, GetThreadGroupSize)
  (THIS_ _Out_opt_ UINT * pSizeX, _Out_opt_ UINT * pSizeY, _Out_opt_ UINT * pSizeZ) PURE;

  STDMETHOD_(UINT64, GetRequiresFlags)(THIS) PURE;
};

interface DECLSPEC_UUID("C59598B4-48B3-4869-B9B1-B1618B14A8B7") ID3D12ShaderReflectionConstantBuffer;

#undef INTERFACE
#define INTERFACE ID3D12ShaderReflectionConstantBuffer

DECLARE_INTERFACE(ID3D12ShaderReflectionConstantBuffer)
{
  STDMETHOD(GetDesc)(THIS_ D3D12_SHADER_BUFFER_DESC * pDesc) PURE;

  STDMETHOD_(ID3D12ShaderReflectionVariable *, GetVariableByIndex)(THIS_ _In_ UINT Index) PURE;
  STDMETHOD_(ID3D12ShaderReflectionVariable *, GetVariableByName)(THIS_ _In_ LPCSTR Name) PURE;
};

interface DECLSPEC_UUID("8337A8A6-A216-444A-B2F4-314733A73AEA") ID3D12ShaderReflectionVariable;

#undef INTERFACE
#define INTERFACE ID3D12ShaderReflectionVariable

DECLARE_INTERFACE(ID3D12ShaderReflectionVariable)
{
  STDMETHOD(GetDesc)(THIS_ _Out_ D3D12_SHADER_VARIABLE_DESC * pDesc) PURE;

  STDMETHOD_(ID3D12ShaderReflectionType *, GetType)(THIS) PURE;
  STDMETHOD_(ID3D12ShaderReflectionConstantBuffer *, GetBuffer)(THIS) PURE;

  STDMETHOD_(UINT, GetInterfaceSlot)(THIS_ _In_ UINT uArrayIndex) PURE;
};

interface DECLSPEC_UUID("E913C351-783D-48CA-A1D1-4F306284AD56") ID3D12ShaderReflectionType;

#undef INTERFACE
#define INTERFACE ID3D12ShaderReflectionType

DECLARE_INTERFACE(ID3D12ShaderReflectionType)
{
  STDMETHOD(GetDesc)(THIS_ _Out_ D3D12_SHADER_TYPE_DESC * pDesc) PURE;

  STDMETHOD_(ID3D12ShaderReflectionType *, GetMemberTypeByIndex)(THIS_ _In_ UINT Index) PURE;
  STDMETHOD_(ID3D12ShaderReflectionType *, GetMemberTypeByName)(THIS_ _In_ LPCSTR Name) PURE;
  STDMETHOD_(LPCSTR, GetMemberTypeName)(THIS_ _In_ UINT Index) PURE;

  STDMETHOD(IsEqual)(THIS_ _In_ ID3D12ShaderReflectionType * pType) PURE;
  STDMETHOD_(ID3D12ShaderReflectionType *, GetSubType)(THIS) PURE;
  STDMETHOD_(ID3D12ShaderReflectionType *, GetBaseClass)(THIS) PURE;
  STDMETHOD_(UINT, GetNumInterfaces)(THIS) PURE;
  STDMETHOD_(ID3D12ShaderReflectionType *, GetInterfaceByIndex)(THIS_ _In_ UINT uIndex) PURE;
  STDMETHOD(IsOfType)(THIS_ _In_ ID3D12ShaderReflectionType * pType) PURE;
  STDMETHOD(ImplementsInterface)(THIS_ _In_ ID3D12ShaderReflectionType * pBase) PURE;
};