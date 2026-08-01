// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <assets/asset.h>
#include <assets/assetMgr.h>
#include <assets/assetExporter.h>
#include <libTools/util/makeBindump.h>
#include <libTools/util/iLogWriter.h>
#include <math/dag_geomTree.h>

static GeomNodeTreeUniquePtr getSkeleton(DagorAssetMgr &m, const char *gn_name, ILogWriter &log)
{
  static DagorAssetMgr *mgr = NULL;
  static int skeleton_atype = -2;

  if (&m != mgr)
  {
    mgr = &m;
    skeleton_atype = m.getAssetTypeId("skeleton");
  }

  if (!gn_name)
  {
    log.addMessage(ILogWriter::ERROR, "skeleton asset not specified");
    return {};
  }
  DagorAsset *gn_a = m.findAsset(gn_name, skeleton_atype);
  if (!gn_a)
  {
    log.addMessage(ILogWriter::ERROR, "can't get skeleton asset <%s>", gn_name);
    return {};
  }

  IDagorAssetExporter *e = m.getAssetExporter(skeleton_atype);
  if (!e)
  {
    log.addMessage(ILogWriter::ERROR, "can't get skeleton exporter plugin");
    return {};
  }

  mkbindump::BinDumpSaveCB cwr(16 << 10, _MAKE4C('PC'), false);
  if (!e->exportAsset(*gn_a, cwr, log))
  {
    log.addMessage(ILogWriter::ERROR, "can't build skeleton asset <%s> data", gn_name);
    return {};
  }

  MemoryLoadCB crd(cwr.getMem(), false);
  return GeomNodeTreeUniquePtr(GeomNodeTree::load(crd));
}
