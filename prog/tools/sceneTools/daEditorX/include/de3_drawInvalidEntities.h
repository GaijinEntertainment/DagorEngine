//
// DaEditorX
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <de3_entityFilter.h>
#include <de3_objEntity.h>
#include <generic/dag_span.h>
#include <shaders/dag_shaderMesh.h>
#include <debug/dag_debug3d.h>

template <typename V, typename T = typename V::value_type>
void render_invalid_entities(const V &ent, int subtype_mask)
{
  if (!IObjEntityFilter::getShowInvalidAsset())
    return;

  ::begin_draw_cached_debug_lines();
  ::set_cached_debug_lines_wtm(TMatrix::IDENT);

  E3DCOLOR c(255, 255, 255, 255);
  TMatrix tm;

  for (int j = 0; j < ent.size(); j++)
    if (ent[j] && ent[j]->checkSubtypeMask(subtype_mask))
    {
      ent[j]->getTm(tm);
      ::draw_cached_debug_box(tm.getcol(3) - (tm.getcol(0) + tm.getcol(1) + tm.getcol(2)) / 2, tm.getcol(0), tm.getcol(1),
        tm.getcol(2), c);
    }

  ::end_draw_cached_debug_lines();
}
