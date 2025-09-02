// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ioSys/dag_genIo.h>

#include <propPanel/c_common.h>
#include "tunedParams.h"

using namespace ScriptHelpers;


namespace ScriptHelpers
{
class TunedGradientBoxParam : public TunedElement
{
public:
  static const int MAX_POINTS = 64;

  Tab<PropPanel::GradientKey> values;
  TunedGradientBoxParam(const char *nm) { name = nm; }
  ~TunedGradientBoxParam() override {}

  //---------------------------------------------

  TunedElement *cloneElem() override { return new TunedGradientBoxParam(*this); }

  int subElemCount() const override { return 0; }
  TunedElement *getSubElem(int index) const override { return NULL; }

  void saveData(mkbindump::BinDumpSaveCB &cwr, SaveDataCB *save_cb) override
  {
    cwr.writeInt32e((uint32_t)values.size());
    for (size_t i = 0; i < values.size(); ++i)
    {
      cwr.writeReal(values[i].position);
      cwr.writeInt32e(values[i].color);
    }
  }

  void saveValues(DataBlock &blk, SaveValuesCB *save_cb) override
  {
    for (int i = 0; i < values.size(); ++i)
    {
      blk.setReal(String(32, "pos_%d", i), values[i].position);
      blk.setE3dcolor(String(32, "color_%d", i), values[i].color);
    }

    for (size_t i = values.size(); i < MAX_POINTS; ++i)
    {
      blk.removeParam(String(32, "pos_%d", i));
      blk.removeParam(String(32, "color_%d", i));
    }
  }

  void loadValues(const DataBlock &blk) override
  {
    clear_and_shrink(values);

    for (int i = 0; i < MAX_POINTS; ++i)
      if (blk.paramExists(String(32, "pos_%d", i)))
      {
        PropPanel::GradientKey v;
        v.position = blk.getReal(String(32, "pos_%d", i), 0);
        v.color = blk.getE3dcolor(String(32, "color_%d", i), 0xffffffff);
        v.position = clamp<float>(v.position, 0, 1);
        values.push_back(v);
      }
      else
        break;
  }
};

TunedElement *create_tuned_gradient_box(const char *name) { return new TunedGradientBoxParam(name); }

}; // namespace ScriptHelpers
