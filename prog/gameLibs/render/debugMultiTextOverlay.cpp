#include <render/debugMultiTextOverlay.h>
#include <hudprim/dag_hudPrimitives.h>
#include <math/integer/dag_IBBox2.h>
#include <util/dag_fastNameMapTS.h>
#include <memory/dag_framemem.h>
#include <dag/dag_vector.h>
#include <EASTL/string.h>
#include <EASTL/sort.h>

struct Prim
{
  int nameHash;
  float depth;
  Point2 spos;
  IBBox2 box;
  eastl::string_view name;
};

struct Counter
{
  int primIdx;
  int count;
};

struct Group
{
  IBBox2 box;
  dag::Vector<int, framemem_allocator> prims;
  dag::Vector<Counter, framemem_allocator> counters;
};

void prepare_and_sort(dag::ConstSpan<Point3> positions, dag::ConstSpan<eastl::string_view> names, const TMatrix4 &globtm,
  HudPrimitives *imm_prim, const DebugMultiTextOverlay &cfg, dag::Vector<Prim, framemem_allocator> &prims)
{
  const IBBox2 screenBox = {{0, 0}, {imm_prim->screenWidth, imm_prim->screenHeight}};
  const IPoint2 halfScreenRes = {imm_prim->screenWidth / 2, imm_prim->screenHeight / 2};
  const float zoomOffset = (float)imm_prim->screenHeight * cfg.zoomScreenOffset;
  const float zoomRcp = 1.f / cfg.zoomDistance;
  const Point2 zoomDir = normalize(Point2(1.f, -1.f));

  FastNameMapTS<false> nameMap;

  for (int i = 0; i < positions.size(); ++i)
  {
    Point4 spos = Point4::xyz1(positions[i]) * globtm;
    if (spos.w <= 0.f)
      continue;

    spos.x /= spos.w;
    spos.y /= spos.w;
    Point2 fpos = {(halfScreenRes.x * (spos.x + 1.f)), (halfScreenRes.y * (1.f - spos.y))};

    float zoom = 1.f - min(spos.w * zoomRcp, 1.f);
    Point2 zpos = zoomDir * zoom * zoomOffset;

    IPoint2 ipos = ipoint2(fpos) + ipoint2(zpos);
    IPoint2 isize = ipoint2(imm_prim->getColoredTextBBox(names[i].data(), 0));
    ipos.y -= isize.y;
    IBBox2 box = {ipos, ipos + isize};
    if (box & screenBox)
      prims.push_back({nameMap.addNameId(names[i].data()), spos.w, Point2::xy(fpos), box, names[i]});
  }

  eastl::sort(prims.begin(), prims.end(), [](const Prim &v1, const Prim &v2) { return v1.depth < v2.depth; });
}

void recalc_counters(Group &group, dag::ConstSpan<Prim> prims)
{
  group.counters.clear();
  for (int i : group.prims)
  {
    bool found = false;
    for (Counter &counter : group.counters)
    {
      if (prims[i].nameHash == prims[counter.primIdx].nameHash)
      {
        counter.count++;
        found = true;
        break;
      }
    }

    if (!found)
      group.counters.push_back({i, 1});
  }
}

eastl::string_view get_counter_template(const eastl::string_view &name, eastl::string &tmp, int count)
{
  if (count == 1)
  {
    return name;
  }
  else
  {
    tmp.append_sprintf("%s | x%d", name.data(), count);
    return tmp;
  }
}

void recalc_group_box(Group &group, dag::ConstSpan<Prim> prims, HudPrimitives *imm_prim, int border_padding)
{
  recalc_counters(group, prims);
  G_ASSERT_RETURN(!group.counters.empty(), );

  const IPoint2 borderPadding = {border_padding, border_padding};

  group.box[0] = prims[group.counters[0].primIdx].box[0] - borderPadding;
  group.box[1] = group.box[0];

  for (Counter &c : group.counters)
  {
    eastl::string tmp;
    eastl::string_view name = get_counter_template(prims[c.primIdx].name, tmp, c.count);

    IPoint2 isize = ipoint2(imm_prim->getColoredTextBBox(name.data(), 0));
    group.box[1].x = max(group.box[1].x, group.box[0].x + isize.x);
    group.box[1].y += isize.y;
  }

  group.box[1] += borderPadding * 2;
}

bool merge_step(dag::ConstSpan<Prim> prims, dag::Vector<Group, framemem_allocator> &groups, HudPrimitives *imm_prim,
  int max_group_size, int border_padding)
{
  bool done = true;
  for (int i = 0; i < groups.size(); ++i)
  {
    Group &targetGroup = groups[i];
    for (int j = i + 1; j < groups.size(); ++j)
    {
      Group &donorGroup = groups[j];
      if (!(targetGroup.box & donorGroup.box))
        continue;

      bool found = false;
      for (auto p = donorGroup.prims.begin(); p != donorGroup.prims.end(); ++p)
      {
        if (targetGroup.counters.size() >= max_group_size)
          break;

        int primIdx = *p;
        const Prim &prim = prims[primIdx];
        if (!(targetGroup.box & prim.box))
          continue;

        targetGroup.prims.push_back(primIdx);
        donorGroup.prims.erase_unsorted(p);
        --p;
        done = false;
        found = true;
      }

      if (found)
      {
        if (donorGroup.prims.empty())
        {
          groups.erase(groups.begin() + j); // need stable
          --j;
        }
        else
          recalc_group_box(donorGroup, prims, imm_prim, border_padding);
        recalc_group_box(targetGroup, prims, imm_prim, border_padding);
      }
    }
  }

  return done;
}

void render(dag::ConstSpan<Prim> prims, dag::ConstSpan<Group> groups, HudPrimitives *imm_prim, const DebugMultiTextOverlay &cfg)
{
  imm_prim->beginRenderImm();
  // lines should not obstruct text blocks
  for (const Group &group : groups)
  {
    int offset = 0;
    for (const Counter &counter : group.counters)
    {
      int heigth = prims[counter.primIdx].box.width().y;
      for (int p : group.prims)
      {
        if (prims[counter.primIdx].nameHash != prims[p].nameHash)
          continue;
        const Prim &prim = prims[p];
        imm_prim->renderLine(cfg.lineColor, Point2::xy(prim.spos),
          Point2(group.box[0].x, group.box[0].y + cfg.borderPadding + offset + heigth / 2), 1.f);
      }
      offset += heigth;
    }
  }

  for (const Group &group : groups)
  {
    int offset = 0;
    IPoint2 corners[] = {group.box.leftTop(), group.box.rightTop(), group.box.rightBottom(), group.box.leftBottom()};
    for (int i = 0; i < 4; ++i)
      imm_prim->renderLine(cfg.borderColor, corners[i], corners[(i + 1) % 4], 1.f);
    imm_prim->renderQuad(HudTexElem(), cfg.bkgColor, corners[0], corners[1], corners[2], corners[3]);

    offset = 0;
    for (const Counter &counter : group.counters)
    {
      const Prim &prim = prims[counter.primIdx];
      eastl::string tmp;
      eastl::string_view name = get_counter_template(prim.name, tmp, counter.count);
      offset += prim.box.width().y;
      imm_prim->renderText(group.box[0].x + cfg.borderPadding, group.box[0].y + cfg.borderPadding + offset, 0, cfg.textColor,
        name.data());
    }
  }
  imm_prim->endRenderImm();
}

void draw_debug_multitext_overlay(dag::ConstSpan<Point3> positions, dag::ConstSpan<eastl::string_view> names, HudPrimitives *imm_prim,
  const TMatrix4 &globtm, const DebugMultiTextOverlay &cfg)
{
  G_ASSERT_RETURN(positions.size() == names.size(), );

  dag::Vector<Prim, framemem_allocator> prims;
  prepare_and_sort(positions, names, globtm, imm_prim, cfg, prims);

  dag::Vector<Group, framemem_allocator> groups;
  for (int i = 0; i < prims.size(); ++i)
  {
    Group &group = groups.push_back({prims[i].box});
    group.prims.push_back(i);
    recalc_group_box(group, prims, imm_prim, cfg.borderPadding);
  }

  bool done = false;
  for (int i = 0; i < cfg.maxMergeSteps && !done; ++i)
    done = merge_step(prims, groups, imm_prim, cfg.maxGroupSize, cfg.borderPadding);

  render(prims, groups, imm_prim, cfg);
}