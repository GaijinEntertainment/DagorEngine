//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <generic/dag_tab.h>
#include <math/dag_SHlight.h>


class IGenLoad;
class SH3LightingGrid;

typedef bool (*SH3GridVisibilityFunction)(int bindumpId);


class SH3LightingGridManager
{
public:
  // Default lighting to be used without any grids.
  SH3Lighting defaultLighting;

  class Element
  {
  public:
    Element();
    Element(const Element &) = delete;
    ~Element();

    void clear();

    SH3LightingGrid *getLtGrid() const { return ltGrid; }
    unsigned getId() const { return id; }

  protected:
    friend class SH3LightingGridManager;

    SH3LightingGrid *ltGrid;
    unsigned id;
  };


  SH3LightingGridManager(int max_dump_reserve = 32);
  ~SH3LightingGridManager();


  // Returns false if out-of-grid lighting is used (still stored in lt however).
  bool getLighting(const Point3 &pos, SH3Lighting &lt, SH3GridVisibilityFunction is_visible_cb = NULL);


  int addLtGrid(SH3LightingGrid *lt_grid, unsigned id);
  int loadLtGrid(const char *filename, unsigned id);
  int loadLtGridBinary(IGenLoad &crd, unsigned id);
  void delLtGrid(unsigned id);

  void delAllLtGrids();


  dag::Span<Element> getElems() const { return make_span(const_cast<SH3LightingGridManager *>(this)->elems); }

protected:
  Tab<Element> elems;
};
DAG_DECLARE_RELOCATABLE(SH3LightingGridManager::Element);
