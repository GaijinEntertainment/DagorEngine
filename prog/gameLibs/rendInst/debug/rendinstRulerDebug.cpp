// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <rendInst/rendInstDebug.h>
#include <rendInst/rendInstAccess.h>
#include "../riGen/riGenData.h"
#include "../riGen/riGenExtra.h"
#include <debug/dag_debug3d.h>
#include <debug/dag_textMarks.h>
#include <cstdio>

void rendinst::draw_rendinst_info(const Point3 &intersection_pos, const TMatrix &cam_tm, const rendinst::RendInstDesc &desc)
{
  begin_draw_cached_debug_lines();

  float dist = length(cam_tm.getcol(3) - intersection_pos);
  draw_cached_debug_line(cam_tm.getcol(3) - Point3(0.0, 0.05, -0.075), intersection_pos, E3DCOLOR(0xFFFF2020));
  float hitRad = pow(dist / 300.0f, 0.8f);
  draw_cached_debug_sphere(intersection_pos, hitRad, E3DCOLOR(0xFFFFFF00), 24);

  // draw_cached_debug_sphere(collres.boundingSphereCenter, collres.boundingSphereRad, E3DCOLOR(0xFFBBBB00), 24);

  if (desc.isValid())
  {
    AutoLockReadPrimaryAndExtra lock;
    TMatrix tm = rendinst::getRIGenMatrixNoLock(desc);
    const char *name = rendinst::getRIGenResName(desc);

    float markOffsetScale = dist * 0.22f;
    Point3 markPos = intersection_pos + cam_tm.getcol(0) * markOffsetScale + cam_tm.getcol(1) * markOffsetScale * 0.75f;
    float curLine = 0.f;
    auto addLine = [&curLine, &markPos](const char *s, ...) DAGOR_NOINLINE {
      va_list arguments;
      va_start(arguments, s);
      char buf[1024];
      vsnprintf(buf, sizeof(buf), s, arguments);
      va_end(arguments);
      add_debug_text_mark(markPos, buf, -1, curLine);
      curLine += 1.2f;
    };

    addLine("%s: %s", desc.isRiExtra() ? "RiExtra" : "RiGen", name);
    if (desc.isRiExtra())
    {
      addLine("Cell=%i Pool=i Idx=%i Offs=%i Layer=%i", desc.cellIdx, desc.pool, desc.idx, desc.offs, desc.layer);
      addLine("riExtra.handle = %llx", desc.getRiExtraHandle());
      addLine(" collRes: %s", rendinst::riExtra[desc.pool].collRes ? "yes" : "no");
      addLine(" bsphXYZR: (%.1f %.1f %.1f) r=%.1f", V4D(rendinst::riExtra[desc.pool].bsphXYZR));
      addLine(" riPoolRef: %i", rendinst::riExtra[desc.pool].riPoolRef);
      addLine(" posInst: %i; isWalls: %i", rendinst::riExtra[desc.pool].posInst, rendinst::riExtra[desc.pool].isWalls);
      addLine(" hp: %.1f/%.1f; immortal: %i", rendinst::riExtra[desc.pool].getHp(desc.idx), rendinst::riExtra[desc.pool].initialHP,
        rendinst::riExtra[desc.pool].immortal);
    }
    RendInstGenData *rgl = RendInstGenData::getGenDataByLayer(desc);
    int riPoolRef = desc.isRiExtra() ? rendinst::riExtra[desc.pool].riPoolRef : desc.pool;
    addLine("riProperties[%i]", riPoolRef);
    addLine(" matId: %i", rgl->rtData->riProperties[riPoolRef].matId);
    addLine(" immortal: %i", rgl->rtData->riProperties[riPoolRef].immortal);
    addLine("tm.scale: %.1f %.1f %.1f", length(tm.getcol(0)), length(tm.getcol(1)), length(tm.getcol(2)));
    addLine("tm.pos: %.1f %.1f %.1f", P3D(tm.getcol(3)));
  }

  end_draw_cached_debug_lines();
}
