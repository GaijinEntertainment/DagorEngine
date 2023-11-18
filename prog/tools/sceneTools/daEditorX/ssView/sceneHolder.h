#pragma once


#include <de3_dynRenderService.h>
#include <render/dag_cur_view.h>
#include <streaming/dag_streamingBase.h>
#include <scene/dag_visibility.h>
#include <ioSys/dag_roDataBlock.h>
#include "decl.h"
#include <debug/dag_debug.h>


class ssviewplugin::SceneHolder : public BaseStreamingSceneHolder
{
public:
  bool skipEnviData;

  SceneHolder() : skipEnviData(false) {}

  virtual bool bdlCustomLoad(unsigned bindump_id, int tag, IGenLoad &crd, dag::ConstSpan<TEXTUREID> tex)
  {
    if (bindump_id != 0xFFFFFFFF || skipEnviData)
      return BaseStreamingSceneHolder::bdlCustomLoad(bindump_id, tag, crd, tex);

    switch (tag)
    {
      case _MAKE4C('POFX'):
      {
        DataBlock postfxBlkTemp;
        RoDataBlock *roblk = RoDataBlock::load(crd);
        postfxBlkTemp.setFrom(*roblk);
        delete roblk;
        EDITORCORE->queryEditorInterface<IDynRenderService>()->restartPostfx(postfxBlkTemp);
      }
        return true;

      case _MAKE4C('ShGV'):
      {
        RoDataBlock *roblk = RoDataBlock::load(crd);
        ShaderGlobal::set_vars_from_blk(*roblk);
        delete roblk;
      }
        return true;

      case _MAKE4C('VISR'):
      {
        float objectToSceenRatio, nearRatioOffset;
        crd.readReal(objectToSceenRatio);
        crd.readReal(nearRatioOffset);
      }
        return true;
    }
    return BaseStreamingSceneHolder::bdlCustomLoad(bindump_id, tag, crd, tex);
  }
};
