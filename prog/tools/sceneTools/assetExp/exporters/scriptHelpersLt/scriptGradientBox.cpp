#include <ioSys/dag_genIo.h>

#include <propPanel2/c_common.h>
#include "tunedParams.h"

using namespace ScriptHelpers;


namespace ScriptHelpers
{
class TunedGradientBoxParam : public TunedElement
{
public:
  static const int MAX_POINTS = 64;

  Tab<GradientKey> values;
  TunedGradientBoxParam(const char *nm) { name = nm; }
  ~TunedGradientBoxParam() {}

  //---------------------------------------------

  virtual TunedElement *cloneElem() { return new TunedGradientBoxParam(*this); }

  virtual int subElemCount() const { return 0; }
  virtual TunedElement *getSubElem(int index) const { return NULL; }

  virtual void saveData(mkbindump::BinDumpSaveCB &cwr, SaveDataCB *save_cb)
  {
    cwr.writeInt32e((uint32_t)values.size());
    for (size_t i = 0; i < values.size(); ++i)
    {
      cwr.writeReal(values[i].position);
      cwr.writeInt32e(values[i].color);
    }
  }

  virtual void saveValues(DataBlock &blk, SaveValuesCB *save_cb)
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

  virtual void loadValues(const DataBlock &blk)
  {
    clear_and_shrink(values);

    for (int i = 0; i < MAX_POINTS; ++i)
      if (blk.paramExists(String(32, "pos_%d", i)))
      {
        GradientKey v;
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
