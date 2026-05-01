// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <gameDebugRender/textRender.h>

#include <ioSys/dag_dataBlock.h>
#include <util/dag_string.h>
#include <dataBlockUtils/blkIterator.h>
#include <gameDebugRender/textStyle.h>
#include <gameDebugRender/primitives2dCache.h>

struct TextParams
{
  float x = 0.f;
  float indent = 18.f;
  float height = 18.f;
  game_dbg::TextStyle style;
};

static String strBuffer;
static void add_text_block_with_params(game_dbg::Primitives2dCache &render, const DataBlock &text, TextParams params, float &y)
{
  int styleNameId = text.getNameId("_style");
  const DataBlock *styleBlk = styleNameId >= 0 ? text.getBlockByName(styleNameId) : nullptr;
  if (styleBlk)
  {
    params.style.color = text.getE3dcolor("color", params.style.color);
    params.indent = text.getReal("indent", params.indent);
    params.height = text.getReal("height", params.height);
  }

  int titleNameId = text.getNameId("_title");
  const char *titleStr = titleNameId >= 0 ? text.getStrByNameId(titleNameId, nullptr) : nullptr;
  if (titleStr)
  {
    strBuffer.printf(255, "%s:", titleStr);
    render.addText(strBuffer.c_str(), params.x, y, params.style);
    y += params.height;
  }

  params.x += params.indent;

  for (int i = 0; i < text.paramCount(); ++i)
  {
    if (text.getParamNameId(i) == titleNameId)
      continue;

    int paramType = text.getParamType(i);
    if (paramType == DataBlock::TYPE_STRING)
      strBuffer.printf(255, "%s = %s", text.getParamName(i), text.getStr(i));
    else if (paramType == DataBlock::TYPE_INT)
      strBuffer.printf(255, "%s = %d", text.getParamName(i), text.getInt(i));
    else if (paramType == DataBlock::TYPE_REAL)
      strBuffer.printf(255, "%s = %f", text.getParamName(i), text.getReal(i));
    else
      strBuffer = "";

    render.addText(strBuffer.c_str(), params.x, y, params.style);

    y += params.height;
  }

  y += params.height;

  for (const DataBlock &blk : text)
    if (blk.getBlockNameId() != styleNameId)
      add_text_block_with_params(render, blk, params, y);

  y += params.height;
}

void game_dbg::add_text_block(const DataBlock &text, float x, float y, game_dbg::Primitives2dCache &prim_cache)
{
  TextParams params;
  params.x = x;
  add_text_block_with_params(prim_cache, text, params, y);
}
