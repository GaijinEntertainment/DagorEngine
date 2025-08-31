// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <dasModules/dasScriptsLoader.h>

#include <ioSys/dag_dataBlock.h>
#include <util/dag_threadPool.h>
#include <memory/dag_framemem.h>
#include "../../gameLibs/ecs/scripts/das/das_ecs.h"
#include <zstd.h>
#include <hash/crc32c.h>

namespace bind_dascript
{
FramememStackAllocator::FramememStackAllocator(uint32_t size) : das::StackAllocator(size, size ? framemem_ptr()->alloc(size) : nullptr)
{}
FramememStackAllocator::~FramememStackAllocator()
{
  if (stack)
    framemem_ptr()->free(eastl::exchange(stack, nullptr));
}

bool debugSerialization = false;
bool enableSerialization = false;
bool serializationReading = false;
bool suppressSerialization = false;
eastl::string deserializationFileName;
eastl::string serializationFileName;
das::unique_ptr<das::SerializationStorage> initSerializerStorage;
das::unique_ptr<das::SerializationStorage> initDeserializerStorage;
das::unique_ptr<das::AstSerializer> initSerializer;
das::unique_ptr<das::AstSerializer> initDeserializer;


size_t FileSerializationStorage::writingSize() const { return df_tell(file); }

bool FileSerializationStorage::readOverflow(void *data, size_t size)
{
  const int read = df_read(file, data, size);
  return read == size;
}

void FileSerializationStorage::write(const void *data, size_t size)
{
  const int res = df_write(file, data, (int)size);
  if (res != size)
    logerr("can't write to serialization storage '%@'", fileName.c_str());
}

FileSerializationStorage::~FileSerializationStorage() { df_close(file); }

// ----------------------------

struct FileSerializationRead final : FileSerializationStorage
{
  int buffInSize = ZSTD_DStreamInSize();
  int readPos = 0, fileLen = 0;
  const unsigned char *fileMapping = nullptr;
  ZSTD_DCtx *dctx = ZSTD_createDCtx();
  das::vector<char> readBuffer;
  FileSerializationRead(file_ptr_t file_, const das::string &name);
  virtual size_t writingSize() const override { return 0; }
  virtual bool readOverflow(void *data, size_t size) override;
  virtual void write(const void *data, size_t size) override;
  virtual ~FileSerializationRead() override;

  // Big buffer, must be last member of this struct
  char tempReadBuffer[ZSTD_BLOCKSIZE_MAX];
};


FileSerializationRead::FileSerializationRead(file_ptr_t file_, const das::string &name) : //-V730
  FileSerializationStorage(file_, name)
{
  fileMapping = (const unsigned char *)df_mmap(file, &fileLen);
  if (fileMapping)
  {
    if (fileLen <= sizeof(int))
      ;
    else if (*(const int *)fileMapping == ZSTD_MAGICNUMBER)
      return; // Bw-compat with cksum-less data
    else
    {
      uint32_t cksum = crc32c_append(0, fileMapping + sizeof(int), fileLen - sizeof(int));
      if (cksum == *(const uint32_t *)fileMapping)
      {
        readPos = sizeof(cksum);
        return;
      }
      else
        logerr("das: serialize: FileSerializationRead: %s cksum mismatch: %08x != %08x", name.c_str(), cksum,
          *(const uint32_t *)fileMapping);
    }
  }
  else // Low memory condition?
  {
    logwarn("das: serialize: failed to mmap file %s of len %d, fallback to partial read", name.c_str(), fileLen);
    uint32_t cksum = 0;
    if (df_read(file, &cksum, sizeof(cksum)) == sizeof(cksum))
    {
      if (cksum == ZSTD_MAGICNUMBER)
        df_seek_to(file, 0); // Bw-compat with hash-less data
      readBuffer.resize_noinit(buffInSize);
      return;
    }
  }
  // Error condition here
  buffInSize = 0;
}

FileSerializationRead::~FileSerializationRead()
{
  df_unmap(fileMapping, fileLen);
  ZSTD_freeDCtx(dctx);
}

bool FileSerializationRead::readOverflow(void *data, size_t size)
{
  if (!buffInSize) // Mismatched hash?
    return false;

  char *dataPtr = (char *)data;

  while (size)
  {
    if (buffer.empty())
    {
      ZSTD_inBuffer input;
      if (DAGOR_LIKELY(fileMapping))
      {
        int toRead = min(fileLen - readPos, buffInSize);
        if (toRead <= 0)
          return false;
        input = {fileMapping + readPos, (size_t)toRead, 0};
        readPos += toRead; //-V1026 ignore signed overflow possibility
      }
      else
      {
        int readBufferSize = df_read(file, readBuffer.data(), readBuffer.size());
        if (readBufferSize <= 0)
          return false;
        input = {readBuffer.data(), (size_t)readBufferSize, 0};
      }
      buffer.clear();
      while (input.pos < input.size) // To consider: decompress directly to `buffer` instead of `tempReadBuffer`
      {
        ZSTD_outBuffer output = {tempReadBuffer, sizeof(tempReadBuffer), 0};
        const size_t ret = ZSTD_decompressStream(dctx, &output, &input);
        if (DAGOR_UNLIKELY(ZSTD_isError(ret))) // decompression error, maybe previous format file
        {
          logwarn("zstd read %s err=%#x(%s) at %d", fileName.c_str(), (int)ret, ZSTD_getErrorName(ret),
            int(((const uint8_t *)input.src + input.pos) - fileMapping));
          return false;
        }
        auto bSize = buffer.size();
        buffer.resize_noinit(bSize + output.pos);
        memcpy(buffer.data() + bSize, tempReadBuffer, output.pos);
      }
    }

    const size_t bytesToRead = eastl::min(size, buffer.size() - bufferPos);
    if (bytesToRead == 0)
    {
      return false;
    }
    memcpy(dataPtr, buffer.data() + bufferPos, bytesToRead);
    bufferPos += bytesToRead;
    dataPtr += bytesToRead;
    size -= bytesToRead;

    if (bufferPos == buffer.size())
    {
      buffer.resize(0);
      bufferPos = 0;
    }
  }
  return true;
}

void FileSerializationRead::write(const void *, size_t) { logerr("can't write to read-only serialization storage"); }
// ----------------------------


struct FileSerializationWrite final : FileSerializationStorage
{
  das::vector<uint8_t> writeBuffer;
  size_t totalSize = 0;
  ZSTD_CCtx *cctx = nullptr;
  uint32_t cksum = 0;

  FileSerializationWrite(file_ptr_t file_, const das::string &name) : FileSerializationStorage(file_, name) {} // -V730
  virtual ~FileSerializationWrite() override;
  virtual size_t writingSize() const override;
  virtual bool readOverflow(void *data, size_t size) override;
  virtual void write(const void *data, size_t size) override;
  void flush(ZSTD_EndDirective end_mode);

  void init();
};

void FileSerializationWrite::init()
{
  cctx = ZSTD_createCCtx();
  ZSTD_CCtx_setParameter(cctx, ZSTD_c_compressionLevel, 1);
  ZSTD_CCtx_setParameter(cctx, ZSTD_c_checksumFlag, 1);

  size_t const buffInSize = ZSTD_CStreamInSize();
  size_t const buffOutSize = ZSTD_CStreamOutSize();
  buffer.resize(buffInSize);
  writeBuffer.resize_noinit(buffOutSize);

  df_write(file, &cksum, sizeof(cksum));
}

size_t FileSerializationWrite::writingSize() const { return totalSize; }

bool FileSerializationWrite::readOverflow(void *, size_t)
{
  logerr("can't read from write-only serialization storage");
  return false;
}

void FileSerializationWrite::write(const void *data, size_t size)
{
  if (DAGOR_UNLIKELY(!cctx))
    init();
  char *dataPtr = (char *)data;
  while (size)
  {
    const size_t bytesToWrite = eastl::min(size, buffer.size() - bufferPos);
    memcpy(buffer.data() + bufferPos, dataPtr, bytesToWrite);
    bufferPos += bytesToWrite;
    dataPtr += bytesToWrite;
    size -= bytesToWrite;
    totalSize += bytesToWrite;
    if (bufferPos == buffer.size())
      flush(ZSTD_e_continue);
  }
}

void FileSerializationWrite::flush(ZSTD_EndDirective end_mode)
{
  int finished;
  do
  {
    ZSTD_inBuffer input = {buffer.data(), (size_t)bufferPos, 0};
    ZSTD_outBuffer output = {writeBuffer.data(), writeBuffer.size(), 0};
    size_t const remaining = ZSTD_compressStream2(cctx, &output, &input, end_mode);
    G_ASSERT(!ZSTD_isError(remaining));
    df_write(file, writeBuffer.data(), (int)output.pos);
    cksum = crc32c_append(cksum, writeBuffer.data(), output.pos);
    bufferPos = 0;
    finished = end_mode == ZSTD_e_end ? (remaining == 0) : (input.pos == input.size);
  } while (!finished);
}

FileSerializationWrite::~FileSerializationWrite()
{
  if (!cctx)
    return;

  flush(ZSTD_e_end);

  df_seek_to(file, 0);
  df_write(file, &cksum, sizeof(cksum));

  ZSTD_freeCCtx(cctx);
}

das::SerializationStorage *create_file_read_serialization_storage(file_ptr_t file, const das::string &name)
{
  return new FileSerializationRead(file, name);
}
das::SerializationStorage *create_file_write_serialization_storage(file_ptr_t file, const das::string &name)
{
  return new FileSerializationWrite(file, name);
}

} // namespace bind_dascript
