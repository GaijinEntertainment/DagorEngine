#include <ioSys/dag_brotliIo.h>
#include <brotli/decode.h>

BrotliStreamDecompress::BrotliStreamDecompress() // -V730
{
  state = BrotliDecoderCreateInstance(NULL, NULL, NULL);
}

BrotliStreamDecompress::~BrotliStreamDecompress()
{
  if (state)
    BrotliDecoderDestroyInstance(state);
}

StreamDecompressResult BrotliStreamDecompress::decompress(dag::ConstSpan<uint8_t> in, Tab<char> &out, size_t *nbytes_read)
{
  G_ASSERT_RETURN(state, StreamDecompressResult::FAILED);
  BrotliDecoderResult res = BROTLI_DECODER_RESULT_NEEDS_MORE_OUTPUT;
  const uint8_t *src = in.data();
  size_t size = in.size();
  while (size || res == BROTLI_DECODER_RESULT_NEEDS_MORE_OUTPUT)
  {
    size_t outSize = TEMP_BUFFER_SIZE;
    uint8_t *dst = (uint8_t *)tmpBuffer;
    res = BrotliDecoderDecompressStream(state, &size, &src, &outSize, &dst, nullptr);
    size_t total = TEMP_BUFFER_SIZE - outSize;
    if (total)
      out.insert(out.end(), tmpBuffer, tmpBuffer + total);
    if (res == BROTLI_DECODER_RESULT_NEEDS_MORE_INPUT)
    {
      if (nbytes_read)
        *nbytes_read = in.size() - size;
      return StreamDecompressResult::NEED_MORE_INPUT;
    }
    if (res == BROTLI_DECODER_RESULT_SUCCESS)
    {
      BrotliDecoderDestroyInstance(state);
      state = nullptr;
      break;
    }
    if (res == BROTLI_DECODER_RESULT_NEEDS_MORE_OUTPUT)
      continue;
    else
      return StreamDecompressResult::FAILED;
  }
  return StreamDecompressResult::FINISH;
}
