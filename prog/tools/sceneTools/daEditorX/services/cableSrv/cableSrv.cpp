// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <de3_interface.h>
#include <generic/dag_sort.h>
#include <EditorCore/ec_interface_ex.h>
#include <libTools/util/makeBindump.h>
#include <de3_cableSrv.h>
#include <render/cables.h>

#include <EASTL/functional.h>

constexpr int MAX_CABLES = 16384;

class CableService : public ICableService
{
public:
  Cables *cables_mgr = NULL;
  Tab<cable_handle_t> removed_cables;
  CableService() {}
  ~CableService() { release(); }

  void init()
  {
    init_cables_mgr();
    cables_mgr = get_cables_mgr();
    cables_mgr->setMaxCables(MAX_CABLES);
  }

  void release()
  {
    close_cables_mgr();
    cables_mgr = nullptr;
  }

  cable_handle_t addCable(const Point3 &start, const Point3 &end, float radius, float sag) override
  {
    if (removed_cables.size())
    {
      cable_handle_t id = removed_cables.back();
      cables_mgr->setCable(id, start, end, radius, sag);
      removed_cables.pop_back();
      return id;
    }
    return cables_mgr->addCable(start, end, radius, sag);
  }

  void removeCable(cable_handle_t id) override
  {
    removed_cables.push_back(id);
    cables_mgr->setCable(id, Point3(0, 0, 0), Point3(0, 0, 0), 0, 0);
  }

  void beforeRender(float pixel_scale) override { cables_mgr->beforeRender(pixel_scale); }

  void renderGeometry() override { cables_mgr->render(Cables::RENDER_PASS_TRANS); }

  void exportCables(BinDumpSaveCB &cwr) override
  {
    Tab<CableData> *data = cables_mgr->getCablesData();
    if (data->size() - removed_cables.size() == 0)
      return;
    cwr.beginTaggedBlock(_MAKE4C('Wire'));
    if (!removed_cables.size())
    {
      cwr.writeInt32e(data->size());
      cwr.writeTabData32ex(*data);
    }
    else
    {
      fast_sort(removed_cables, eastl::less<int>());
      Tab<CableData> to_export;
      int skipped = 0;
      for (int i = 0; i < data->size() - removed_cables.size(); ++i)
      {
        while (i + skipped == removed_cables[skipped])
          ++skipped;
        to_export.push_back(data->at(i + skipped));
      }
      cwr.writeInt32e(to_export.size());
      cwr.writeTabData32ex(to_export);
    }
    cwr.align8();
    cwr.endBlock();
  }
};


static CableService srv;
static bool is_inited = false;

void *get_generic_cable_service()
{
  if (!is_inited)
  {
    is_inited = true;
    srv.init();
  }
  return &srv;
}

void release_generic_cable_service()
{
  if (is_inited)
  {
    is_inited = false;
    srv.release();
  }
}
