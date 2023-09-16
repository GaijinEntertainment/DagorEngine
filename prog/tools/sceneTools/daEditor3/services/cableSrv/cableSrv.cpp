#include <de3_interface.h>
#include <generic/dag_sort.h>
#include <EditorCore/ec_interface_ex.h>
#include <libTools/util/makeBindump.h>
#include <de3_cableSrv.h>
#include <render/cables.h>

constexpr int MAX_CABLES = 16384;

class CableService : public ICableService
{
public:
  Cables *cables_mgr = NULL;
  Tab<cable_handle_t> removed_cables;
  CableService() {}
  ~CableService() { close_cables_mgr(); }

  void init()
  {
    init_cables_mgr();
    cables_mgr = get_cables_mgr();
    cables_mgr->setMaxCables(MAX_CABLES);
  }

  virtual cable_handle_t addCable(const Point3 &start, const Point3 &end, float radius, float sag)
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

  virtual void removeCable(cable_handle_t id)
  {
    removed_cables.push_back(id);
    cables_mgr->setCable(id, Point3(0, 0, 0), Point3(0, 0, 0), 0, 0);
  }

  virtual void beforeRender(float pixel_scale) { cables_mgr->beforeRender(pixel_scale); }

  virtual void renderGeometry() { cables_mgr->render(Cables::RENDER_PASS_TRANS); }

  virtual void exportCables(BinDumpSaveCB &cwr)
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
      fast_sort(removed_cables, std::less<int>());
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

void *get_generic_cable_service()
{
  static bool is_inited = false;
  if (!is_inited)
  {
    is_inited = true;
    srv.init();
  }
  return &srv;
}
