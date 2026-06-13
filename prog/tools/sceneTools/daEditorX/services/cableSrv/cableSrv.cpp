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
    if (check_nan(start) || check_nan(end) || check_nan(radius) || check_nan(sag))
    {
      DAEDITOR3.conError("NaNs in %s(start=%@, end=%@, radius=%g, sag=%g)", __FUNCTION__, start, end, radius, sag);
      debug_dump_stack("NaNs detected!");
      return ~0u;
    }

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
    if (find_value_idx(removed_cables, id) >= 0)
      DAEDITOR3.conError("double %s(%d) call", __FUNCTION__, id);
    else
      removed_cables.push_back(id);
    cables_mgr->setCable(id, Point3(0, 0, 0), Point3(0, 0, 0), 0, 0);
  }

  void beforeRender(float pixel_scale) override { cables_mgr->beforeRender(pixel_scale); }

  void renderGeometry() override { cables_mgr->render(Cables::RENDER_PASS_TRANS); }

  void exportCables(BinDumpSaveCB &cwr) override
  {
    G_STATIC_ASSERT(sizeof(CableData) == 4 * 4 * 2);

    Tab<CableData> *data = cables_mgr->getCablesData();
    if (data->size() - removed_cables.size() == 0)
      return;
    for (const CableData &cable : *data)
      if (check_nan(cable.point1_rad) || check_nan(cable.point2_sag))
        DAEDITOR3.conError("NaNs in getCablesData[%d]=(point1_rad=%@, point2_sag=%@), getCablesData.size=%d", //
          &cable - data->data(), cable.point1_rad, cable.point2_sag, data->size());

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
      to_export.reserve(data->size() - removed_cables.size());
      for (int i = 0, skipped = 0; i < data->size() - removed_cables.size(); ++i)
      {
        while (skipped < removed_cables.size() && i + skipped == removed_cables[skipped])
          ++skipped;
        to_export.push_back(data->at(i + skipped));
      }

      for (const CableData &cable : to_export)
        if (check_nan(cable.point1_rad) || check_nan(cable.point2_sag))
          DAEDITOR3.conError("NaNs in to_export[%d]=(point1_rad=%@, point2_sag=%@), to_export.size=%d, getCablesData.size=%d",
            &cable - to_export.data(), cable.point1_rad, cable.point2_sag, to_export.size(), data->size());

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
