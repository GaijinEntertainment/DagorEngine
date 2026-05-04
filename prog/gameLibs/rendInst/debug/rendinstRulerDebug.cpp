// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <rendInst/rendInstDebug.h>
#include <rendInst/rendInstAccess.h>
#include <rendInst/rendInstCollision.h>
#include "../riGen/riGenData.h"
#include "../riGen/riGenExtra.h"
#include <debug/dag_debug3d.h>
#include <debug/dag_textMarks.h>
#include <cstdio>

void rendinst::draw_rendinst_info(const Point3 &intersection_pos, const TMatrix &cam_tm, const rendinst::RendInstDesc &desc,
  int mat_id)
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
    const RendInstGenData *rgl = RendInstGenData::getGenDataByLayer(desc);
    rendinst::RiExtraPool &rxPool = rendinst::riExtra[desc.pool];
    int riPoolRef = desc.isRiExtra() ? rxPool.riPoolRef : desc.pool;
    if (rgl == nullptr || rgl->rtData == nullptr)
      return;
    const RendInstGenData::RendinstProperties &riProp = rgl->rtData->riProperties[riPoolRef];

    draw_cached_matrix_axis(tm);

    BBox3 fullBb = rendinst::getRIGenFullBBox(desc);
    draw_cached_debug_box(fullBb, E3DCOLOR(0xFF00C0FF), tm);

    BBox3 colBb = rendinst::getRIGenBBox(desc);
    draw_cached_debug_box(colBb, E3DCOLOR(0xFFFFC000), tm);

    // canopy bbox
    if (!desc.isRiExtra() && (uint32_t)desc.pool < rgl->rtData->riProperties.size())
    {
      BBox3 canopyBox;
      rendinst::getRIGenCanopyBBox(riProp, fullBb, canopyBox);
      draw_cached_debug_box(canopyBox, E3DCOLOR(0xFF08FF08), tm);
    }

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
    addLine("Cell=%i Pool=i Idx=%i Offs=%i Layer=%i", desc.cellIdx, desc.pool, desc.idx, desc.offs, desc.layer);
    if (desc.isRiExtra())
    {
      addLine("riExtra.handle = %llx", desc.getRiExtraHandle());
      addLine(" collRes: %s", rxPool.collRes ? "yes" : "no");
      addLine(" bsphXYZR: (%.1f %.1f %.1f) r=%.1f", V4D(rxPool.bsphXYZR));
      addLine(" riPoolRef: %i", rxPool.riPoolRef);
      addLine(" posInst: %i; isTree %i; isWalls: %i", rxPool.posInst, rxPool.isTree, rxPool.isWalls);
      addLine(" hp: %.1f/%.1f; immortal: %i", rxPool.getHp(desc.idx), rxPool.initialHP, rxPool.immortal);
    }
    addLine("riProperties[%i]", riPoolRef);
    const PhysMat::MaterialData &mat = PhysMat::getMaterial(mat_id);
    const PhysMat::MaterialData &riPropMat = PhysMat::getMaterial(riProp.matId);
    addLine(" trace matId: %i (%s)", mat_id, mat.name.c_str());
    addLine(" rendinst_dmg matId: %i (%s)", riProp.matId, riPropMat.name.c_str());
    addLine(" overrideMaterialForTraces: %i", riProp.overrideMaterialForTraces);
    addLine(" immortal: %i", riProp.immortal);
    addLine(" treeBehaviour: %i; bushBehaviour %i", riProp.treeBehaviour, riProp.bushBehaviour);
    addLine("tm.scale: %.1f %.1f %.1f", length(tm.getcol(0)), length(tm.getcol(1)), length(tm.getcol(2)));
    addLine("tm.pos: %.1f %.1f %.1f", P3D(tm.getcol(3)));
  }

  end_draw_cached_debug_lines();
}

void rendinst::draw_rendinst_canopy(const Point3 &intersection_pos, const TMatrix &cam_tm, const RendInstDesc &desc)
{
  begin_draw_cached_debug_lines();
  if (desc.isValid())
  {
    AutoLockReadPrimaryAndExtra lock;
    TMatrix tm = rendinst::getRIGenMatrixNoLock(desc);
    BBox3 canopyBBox;
    getRIGenCanopyBBox(desc, tm, canopyBBox, false);
    draw_cached_debug_box(canopyBBox, E3DCOLOR(0xFF08FF08));

    Point3 dir = normalize(intersection_pos - cam_tm.getcol(3));
    float traceDist = 4000.f;
    rendinst::traceTransparencyRayRIGenNormalizedWithDist(cam_tm.getcol(3), dir, traceDist, 0.01);
    float hitRad = pow(traceDist / 300.0f, 0.8f);
    draw_cached_debug_sphere(cam_tm.getcol(3) + dir * traceDist, hitRad, E3DCOLOR(0xFFFFFF00), 24);

    float dist = length(cam_tm.getcol(3) - intersection_pos);
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

    const char *name = rendinst::getRIGenResName(desc);
    addLine("ri: %s", name);
  }
  end_draw_cached_debug_lines();
}
