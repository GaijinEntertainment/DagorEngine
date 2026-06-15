// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <dag/dag_vector.h>
#include <generic/dag_span.h>
#include <shaders/dag_shaderHash.h>

typedef struct D3D12_GRAPHICS_PIPELINE_STATE_DESC D3D12_GRAPHICS_PIPELINE_STATE_DESC;
typedef struct D3D12_COMPUTE_PIPELINE_STATE_DESC D3D12_COMPUTE_PIPELINE_STATE_DESC;
typedef struct D3D12_PIPELINE_STATE_STREAM_DESC D3D12_PIPELINE_STATE_STREAM_DESC;
typedef struct ID3D12RootSignature ID3D12RootSignature;

using SerializedGpuPipeline = dag::Vector<uint8_t>;

/// Some settings that control what is how encoded into the serialized data.
struct SerializationSettings
{
  /// Serialize hashes of shaders instead of the byte code it self. This reduces the size of the serialized data, but needs a matching
  /// shader bin dump for later proper deserialization and testing.
  bool shaderHashes : 1 = false;
};

using RootSignatureByteCode = dag::Span<const uint8_t>;

/// Attempts to serialize the given desc and root signature into a portable binary format.
/// @param settings The settings that control aspects of the serialization process.
/// @param desc The description of the graphics pipeline that should be serialized.
/// @param root_signature The byte code that describes the root signature of the graphics pipeline.
/// @returns On success a non empty byte vector with the serialized data, on error the vector will be empty.
SerializedGpuPipeline serialize_graphics_pipeline(const SerializationSettings &settings,
  const D3D12_GRAPHICS_PIPELINE_STATE_DESC &desc, const RootSignatureByteCode &root_signature);
/// Attempts to serialize the given desc and root signature into a portable binary format.
/// @param settings The settings that control aspects of the serialization process.
/// @param desc The description of the compute pipeline that should be serialized.
/// @param root_signature The byte code that describes the root signature of the compute pipeline.
/// @returns On success a non empty byte vector with the serialized data, on error the vector will be empty.
SerializedGpuPipeline serialize_compute_pipeline(const SerializationSettings &settings, const D3D12_COMPUTE_PIPELINE_STATE_DESC &desc,
  const RootSignatureByteCode &root_signature);
/// Attempts to serialize the given desc and root signature into a portable binary format.
/// @param settings The settings that control aspects of the serialization process.
/// @param desc The description of the pipeline that should be serialized.
/// @param root_signature The byte code that describes the root signature of the pipeline.
/// @returns On success a non empty byte vector with the serialized data, on error the vector will be empty.
SerializedGpuPipeline serialize_pipeline(const SerializationSettings &settings, const D3D12_PIPELINE_STATE_STREAM_DESC &desc,
  const RootSignatureByteCode &root_signature);

/// A receiver interface for deserialization of serialized pipeline descriptions.
struct DeserializerReceiver
{
  /// Just the default for proper virtual interface.
  virtual ~DeserializerReceiver() = default;

  /// Invoked when the deserializer found a root signature and wants to translate its byte code into a proper root signature object
  /// that is later used to initialize the desc structures properly.
  /// @param byte_code Byte code that defines a root signature.
  /// @returns The result of ID3D12Device::CreateRootSignature that should be used to initialize the desc data structures for the other
  /// on* method invocations.
  virtual ID3D12RootSignature *onRootSignature(const RootSignatureByteCode &byte_code) = 0;
  /// Invoked when the deserializer found a graphics pipeline description and finished translating it into a desc structure, it may
  /// have invoked onRootSignature beforehand to translate the paired root signature byte code into a root signature object.
  /// @param desc Description describing a graphics pipeline.
  virtual void onGraphicsPipeline(const D3D12_GRAPHICS_PIPELINE_STATE_DESC &desc) = 0;
  /// Invoked when the deserializer found a compute pipeline description and finished translating it into a desc structure, it may have
  /// invoked onRootSignature beforehand to translate the paired root signature byte code into a root signature object.
  /// @param desc Description describing a graphics pipeline.
  virtual void onComputePipeline(const D3D12_COMPUTE_PIPELINE_STATE_DESC &desc) = 0;
  /// Invoked when the deserializer found a pipeline description and finished translating it into a desc structure, it may have invoked
  /// onRootSignature beforehand to translate the paired root signature byte code into a root signature object.
  /// @param desc Description describing a pipeline.
  virtual void onPipeline(const D3D12_PIPELINE_STATE_STREAM_DESC &desc) = 0;
  /// Type of shader, eg shader stage.
  enum class ShaderType
  {
    /// Vertex shader
    VS,
    /// Pixel or fragment shader
    PS,
    /// Geometry shader
    GS,
    /// Domain shader
    DS,
    /// Hull shader
    HS,
    /// Compute shader
    CS,
    /// Mesh shader
    MS,
    /// Amplification shader
    AS
  };
  /// Invoked when the pipeline was serialized with 'shaderHashes' mode enabled. This handler is supposed to translate the hash
  /// value back into the matching shader binary.
  /// @param shader_type Type of the shader the hash should be resolved for.
  /// @param shader_binary_size Size of the shader binary the hash value refers to.
  /// @param hash Hash value of the shader binary to get.
  /// @returns A pointer to a shader binary with the given size and matching hash if itself. Should return nullptr when not available.
  virtual const void *onShaderHash(ShaderType shader_type, size_t shader_binary_size, const ShaderHashValue &hash) = 0;
};
/// Attempts to deserialize a serialized pipeline of any type.
/// @param data Serialized data.
/// @param receiver The receiver for the deserialized data structures.
/// May not invoke any of the on* methods of receiver when a decoding error was encountered.
void deserialize_pipeline(dag::Span<const uint8_t> data, DeserializerReceiver &receiver);
