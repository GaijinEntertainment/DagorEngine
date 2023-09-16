#include "mat_shader.h"

#include <ioSys/dag_dataBlock.h>


MatShader::MatShader(const DataBlock &blk) : parameters(midmem)
{
  name = blk.getBlockName();

  for (int bi = 0; bi < blk.blockCount(); ++bi)
  {
    const DataBlock *paramBlk = blk.getBlock(bi);

    if (paramBlk)
    {
      ParamRec rec;

      rec.name = paramBlk->getBlockName();
      rec.isBinding = paramBlk->getBool("binding", false);

      parameters.push_back(rec);
    }
  }
}
