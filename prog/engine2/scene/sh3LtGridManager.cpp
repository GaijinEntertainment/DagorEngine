#include <scene/dag_sh3LtGrid.h>
#include <scene/dag_sh3LtGridManager.h>
#include <3d/dag_texMgr.h>


SH3LightingGridManager::Element::Element() : ltGrid(NULL), id(unsigned(-1)) {}


SH3LightingGridManager::Element::~Element() { clear(); }


void SH3LightingGridManager::Element::clear()
{
  del_it(ltGrid);
  id = unsigned(-1);
}


SH3LightingGridManager::SH3LightingGridManager(int max_dump_reserve) : elems(midmem_ptr()) { elems.reserve(max_dump_reserve); }


SH3LightingGridManager::~SH3LightingGridManager() { delAllLtGrids(); }


bool SH3LightingGridManager::getLighting(const Point3 &pos, SH3Lighting &lt, SH3GridVisibilityFunction is_visible_cb /*= NULL*/)
{
  bool gotLighting = false;

  int bestIndex = -1;
  float bestHeight = 1e20f;

  for (int i = 0; i < elems.size(); ++i)
  {
    if (!elems[i].ltGrid)
      continue;
    if (is_visible_cb && !(*is_visible_cb)(elems[i].id))
      continue;
    gotLighting = true;

    float height = elems[i].ltGrid->getAltitude(pos);
    if (bestHeight > height && height >= 0)
    {
      bestHeight = height;
      bestIndex = i;
    }
  }
  if (bestIndex != -1)
  {
    if (elems[bestIndex].ltGrid->getLight(pos, lt))
      return true;
  }

  if (!gotLighting)
    lt = defaultLighting;

  return false;
}


int SH3LightingGridManager::addLtGrid(SH3LightingGrid *lt_grid, unsigned id)
{
  int i;
  for (i = 0; i < elems.size(); ++i)
    if (!elems[i].ltGrid)
      break;

  if (i >= elems.size())
  {
    i = append_items(elems, 1);
  }

  elems[i].ltGrid = lt_grid;
  elems[i].id = id;

  return i;
}


int SH3LightingGridManager::loadLtGrid(const char *filename, unsigned id)
{
  SH3LightingGrid *ltGrid = new (midmem) SH3LightingGrid;

  if (!ltGrid->load(filename))
  {
    delete ltGrid;
    return -1;
  }

  return addLtGrid(ltGrid, id);
}


int SH3LightingGridManager::loadLtGridBinary(IGenLoad &crd, unsigned id)
{
  SH3LightingGrid *ltGrid = new (midmem) SH3LightingGrid;

  if (!ltGrid->loadBinary(crd))
  {
    delete ltGrid;
    return -1;
  }

  return addLtGrid(ltGrid, id);
}


void SH3LightingGridManager::delLtGrid(unsigned id)
{
  int i;
  for (i = 0; i < elems.size(); ++i)
  {
    if (!elems[i].ltGrid)
      continue;
    if (elems[i].id != id)
      continue;

    elems[i].clear();
    break;
  }

  if (i >= elems.size())
    return;

  // erase empty tail elements
  int j;
  for (j = i + 1; j < elems.size(); ++j)
    if (elems[j].ltGrid)
      break;

  if (j >= elems.size())
    erase_items(elems, i, elems.size() - i);
}


void SH3LightingGridManager::delAllLtGrids() { elems.clear(); }
