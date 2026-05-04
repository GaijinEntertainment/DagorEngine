// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "decompressBinaryMessage.h"

#include "jsonRpcProtocol.h"

#include <debug/dag_assert.h>

#include <zstd.h>

#include <EASTL/deque.h>
#include <EASTL/optional.h>


namespace websocketjsonrpc::details
{

static eastl::deque<MemoryChunk> zstd_decompress_buffer_to_chunks(eastl::string_view compressed, eastl::string &error_details)
{
  eastl::deque<MemoryChunk> outputChunks;

  eastl::unique_ptr<ZSTD_DStream, decltype(&ZSTD_freeDStream)> stream(ZSTD_createDStream(), &ZSTD_freeDStream);
  if (!stream)
  {
    error_details = "failed to initialize decompressor";
    return outputChunks;
  }

  ZSTD_inBuffer input = {compressed.data(), compressed.size(), 0};
  const size_t outBufferSize = eastl::max(ZSTD_DStreamOutSize(), compressed.size());
  size_t lastDecompressionResult = 0;

  // Given a valid frame, zstd won't consume the last byte of the frame
  // until it has flushed all of the decompressed data of the frame.
  // Therefore, instead of checking if the return code is 0, we can
  // decompress just check if input.pos < input.size.
  // The source of the information above:
  // https://github.com/facebook/zstd/blob/v1.5.7/examples/streaming_decompression.c#L42C1-L46C12

  while (input.pos < input.size)
  {
    eastl::unique_ptr<char[]> outChunk(new char[outBufferSize]);
    ZSTD_outBuffer output = {outChunk.get(), outBufferSize, 0};

    // The return code is zero if the frame is complete, but there may
    // be multiple frames concatenated together. Zstd will automatically
    // reset the context when a frame is complete. Still, calling
    // ZSTD_DCtx_reset() can be useful to reset the context to a clean
    // state, for instance if the last decompression call returned an
    // error.
    // The source of the information above:
    // https://github.com/facebook/zstd/blob/v1.5.7/examples/streaming_decompression.c#L49C1-L55C16

    lastDecompressionResult = ZSTD_decompressStream(stream.get(), &output, &input);
    if (ZSTD_isError(lastDecompressionResult))
    {
      error_details = ZSTD_getErrorName(lastDecompressionResult);
      return outputChunks;
    }

    if (output.pos > 0)
    {
      outputChunks.emplace_back(eastl::move(outChunk), output.pos);
    }
  }

  if (lastDecompressionResult != 0)
  {
    // The last return value from ZSTD_decompressStream did not end on a
    // frame, but we reached the end of the file! We assume this is an
    // error, and the input was truncated.
    // The source of the information above:
    // https://github.com/facebook/zstd/blob/v1.5.7/examples/streaming_decompression.c#L69C1-L72C12

    error_details = "reached end of input buffer before end of compressed stream";
    return outputChunks;
  }

  return outputChunks;
}


static MemoryChunk zstd_decompress_buffer(eastl::string_view compressed_data, eastl::string &error_details)
{
  eastl::string lowerErrorDetails;
  eastl::deque<MemoryChunk> chunks = zstd_decompress_buffer_to_chunks(compressed_data, lowerErrorDetails);
  if (!lowerErrorDetails.empty())
  {
    error_details.sprintf("failed zstd decompression of %llu bytes: %s", (unsigned long long)compressed_data.size(),
      lowerErrorDetails.c_str());
    return {};
  }

  size_t totalSize = 0;
  for (const MemoryChunk &chunk : chunks)
  {
    totalSize += chunk.size;
  }

  if (totalSize == 0)
  {
    return {};
  }

  if (chunks.size() == 1)
  {
    // If we have only one chunk, we can return it directly
    return eastl::move(chunks.front());
  }

  MemoryChunk resultChunk(eastl::unique_ptr<char[]>(new char[totalSize]), totalSize);
  char *outPtr = resultChunk.data.get();

  for (const MemoryChunk &chunk : chunks)
  {
    if (chunk.data.get() == nullptr)
    {
      G_ASSERT(chunk.size == 0); // check MemoryChunk contract
      continue;                  // avoid UB in memcpy
    }

    std::memcpy(outPtr, chunk.data.get(), chunk.size);
    outPtr += chunk.size;
  }

  return resultChunk;
}


MemoryChunk decompress_binary_message(eastl::string_view binary_message_data, const WsJsonRpcConnectionConfig &config,
  eastl::string &error_details)
{
  // `binary_message_data` format:
  // (this is a fragment of documentation from `prog/gameLibs/sepClient/sep_protocol.md`)
  // WebSocket binary message will have a zero-terminated compression name as a prefix.
  // Here is an example of such prefix for zstd algorithm:
  // 0000  |  7A 73 74 64 00  |  zstd\0
  // Max allowed prefix length is 100 ASCII bytes including the terminating zero.

  const size_t posEndOfPrefix = binary_message_data.substr(0, protocol::binary_message::MESSAGE_PREFIX_MAX_LENGTH).find('\0');
  if (posEndOfPrefix == eastl::string_view::npos)
  {
    error_details = "binary SEP message: invalid prefix";
    return {};
  }

  const eastl::string_view compressionAlgorithm = binary_message_data.substr(0, posEndOfPrefix);
  const eastl::string_view compressedData = binary_message_data.substr(posEndOfPrefix + 1);

  if (compressionAlgorithm == protocol::binary_message::COMRESSION_ALGO_ZSTD && config.enableZstdDecompressionSupport)
  {
    return zstd_decompress_buffer(compressedData, error_details);
  }

  error_details.sprintf("binary SEP message: unsupported compression algorithm: %.*s", static_cast<int>(compressionAlgorithm.size()),
    compressionAlgorithm.data());
  return {};
}

} // namespace websocketjsonrpc::details
