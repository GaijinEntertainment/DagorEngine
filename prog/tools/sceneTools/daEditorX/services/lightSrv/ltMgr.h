#pragma once

namespace mkbindump
{
class BinDumpSaveCB;
}
class SH3LightingManager;
class ISceneLightService;
class StaticGeometryContainer;


class LtMgrForLightService
{
public:
  LtMgrForLightService(ISceneLightService &s) : mgr(NULL), srv(s) {}
  ~LtMgrForLightService();

  void exportShlt(mkbindump::BinDumpSaveCB &cwr, StaticGeometryContainer &omni_lt_geom);

  void save(const char *fname, StaticGeometryContainer &omni_lt_geom);
  bool load(const char *fname);

public:
  class StaticRayHit;
  SH3LightingManager *mgr;
  ISceneLightService &srv;
};
