#include <gamePhys/collision/collisionLinks.h>
#include <ioSys/dag_dataBlock.h>
#include <util/dag_oaHashNameMap.h>
#include <math/dag_mathAng.h>

namespace dacoll
{

static FastNameMap collision_names;

void load_collision_links(CollisionLinks &links, const DataBlock &blk, float scale)
{
  CollisionLinkData &link = links.emplace_back();
  link.axis = blk.getPoint3("axis", Point3(0.f, 1.f, 0.f));
  link.offset = blk.getPoint3("offset", Point3(0.f, 0.f, 0.f)) * scale;
  link.capsuleRadiusScale = blk.getReal("capsuleRadiusScale", 1.0f);
  link.paramMult = blk.getReal("paramMult", 1.f);
  link.oriParamMult = blk.getReal("oriParamMult", 0.f);
  link.haveCollision = blk.getBool("haveCollision", false);
  link.nameId = collision_names.addNameId(blk.getBlockName());

  for (int i = 0, nb = blk.blockCount(); i < nb; ++i)
    load_collision_links(links, *blk.getBlock(i), scale);
}

static TMatrix generate_collisions(const TMatrix &tm, float param, const Point2 &ori_param, const CollisionLinkData &link,
  tmp_collisions_t &out_collisions)
{
  TMatrix curTm = tm;

  curTm = curTm * makeTM(link.axis, link.paramMult * param);
  if (link.oriParamMult != 0.f)
  {
    curTm = curTm * makeTM(Point3(0.f, 1.f, 0.f), safe_atan2(ori_param.y, ori_param.x) * length(ori_param));
    curTm = curTm * makeTM(Point3(0.f, 0.f, -1.f), length(ori_param) * link.oriParamMult);
  }
  curTm.setcol(3, curTm * link.offset);

  {
    Point3 from = tm.getcol(3);
    Point3 to = curTm.getcol(3);
    Point3 dir = to - from;
    float len = length(dir);
    dir *= safeinv(len);

    CollisionCapsuleProperties &colProps = out_collisions.push_back();
    colProps.tm = curTm;
    colProps.tm.setcol(1, dir);
    colProps.nameId = link.nameId;
    colProps.haveCollision = link.haveCollision;
    Point3 left = colProps.tm.getcol(0) % colProps.tm.getcol(1);
    if (lengthSq(left) < 1e-5f)
    {
      colProps.tm.setcol(0, normalize(colProps.tm.getcol(1) % colProps.tm.getcol(2)));
      colProps.tm.setcol(2, normalize(colProps.tm.getcol(0) % colProps.tm.getcol(1)));
    }
    else
    {
      colProps.tm.setcol(2, normalize(left));
      colProps.tm.setcol(0, normalize(colProps.tm.getcol(1) % colProps.tm.getcol(2)));
    }

    colProps.tm.setcol(3, (from + to) * 0.5f);
    colProps.scale.set(link.capsuleRadiusScale, len / link.capsuleRadiusScale, 1.f);
  }

  return curTm;
}

void generate_collisions(const TMatrix &tm, float param, const Point2 &ori_param, const CollisionLinks &links,
  tmp_collisions_t &out_collisions)
{
  TMatrix curTm = tm;
  for (auto &link : links)
    curTm = generate_collisions(curTm, param, ori_param, link, out_collisions);
}

void lerp_collisions(tmp_collisions_t &a_collisions, const tmp_collisions_t &b_collisions, float factor)
{
  for (int i = 0; i < a_collisions.size(); ++i)
  {
    CollisionCapsuleProperties &acol = a_collisions[i];
    for (int j = 0; j < b_collisions.size(); ++j)
    {
      const CollisionCapsuleProperties &bcol = b_collisions[j];
      if (bcol.nameId != acol.nameId)
        continue;

      acol.scale = lerp(bcol.scale, acol.scale, factor);
      // first quat, next pos
      Quat prevQ(bcol.tm);
      Quat curQ(acol.tm);
      Quat resQ = qinterp(prevQ, curQ, factor);
      Point3 interpPos = lerp(bcol.tm.getcol(3), acol.tm.getcol(3), factor);

      acol.tm = makeTM(resQ);
      acol.tm.setcol(3, interpPos);
      break;
    }
  }
}

int get_link_name_id(const char *name) { return collision_names.getNameId(name); }

} // namespace dacoll
