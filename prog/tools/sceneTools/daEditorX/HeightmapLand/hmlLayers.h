#pragma once

#include <util/dag_oaHashNameMap.h>
class DataBlock;

struct EditLayerProps
{
  enum
  {
    ENT = 0,
    SPL,
    PLG,
    TYPENUM
  };
  static const int MAX_LAYERS = 60;

  unsigned lock : 1, hide : 1, renderToMask : 1;
  unsigned type : 3;
  unsigned nameId : 10;

  EditLayerProps() : lock(0), hide(0), renderToMask(0), type(TYPENUM), nameId(0) {}
  EditLayerProps(unsigned t, unsigned nid) : lock(0), hide(0), renderToMask(0), type(t), nameId(nid) {}
  const char *name() const { return layerNames.getName(nameId); }

public:
  static FastNameMapEx layerNames;
  static Tab<EditLayerProps> layerProps;
  static int activeLayerIdx[TYPENUM];

  static void resetLayersToDefauls();
  static void loadLayersConfig(const DataBlock &blk, const DataBlock &local_data);
  static void saveLayersConfig(DataBlock &blk, DataBlock &local_data);
  static int findLayer(unsigned t, unsigned nid)
  {
    for (int i = 0; i < layerProps.size(); i++)
      if (layerProps[i].type == t && layerProps[i].nameId == nid)
        return i;
    return -1;
  }
  static int findLayerOrCreate(unsigned t, unsigned nid)
  {
    int lidx = findLayer(t, nid);
    if (lidx < 0)
    {
      lidx = layerProps.size();
      layerProps.push_back(EditLayerProps(t, nid));
    }
    return lidx;
  }
  static const char *layerName(int idx) { return layerProps[idx].name(); }
};
