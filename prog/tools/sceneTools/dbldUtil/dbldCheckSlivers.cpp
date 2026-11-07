// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <libTools/collision/collResSliverCheck.h>
#include <scene/dag_loadLevelVer.h>
#include <ioSys/dag_genIo.h>
#include <util/dag_string.h>
#include <stdio.h>

// Lists the sliver faces of SCol and WCol. Returns 0 when there are none, 2 when the level needs a re-export, 13 on an error
// (not 1: a fatal error exits with 1).
int checkDbldCollSlivers(IGenLoad &crd)
{
  if (crd.readInt() != _MAKE4C('DBLD') || crd.readInt() != DBLD_Version)
  {
    printf("ERROR: invalid DBLD <%s>\n", crd.getTargetName());
    return 13;
  }
  crd.readInt(); // total tex number

  int bad = 0, streams = 0;
  for (;;)
  {
    int tag = crd.beginTaggedBlock();
    if (tag == _MAKE4C('END'))
    {
      crd.endBlock();
      break;
    }
    if (tag == _MAKE4C('SCol') || tag == _MAKE4C('WCol'))
    {
      streams++;
      eastl::unique_ptr<CollisionResource> coll =
        load_collres_checked(crd, String(0, "%s %c%c%c%c", crd.getTargetName(), _DUMP4C(tag)));
      if (!coll)
        return 13;
      printf("%c%c%c%c: %d mesh nodes\n", _DUMP4C(tag), (int)coll->meshNodes().size());
      CollResSliverStats st;
      check_collres_slivers(*coll, st);
      st.print();
      bad += st.bad + st.convexBad; // a level builds a convex node's faces as Jolt triangles, not a hull
    }
    crd.endBlock();
  }
  if (!streams)
    printf("  no SCol or WCol\n");
  printf("%s: %s\n", crd.getTargetName(), bad ? "RE-EXPORT (sliver collision faces)" : "ok");
  return bad ? 2 : 0;
}
