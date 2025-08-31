// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <assets/daBuildExpPluginChain.h>
#include <assets/assetExporter.h>
#include <assets/assetRefs.h>
#include <assets/assetPlugin.h>
#include <assets/asset.h>
#include <assets/assetMgr.h>
#include <libTools/util/makeBindump.h>
#include <libTools/util/iLogWriter.h>
#include <libTools/dagFileRW/dagFileNode.h>
#include <libTools/util/strUtil.h>
#include <ioSys/dag_memIo.h>
#include <ioSys/dag_dataBlockUtils.h>
#include <gameRes/dag_stdGameRes.h>
#include "phSysObj.h"

BEGIN_DABUILD_PLUGIN_NAMESPACE(physObj)

static const char *TYPE = "physObj";

enum
{
  NODEFLG_SRV_BODY_CUR = 0x10000000,
  NODEFLG_SRV_BODY = 0x20000000
};

static const char *curDagFname = NULL;
static DataBlock defRagdollBlk;
static SimpleString appBlkFname;

class PhysObjExporter : public IDagorAssetExporter
{
public:
  const char *__stdcall getExporterIdStr() const override { return "physObj exp"; }

  const char *__stdcall getAssetType() const override { return TYPE; }
  unsigned __stdcall getGameResClassId() const override { return PhysObjGameResClassId; }
  unsigned __stdcall getGameResVersion() const override { return 8; }

  void __stdcall onRegister() override {}
  void __stdcall onUnregister() override {}

  void __stdcall gatherSrcDataFiles(const DagorAsset &a, Tab<SimpleString> &files) override
  {
    files.clear();
    files.push_back() = a.getTargetFilePath();
    if (a.props.getBool("ragdoll", false))
      files.push_back() = appBlkFname;
  }

  bool __stdcall isExportableAsset(DagorAsset &a) override { return true; }

  bool __stdcall exportAsset(DagorAsset &a, mkbindump::BinDumpSaveCB &cwr, ILogWriter &log) override
  {
    String fpath(a.getTargetFilePath());
    AScene scene;

    if (!load_ascene(fpath, scene, LASF_NOMATS, false))
    {
      log.addMessage(log.ERROR, "%s: can't read '%s'", a.getName(), fpath.str());
      return false;
    }
    scene.root->invalidate_wtm();
    scene.root->calc_wtm();

    curDagFname = fpath;
    Tab<PhBodyRec *> bodies(tmpmem);
    if (a.props.getBool("simplePhysObj", false))
    {
      PhBodyRec *b = buildPhysObj(a, scene.root, scene.root->find_inode("::mainBody"), log);
      if (!b)
        return false;
      bodies.push_back(b);
    }
    else
    {
      struct RagdollCB : public Node::NodeEnumCB
      {
        const DataBlock *aRagdoll;

        RagdollCB(const DataBlock *ard) : aRagdoll(ard) {}
        int node(Node &c) override
        {
          const DataBlock *nodeBlk = aRagdoll ? aRagdoll->getBlockByNameEx("name_full")->getBlockByName(c.name) : NULL;
          if (!nodeBlk && aRagdoll)
            if (const DataBlock *b = aRagdoll->getBlockByName("name_tail"))
              for (int i = 0, ie = b->blockCount(); i < ie; i++)
                if (::trail_stricmp(c.name, b->getBlock(i)->getBlockName()))
                {
                  nodeBlk = b->getBlock(i);
                  break;
                }

          if (!nodeBlk)
            nodeBlk = defRagdollBlk.getBlockByNameEx("name_full")->getBlockByName(c.name);
          if (!nodeBlk)
            if (const DataBlock *b = defRagdollBlk.getBlockByName("name_tail"))
              for (int i = 0, ie = b->blockCount(); i < ie; i++)
                if (::trail_stricmp(c.name, b->getBlock(i)->getBlockName()))
                {
                  nodeBlk = b->getBlock(i);
                  break;
                }

          if (!nodeBlk && aRagdoll)
            nodeBlk = aRagdoll->getBlockByName("def_helper");
          if (!nodeBlk)
            nodeBlk = defRagdollBlk.getBlockByName("def_helper");

          if (nodeBlk)
          {
            DataBlock blk;
            blk = *nodeBlk;
            if (!c.script.empty())
            {
              DataBlock nblk;
              dblk::load_text(nblk, make_span_const(c.script), dblk::ReadFlag::ROBUST, curDagFname);
              merge_data_block(blk, nblk);
            }
            blk.setBool("physObj", blk.getBool("physObj", true));
            if (blk.getBool("physObj", true))
              c.flags |= NODEFLG_SRV_BODY;
            blk.setBool("animated_node", blk.getBool("animated_node", true));
            blk.setReal("density", blk.getReal("density", defRagdollBlk.getReal("def_density", 950)));
            blk.setStr("massType", blk.getStr("massType", defRagdollBlk.getStr("def_massType", "none")));
            blk.setStr("collType", blk.getStr("collType", defRagdollBlk.getStr("def_collType", "none")));
            if (DataBlock *jb = blk.getBlockByName("joint"))
            {
              Node *parentNode = c.parent;
              const char *excl_nm = jb->getStr("exclParent", NULL);
              for (; parentNode; parentNode = parentNode->parent)
                if (excl_nm && ::trail_stricmp(parentNode->name, excl_nm))
                  continue;
                else if (parentNode->flags & NODEFLG_SRV_BODY)
                  break;
              jb->removeParam("exclParent");

              if (parentNode)
              {
                jb->setStr("body", jb->getStr("body", parentNode->name));
                jb->setTm("tm", c.wtm);
              }
              else
                blk.removeBlock("joint");
            }

            DynamicMemGeneralSaveCB mcwr(tmpmem);
            blk.saveToTextStream(mcwr);
            c.script = String(0, "%.*s", mcwr.size(), mcwr.data());
            // debug("%s:\n%s", c.name, c.script);
          }
          return 0;
        }
      };

      if (a.props.getBool("ragdoll", false))
      {
        RagdollCB cb(a.props.getBlockByName("ragdoll"));
        scene.root->enum_nodes(cb, NULL);
      }

      class CB : public Node::NodeEnumCB
      {
      public:
        CB(Tab<Node *> &t) : tab(t) {}
        int node(Node &c) override
        {
          if (c.script.empty())
            return 0;

          DataBlock blk;
          dblk::load_text(blk, make_span_const(c.script), dblk::ReadFlag::ROBUST, curDagFname);
          if (blk.getBool("physObj", false))
          {
            c.flags |= NODEFLG_SRV_BODY;
            tab.push_back(&c);
          }

          return 0;
        }

        Tab<Node *> &tab;
      };

      Tab<Node *> nodes(tmpmem);
      CB cb(nodes);

      scene.root->enum_nodes(cb, NULL);
      if (!nodes.size())
      {
        log.addMessage(log.ERROR, "%s: no nodes with physObj:b=yes in complex physobj asset", a.getName());
        return false;
      }

      bodies.reserve(nodes.size());
      for (int i = 0; i < nodes.size(); i++)
      {
        nodes[i]->flags |= NODEFLG_SRV_BODY_CUR;
        PhBodyRec *b = buildPhysObj(a, nodes[i], nodes[i], log);
        nodes[i]->flags &= ~NODEFLG_SRV_BODY_CUR;
        if (!b)
        {
          clear_all_ptr_items(bodies);
          return false;
        }
        bodies.push_back(b);
      }
    }
    curDagFname = NULL;

    exportPhysSys(bodies, cwr, !a.props.getBool("expAllHelpers", a.props.getBool("ragdoll", false)),
      a.props.getBool("ragdoll", false));
    clear_all_ptr_items(bodies);
    return true;
  }

protected:
  struct PhBodyRec
  {
    Tab<PhysSysObject *> objs;
    DataBlock bodyBlk;
    TMatrix wtm;

    PhBodyRec() : objs(tmpmem) {}
    ~PhBodyRec() { clear_all_ptr_items(objs); }
  };

  PhBodyRec *buildPhysObj(DagorAsset &a, Node *root, Node *bodyNode, ILogWriter &log)
  {
    class CB : public Node::NodeEnumCB
    {
    public:
      CB(Tab<PhysSysObject *> &t) : tab(t) {}
      int node(Node &c) override
      {
        if (!(c.flags & NODEFLG_RCVSHADOW))
          return 0;
        if (c.name && stricmp(c.name, "::mainBody") == 0)
          return 0;

        if (!checkBodyNode(&c))
          return 0;
        if (!c.obj || !c.obj->isSubOf(OCID_MESHHOLDER))
          return 0;

        MeshHolderObj &mh = *(MeshHolderObj *)c.obj;
        if (!mh.mesh)
          return 0;

        if (c.script.empty())
          return 0;

        DataBlock blk;
        dblk::load_text(blk, make_span_const(c.script), dblk::ReadFlag::ROBUST, curDagFname);

        if (!blk.getBool("add_to_phys", true))
          return 0;

        PhysSysObject *po = new (tmpmem) PhysSysObject(blk.getBool("animated_node", false) ? c.name.str() : NULL);
        if (!po->load(*mh.mesh, c.wtm, blk))
        {
          delete po;
          return -1;
        }
        tab.push_back(po);

        return 0;
      }

      bool checkBodyNode(Node *c)
      {
        while (c)
        {
          if (c->flags & NODEFLG_SRV_BODY)
            return (c->flags & NODEFLG_SRV_BODY_CUR) ? true : false;
          c = c->parent;
        }
        return true;
      }

      Tab<PhysSysObject *> &tab;
    };

    PhBodyRec *rec = new PhBodyRec;
    CB cb(rec->objs);
    Node *n = NULL;
    root->enum_nodes(cb, &n);

    if (n)
    {
      log.addMessage(log.ERROR, "%s: bad settings in node <%s>", a.getName(), n->name.str());
      delete rec;
      return NULL;
    }

    if (bodyNode)
      if (!rec->bodyBlk.loadText(bodyNode->script, (int)strlen(bodyNode->script), curDagFname))
      {
        log.addMessage(log.ERROR, "%s: bad script in node <::mainBody>", a.getName());
        delete rec;
        return NULL;
      }
    if (!rec->bodyBlk.getStr("phmat", NULL) && a.props.getStr("phmat", NULL))
      rec->bodyBlk.setStr("phmat", a.props.getStr("phmat", NULL));
    if (!rec->bodyBlk.getStr("name", NULL) && bodyNode && !bodyNode->name.empty())
      rec->bodyBlk.setStr("name", bodyNode->name);
    rec->wtm = bodyNode ? bodyNode->wtm : TMatrix::IDENT;

    return rec;
  }

  void exportPhysSys(dag::ConstSpan<PhBodyRec *> po, mkbindump::BinDumpSaveCB &cwr, bool min_hlp, bool ragdoll)
  {
    cwr.writeFourCC(_MAKE4C('po1s'));
    cwr.beginBlock();
    for (int i = 0; i < po.size(); i++)
      exportPhysObj(cwr, po[i]->objs, po[i]->bodyBlk, po[i]->wtm, min_hlp);

    for (int i = 0; i < po.size(); i++)
      if (const DataBlock *jb = po[i]->bodyBlk.getBlockByName("joint"))
      {
        const char *dest_nm = jb->getStr("body", "?");
        for (int j = 0; j < po.size(); j++)
          if (i != j && strcmp(po[j]->bodyBlk.getStr("name", ""), dest_nm) == 0)
          {
            exportPhysJoint(cwr, *jb, i, j);
            dest_nm = NULL;
            break;
          }
        if (dest_nm)
          logerr("failed to find <%s> for joint in <%s>", dest_nm, po[i]->bodyBlk.getStr("name", ""));
      }

    if (ragdoll) // write align controllers for ragdoll
      for (int i = 0, nid = defRagdollBlk.getNameId("twist_ctrl"); i < defRagdollBlk.blockCount(); i++)
        if (defRagdollBlk.getBlock(i)->getBlockNameId() == nid)
        {
          const DataBlock &b = *defRagdollBlk.getBlock(i);
          cwr.beginBlock();
          cwr.writeFourCC(_MAKE4C('Nt1c'));
          cwr.writeDwString(b.getStr("node0", ""));
          cwr.writeDwString(b.getStr("node1", ""));
          cwr.writeFloat32e(b.getReal("angDiff", 0) * DEG_TO_RAD);
          for (int j = 0, nid = defRagdollBlk.getNameId("twistNode"); j < b.paramCount(); j++)
            if (b.getParamNameId(j) == nid && b.getParamType(j) == defRagdollBlk.TYPE_STRING)
              cwr.writeDwString(b.getStr(j));
          cwr.endBlock();
        }
    cwr.endBlock();
  }

  void exportPhysObj(mkbindump::BinDumpSaveCB &cwr, dag::ConstSpan<PhysSysObject *> objs, const DataBlock &bodyBlk,
    const TMatrix &nodeWtm, bool min_hlp)
  {
    PhysSysBodyObject body;
    body.name = bodyBlk.getStr("name", NULL);
    body.ref = bodyBlk.getStr("ref", NULL);
    body.materialName = bodyBlk.getStr("phmat", NULL);
    body.bodyMass = bodyBlk.getReal("mass", 0);
    body.bodyMomj = bodyBlk.getPoint3("momj", Point3(0, 0, 0));
    body.momjInIdentMatrix = bodyBlk.getBool("momjInIdentMatrix", false);
    body.forceCmPosX0 = bodyBlk.getBool("forceCmPosX0", false);
    body.forceCmPosY0 = bodyBlk.getBool("forceCmPosY0", false);
    body.forceCmPosZ0 = bodyBlk.getBool("forceCmPosZ0", false);
    body.calcMomjAndTm(objs);

    Tab<PhysSysObject *> boxColl(tmpmem), sphColl(tmpmem), capColl(tmpmem);

    // process body's objects
    for (int oi = 0; oi < objs.size(); ++oi)
    {
      if (objs[oi]->collType == PhysSysObject::COLL_BOX)
        boxColl.push_back(objs[oi]);
      else if (objs[oi]->collType == PhysSysObject::COLL_SPHERE)
        sphColl.push_back(objs[oi]);
      else if (objs[oi]->collType == PhysSysObject::COLL_CAPSULE)
        capColl.push_back(objs[oi]);
    }

    TMatrix bodyTm = body.matrix;
    real bodyMass = body.bodyMass;
    Point3 bodyMomj = body.bodyMomj;

    TMatrix invBodyTm = inverse(bodyTm);

    // write body data
    cwr.beginBlock();
    cwr.writeFourCC(_MAKE4C('body'));

    cwr.writeDwString(body.name);

    cwr.write32ex(&bodyTm, sizeof(bodyTm));
    cwr.writeReal(bodyMass);
    cwr.write32ex(&bodyMomj, sizeof(bodyMomj));

    cwr.beginBlock();
    cwr.writeFourCC(_MAKE4C('Alia'));
    cwr.writeDwString(body.ref);
    cwr.endBlock();

    cwr.beginBlock();
    cwr.writeFourCC(_MAKE4C('Mat '));
    cwr.writeDwString(body.materialName);
    cwr.endBlock();

    for (int oi = 0; oi < objs.size(); ++oi)
    {
      if (objs[oi]->helperName.empty())
        continue;
      TMatrix tm = invBodyTm * objs[oi]->matrix;

      tm.setcol(0, normalize(tm.getcol(0)));
      tm.setcol(1, normalize(tm.getcol(1)));
      tm.setcol(2, normalize(tm.getcol(2)));

      cwr.beginBlock();
      cwr.writeFourCC(_MAKE4C('Htm '));
      cwr.writeDwString(objs[oi]->helperName);
      cwr.write32ex(&tm, sizeof(tm));
      cwr.endBlock();
      if (min_hlp)
        break;
    }

    // write box collision
    for (int i = 0; i < boxColl.size(); ++i)
    {
      PhysSysObject *obj = boxColl[i];

      BBox3 box;
      if (!obj->getBoxCollision(box))
        continue;

      TMatrix tm = invBodyTm * obj->matrix;
      tm.setcol(3, tm * box.center());

      Point3 size = box.width();
      size.x *= tm.getcol(0).length();
      size.y *= tm.getcol(1).length();
      size.z *= tm.getcol(2).length();

      tm.setcol(0, normalize(tm.getcol(0)));
      tm.setcol(1, normalize(tm.getcol(1)));
      tm.setcol(2, normalize(tm.getcol(2)));

      if (tm.det() < 0)
        tm.setcol(2, -tm.getcol(2));

      cwr.beginBlock();
      cwr.writeFourCC(_MAKE4C('Cbox'));

      cwr.write32ex(&tm, sizeof(tm));
      cwr.write32ex(&size, sizeof(size));
      cwr.writeDwString(obj->materialName);

      cwr.endBlock();
    }

    // write sphere collision
    for (int i = 0; i < sphColl.size(); ++i)
    {
      PhysSysObject *obj = sphColl[i];

      Point3 center;
      real radius;
      if (!obj->getSphereCollision(center, radius))
        continue;

      TMatrix tm = invBodyTm * obj->matrix;

      center = tm * center;
      radius *= (tm.getcol(0).length() + tm.getcol(1).length() + tm.getcol(2).length()) / 3;

      cwr.beginBlock();
      cwr.writeFourCC(_MAKE4C('Csph'));

      cwr.write32ex(&center, sizeof(center));
      cwr.writeReal(radius);
      cwr.writeDwString(obj->materialName);

      cwr.endBlock();
    }

    // write capsule collision
    for (int i = 0; i < capColl.size(); ++i)
    {
      PhysSysObject *obj = capColl[i];

      TMatrix tm = invBodyTm * obj->matrix;

      Point3 center, extent;
      real radius;
      bool ok =
        obj->getCapsuleCollision(center, extent, radius, Point3(tm.getcol(0).length(), tm.getcol(1).length(), tm.getcol(2).length()));

      if (!ok)
        continue;

      center = tm * center;

      Matrix3 ntm;
      ntm.setcol(0, normalize(tm.getcol(0)));
      ntm.setcol(1, normalize(tm.getcol(1)));
      ntm.setcol(2, normalize(tm.getcol(2)));

      extent = ntm * extent;

      cwr.beginBlock();
      cwr.writeFourCC(_MAKE4C('Ccap'));

      cwr.write32ex(&center, sizeof(center));
      cwr.write32ex(&extent, sizeof(extent));
      cwr.writeReal(radius);
      cwr.writeDwString(obj->materialName);

      cwr.endBlock();
    }

    cwr.endBlock();
  }

  void exportPhysJoint(mkbindump::BinDumpSaveCB &cwr, const DataBlock &jBlk, int body1Id, int body2Id)
  {
    const char *jtype = jBlk.getStr("type", "");
    TMatrix tm = jBlk.getTm("tm", TMatrix::IDENT);

    if (strcmp(jtype, "rag_ball") == 0)
    {
      Point3 minLim = jBlk.getPoint3("minLimit", Point3(0, 0, 0)) * DEG_TO_RAD;
      Point3 maxLim = jBlk.getPoint3("maxLimit", Point3(0, 0, 0)) * DEG_TO_RAD;
      bool swap_z = tm.det() < 0;
      if (swap_z)
        tm.setcol(2, -tm.getcol(2));
      if (jBlk.getBool("swapLimZ", false))
        swap_z = !swap_z;
      if (swap_z)
      {
        float tmp = minLim.z;
        minLim.z = maxLim.z;
        maxLim.z = tmp;
      }

      cwr.beginBlock();
      cwr.writeFourCC(_MAKE4C('RJba'));

      cwr.writeInt32e(body1Id);
      cwr.writeInt32e(body2Id);

      cwr.write32ex(&tm, sizeof(tm));
      cwr.write32ex(&minLim, sizeof(minLim));
      cwr.write32ex(&maxLim, sizeof(maxLim));

      cwr.writeReal(jBlk.getReal("damping", 0));
      cwr.writeReal(jBlk.getReal("twistDamping", 0));
      cwr.writeReal(jBlk.getReal("stiffness", 0));

      cwr.writeDwString(jBlk.getStr("name", ""));

      cwr.endBlock();
    }
    else if (strcmp(jtype, "hinge") == 0)
    {
      Point3 pos = tm.getcol(3);
      Point3 axis = tm.getcol(2);
      Point3 xAxis = tm.getcol(0);
      Point3 yAxis = tm.getcol(1);

      real minA = DegToRad(jBlk.getPoint3("minLimit", Point3(0, 0, 0)).x);
      real maxA = DegToRad(jBlk.getPoint3("maxLimit", Point3(0, 0, 0)).x);

      real midA = (minA + maxA) / 2;

      Point3 midAxis = xAxis * cosf(midA) + yAxis * sinf(midA);
      real angLimit = rabs(maxA - midA);

      cwr.beginBlock();
      cwr.writeFourCC(_MAKE4C('RJhi'));

      cwr.writeInt32e(body1Id);
      cwr.writeInt32e(body2Id);

      cwr.write32ex(&pos, sizeof(pos));
      cwr.write32ex(&axis, sizeof(axis));
      cwr.write32ex(&midAxis, sizeof(midAxis));
      cwr.write32ex(&xAxis, sizeof(xAxis));
      cwr.writeReal(angLimit);

      cwr.writeReal(jBlk.getReal("damping", 0));
      cwr.writeReal(jBlk.getReal("stiffness", 0));

      cwr.writeDwString(jBlk.getStr("name", ""));

      cwr.endBlock();
    }
    else if (strcmp(jtype, "revolution") == 0)
    {
      Point3 pos = tm.getcol(3);
      Point3 axis = tm.getcol(2);

      real minA = DegToRad(jBlk.getPoint3("minLimit", Point3(0, 0, 0)).x);
      real maxA = DegToRad(jBlk.getPoint3("maxLimit", Point3(0, 0, 0)).x);

      cwr.beginBlock();
      cwr.writeFourCC(_MAKE4C('Jrev'));

      cwr.writeInt32e(body1Id);
      cwr.writeInt32e(body2Id);

      cwr.write32ex(&pos, sizeof(pos));
      cwr.write32ex(&axis, sizeof(axis));
      cwr.writeReal(minA);
      cwr.writeReal(maxA);

      cwr.writeReal(jBlk.getReal("maxRestitution", 0));
      cwr.writeReal(jBlk.getReal("minRestitution", 0));
      cwr.writeReal(DegToRad(jBlk.getReal("projAngle", 0)));
      cwr.writeReal(jBlk.getReal("projDistance", 0));
      cwr.writeReal(jBlk.getReal("spring", 0));
      cwr.writeInt16e(jBlk.getInt("projType", 0));

      short flags = 0;
      if (jBlk.getBool("springEnabled", false))
        flags |= 0x1;
      if (jBlk.getBool("limitsEnabled", false))
        flags |= 0x2;

      cwr.writeInt16e(flags);
      cwr.writeReal(jBlk.getReal("damping", 0));

      cwr.writeDwString(jBlk.getStr("name", ""));

      cwr.endBlock();
    }
    else if (strcmp(jtype, "spherical") == 0)
    {
      Point3 pos = tm.getcol(3);
      Point3 dir = tm.getcol(0);
      Point3 axis = tm.getcol(2);

      cwr.beginBlock();
      cwr.writeFourCC(_MAKE4C('Jsph'));

      cwr.writeInt32e(body1Id);
      cwr.writeInt32e(body2Id);

      cwr.write32ex(&pos, sizeof(pos));
      cwr.write32ex(&dir, sizeof(dir));
      cwr.write32ex(&axis, sizeof(axis));

      cwr.writeReal(DegToRad(jBlk.getPoint3("minLimit", Point3(0, 0, 0)).x));
      cwr.writeReal(DegToRad(jBlk.getPoint3("maxLimit", Point3(0, 0, 0)).x));
      cwr.writeReal(jBlk.getReal("maxRestitution", 0));
      cwr.writeReal(jBlk.getReal("minRestitution", 0));
      cwr.writeReal(DegToRad(jBlk.getReal("swingValue", 0)));
      cwr.writeReal(jBlk.getReal("swingRestitution", 0));

      cwr.writeReal(jBlk.getReal("spring", 0));
      cwr.writeReal(jBlk.getReal("damping", 0));
      cwr.writeReal(jBlk.getReal("twistSpring", 0));
      cwr.writeReal(jBlk.getReal("twistDamping", 0));
      cwr.writeReal(jBlk.getReal("swingSpring", 0));
      cwr.writeReal(jBlk.getReal("swingDamper", 0));

      cwr.writeReal(jBlk.getReal("projDistance", 0));
      cwr.writeInt16e(jBlk.getInt("projType", 0));
      short flags = 0;
      if (jBlk.getBool("springEnabled", false))
        flags |= 0x1;
      if (jBlk.getBool("limitsEnabled", false))
        flags |= 0x2;

      cwr.writeInt16e(flags);

      cwr.writeDwString(jBlk.getStr("name", ""));

      cwr.endBlock();
    }
    else
      logerr("skipped unrecognized joint type=%s", jtype);
  }
};


class PhysObjRefs : public IDagorAssetRefProvider
{
public:
  const char *__stdcall getRefProviderIdStr() const override { return "physObj refs"; }

  const char *__stdcall getAssetType() const override { return TYPE; }

  void __stdcall onRegister() override {}
  void __stdcall onUnregister() override {}

  void __stdcall getAssetRefs(DagorAsset &a, Tab<Ref> &refs) override
  {
    static int dm_atype = -2;
    static int gn_atype = -2;
    if (dm_atype == -2)
      dm_atype = a.getMgr().getAssetTypeId("dynModel");
    if (gn_atype == -2)
      gn_atype = a.getMgr().getAssetTypeId("skeleton");

    refs.clear();
    if (dm_atype < 0)
      return;

    const char *dm_name = a.props.getStr("dynModel", "");
    DagorAsset *dm_a = a.getMgr().findAsset(dm_name, dm_atype);

    IDagorAssetRefProvider::Ref &r = refs.push_back();
    r.flags = RFLG_EXTERNAL;
    if (!dm_a)
      r.setBrokenRef(String(64, "%s:dynModel", dm_name));
    else
      r.refAsset = dm_a;

    const char *gn_name = a.props.getStr("skeleton", NULL);
    if (gn_name && gn_atype >= 0)
    {
      DagorAsset *gn_a = a.getMgr().findAsset(gn_name, gn_atype);

      IDagorAssetRefProvider::Ref &r = refs.push_back();
      r.flags = RFLG_EXTERNAL | RFLG_OPTIONAL;
      if (!gn_a)
        r.setBrokenRef(String(64, "%s:skeleton", gn_name));
      else
        r.refAsset = gn_a;
    }
  }
};


class PhysObjExporterPlugin : public IDaBuildPlugin
{
public:
  bool __stdcall init(const DataBlock &appblk) override
  {
    defRagdollBlk =
      *appblk.getBlockByNameEx("assets")->getBlockByNameEx("build")->getBlockByNameEx("physObj")->getBlockByNameEx("ragdoll");
    if (!defRagdollBlk.isEmpty())
      appBlkFname = appblk.resolveFilename();
    return true;
  }
  void __stdcall destroy() override { delete this; }

  int __stdcall getExpCount() override { return 1; }
  const char *__stdcall getExpType(int idx) override { return TYPE; }
  IDagorAssetExporter *__stdcall getExp(int idx) override { return &exp; }

  int __stdcall getRefProvCount() override { return 1; }
  const char *__stdcall getRefProvType(int idx) override { return TYPE; }
  IDagorAssetRefProvider *__stdcall getRefProv(int idx) override { return &ref; }

protected:
  PhysObjExporter exp;
  PhysObjRefs ref;
};

DABUILD_PLUGIN_API IDaBuildPlugin *__stdcall get_dabuild_plugin() { return new (midmem) PhysObjExporterPlugin; }
END_DABUILD_PLUGIN_NAMESPACE(physObj)
REGISTER_DABUILD_PLUGIN(physObj, nullptr)
