//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <generic/dag_tab.h>
#include <math/dag_Point3.h>
#include <math/dag_color.h>


class IGenLoad;
class SH3LightingData;

class SH3Lighting;
struct SHDirLighting;


class SH3LightingManager
{
public:
  struct LightingContext;

  class IRayTracer
  {
  public:
    virtual bool rayHit(const Point3 &p, const Point3 &norm_dir, float max_dist) = 0;
    virtual bool rayTrace(const Point3 &p, const Point3 &norm_dir, float max_dist, float &out_dist) = 0;
  };

  class Element
  {
  public:
    Element();
    Element(const Element &) = delete;
    ~Element();

    void clear();

    SH3LightingData *getLtData() const { return ltData; }
    unsigned getId() const { return id; }

  protected:
    SH3LightingData *ltData;
    unsigned id;

    friend class SH3LightingManager;
  };


  SH3LightingManager(int max_dump_reserve = 32);
  ~SH3LightingManager();


  //! get lighting as SH3 in point
  //! returns false if out-of-grid lighting is used (still stored in lt however).
  bool getLighting(const Point3 &pos, SH3Lighting &lt);

  //! get lighting as (2 dirlt + ambient) in point, optionally forces 1st dirlt to be sun
  void getLighting(const Point3 &pos, SHDirLighting &lt, bool sun_as_dir1, int *lt1_id, int *lt2_id);

  //! creates context for getting multisampled ligthing
  LightingContext *createContext();
  //! destroys context
  void destroyContext(LightingContext *ctx);

  //! resets lighting in context
  void startGetLighting(LightingContext *ctx, int ring_sz = 0);
  //! adds one light sample to context
  void addLightingToCtx(const Point3 &pos, LightingContext *ctx, bool sun_as_dir1);
  //! returns current multisampled lighting from context
  void getLighting(const LightingContext *ctx, SHDirLighting &lt);

  //! changes parameters of sun directional light
  void changeSunLight(const Point3 &dir, const Color3 &col, float ang_sz);
  //! changes colored scale of skylight
  void changeAmbientScale(const Color3 &col_scale);


  int addLtData(SH3LightingData *lt_grid, unsigned id);
  int loadLtDataBinary(IGenLoad &crd, unsigned id);
  void delLtData(unsigned id);

  void delAllLtData();


  dag::Span<Element> getElems() const { return make_span(const_cast<SH3LightingManager *>(this)->elems); }

public:
  IRayTracer *tracer;
  static float min_power_of_light_use; // 0.004
  static float max_sun_dist;           // = 100.0;
  static float start_sun_fade_dist;    // amount of max_sun_dist, default 0.2;
protected:
  Tab<Element> elems;
};
DAG_DECLARE_RELOCATABLE(SH3LightingManager::Element);
