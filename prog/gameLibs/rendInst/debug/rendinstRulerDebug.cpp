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

  if (desc.isValid() && desc.isRiExtra())
  {
    AutoLockReadPrimaryAndExtra lock;
    TMatrix tm = rendinst::getRIGenMatrixNoLock(desc);
    const char *name = rendinst::getRIGenResName(desc);
    G_UNUSED(tm);

    RendInstGenData *rgl = RendInstGenData::getGenDataByLayer(desc);
    int riPoolRef = rendinst::riExtra[desc.pool].riPoolRef;

    char buf[1024];
    snprintf(buf, sizeof(buf), "%s: %s", desc.isRiExtra() ? "RiExtra" : "RiGen", name);
    buf[sizeof(buf) - 1] = '\0';
    float markOffsetScale = dist * 0.22f;
    Point3 markPos = intersection_pos + cam_tm.getcol(0) * markOffsetScale + cam_tm.getcol(1) * markOffsetScale * 0.75f;
    add_debug_text_mark(markPos, buf);

    snprintf(buf, sizeof(buf), "riExtra.initialHP: %.1f", rendinst::riExtra[desc.pool].initialHP);
    buf[sizeof(buf) - 1] = '\0';
    add_debug_text_mark(markPos, buf, -1, 1.2f);

    snprintf(buf, sizeof(buf), "riExtra.immortal: %i", rendinst::riExtra[desc.pool].immortal);
    buf[sizeof(buf) - 1] = '\0';
    add_debug_text_mark(markPos, buf, -1, 2.4f);

    snprintf(buf, sizeof(buf), "riProperties.immortal: %i", rgl->rtData->riProperties[riPoolRef].immortal);
    buf[sizeof(buf) - 1] = '\0';
    add_debug_text_mark(markPos, buf, -1, 3.6f);
  }

  end_draw_cached_debug_lines();
}
