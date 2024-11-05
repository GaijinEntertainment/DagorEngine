// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <bulkBlkPack.h>
#include <ioSys/dag_memIo.h>
#include <memSilentInPlaceLoadCB.h>
#include <util/dag_string.h>


void BlobBlkProcessor::fillOutStream(DynamicMemGeneralSaveCB &outData) const { blk.saveToStream(outData); }


bool BlobBlkProcessor::processInputData(const void *data, int size)
{
  MemSilentInPlaceLoadCB crd(data, size);

  struct BlkErrReporter : public DataBlock::IErrorReporterPipe
  {
    ~BlkErrReporter() noexcept
    {
      if (error.empty())
        return;

      DAGOR_TRY { logwarn("BlkError: %s: %s.", "BlobBlkProcessor::processInputData", error); }
      DAGOR_CATCH(...) {}
    }
    void reportError(const char *text, bool fatal) override
    {
      if (error.size() > 1024)
        return;

      error.aprintf(0, "%s%s%s", !error.empty() ? "; " : "", fatal ? "Fatal: " : "", text != nullptr ? text : "");

      if (error.size() > 1024)
        error.append("...");
    }
    String error;
  } rep;
  DataBlock::InstallReporterRAII irep(&rep);

  bool res = dblk::load_from_stream(blk, crd, dblk::ReadFlag::ROBUST | dblk::ReadFlag::BINARY_ONLY);
  clear_and_shrink(decompressData);

  return res;
}


void *BlobBlkProcessor::requestBufferForDecompressedData(int size)
{
  decompressData.resize(size);

  return decompressData.data();
}
