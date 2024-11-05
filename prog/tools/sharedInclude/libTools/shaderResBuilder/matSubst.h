// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <ioSys/dag_dataBlock.h>
#include <3d/dag_materialData.h>
#include <generic/dag_ptrTab.h>
#include <generic/dag_tab.h>
#include <util/dag_oaHashNameMap.h>
#include <util/dag_simpleString.h>
#include <libTools/dagFileRW/dagFileNode.h>
#include <libTools/util/iLogWriter.h>
#include <libTools/shaderResBuilder/processMat.h>
#include <regExp/regExp.h>
#include <debug/dag_debug.h>

// material classname mapper
class DagorMatShaderSubstitute
{
  // parallel arrays: matClassOriginal.nameCount()==matClassSubst.size()
  FastNameMap matClassOriginal;
  Tab<SimpleString> matClassSubst;
  RegExp *reClsAllow = nullptr, *reClsDeny = nullptr;
  IProcessMaterialData *pm = nullptr;

public:
  ~DagorMatShaderSubstitute() { clear(); }

  void clear()
  {
    matClassOriginal.reset();
    clear_and_shrink(matClassSubst);
    del_it(reClsAllow);
    del_it(reClsDeny);
  }

  void setMatProcessor(IProcessMaterialData *_pm) { pm = _pm; }
  IProcessMaterialData *getMatProcessor() const { return pm; }

  void setupMatSubst(const DataBlock &blk)
  {
    if (blk.blockCount())
      if (blk.blockCount() > (blk.getBlockByName("allowShaders") ? 1 : 0) + (blk.getBlockByName("denyShaders") ? 1 : 0))
        logwarn("setupMatSubst uses new, blockless, BLK!");

    for (int i = 0; i < blk.paramCount(); i++)
      if (blk.getParamType(i) == DataBlock::TYPE_STRING)
      {
        int id = matClassOriginal.addNameId(blk.getParamName(i));
        if (id >= matClassSubst.size())
          matClassSubst.resize(id + 1);
        matClassSubst[id] = blk.getStr(i);
      }
    for (int i = 0; i < matClassSubst.size(); i++)
    {
      int id = matClassOriginal.getNameId(matClassSubst[i]);
      if (id >= 0 && id != i && strcmp(matClassSubst[i], matClassSubst[id]) != 0)
        logerr("matSubst: double remap detected at (%s)->(%s) and (%s)->(%s)", matClassOriginal.getName(i), matClassSubst[i],
          matClassSubst[i], matClassSubst[id]);
    }

    if (const DataBlock *b = blk.getBlockByName("allowShaders"))
    {
      const char *pattern = b->getStr("regExp", NULL);
      if (!pattern)
        logerr("matSubst: allow-shader pattern not specified in %s:t", "regExp");
      else
      {
        reClsAllow = new RegExp;
        if (!reClsAllow->compile(pattern, ""))
          logerr("matSubst: allow-shader pattern: '%s'", pattern);
      }
    }
    if (const DataBlock *b = blk.getBlockByName("denyShaders"))
    {
      const char *pattern = b->getStr("regExp", NULL);
      if (!pattern)
        logerr("matSubst: deny-shader pattern not specified in %s:t", "regExp");
      else
      {
        reClsDeny = new RegExp;
        if (!reClsDeny->compile(pattern, ""))
          logerr("matSubst: deny-shader pattern: '%s'", pattern);
      }
    }
    /*
      DEBUG_CTX("matClassOriginal.nameCount=%d matClassSubst.count=%d",
        matClassOriginal.nameCount(), matClassSubst.size());
      for (int i = 0; i < matClassOriginal.nameCount(); i ++)
        debug("  <%s> -> <%s>", matClassOriginal.getName(i), matClassSubst[i]);
    */
  }
  const char *substMatClassName(const char *class_name)
  {
    if (!class_name)
      return "";

    if (!matClassOriginal.nameCount())
      return class_name;
    int id = matClassOriginal.getNameId(class_name);
    if (id == -1)
    {
      if (!*class_name)
        return class_name;
      if ((reClsAllow && !reClsAllow->test(class_name)) || (reClsDeny && reClsDeny->test(class_name)))
        return class_name;
      if (strchr(class_name, ':') || strchr(class_name, '.')) // skip non-shader names (it may be proxymat qualified name)
        return class_name;
      logwarn("matSubst: <%s> -> Can't subst material! Adding equal subst", class_name);
      id = matClassOriginal.addNameId(class_name);
      G_ASSERT(id == matClassSubst.size());
      matClassSubst.push_back() = class_name;
    }
    else if (strcmp(class_name, matClassSubst[id]) != 0)
    {
      debug("matSubst: <%s> (%d) -> <%s>", class_name, id, matClassSubst[id]);
      return matClassSubst[id];
    }
    return class_name;
  }
  bool substMatClass(MaterialData &mat)
  {
    if (pm && pm->processMaterial(&mat) != &mat)
      return false;
    if (matClassOriginal.nameCount())
    {
      const char *n = substMatClassName(mat.className);
      if (n != mat.className.str())
        mat.className = n;
    }

    if (mat.className.empty())
      return true;
    if (reClsAllow && !reClsAllow->test(mat.className))
      return false;
    if (reClsDeny && reClsDeny->test(mat.className))
      return false;
    return true;
  }


  bool checkMatClasses(Node *n, const char *dag_fn, ILogWriter &log) const
  {
    if (!reClsAllow && !reClsDeny)
      return true;

    bool ok = true;
    if (n->mat)
      for (unsigned int materialNo = 0; materialNo < n->mat->subMatCount(); materialNo++)
      {
        MaterialData *subMat = n->mat->getSubMat(materialNo);
        if (!subMat)
          continue;

        int id = matClassOriginal.getNameId(subMat->className);
        const char *nm = id >= 0 ? matClassSubst[id] : subMat->className.str();
        if (!*nm)
          continue;
        if (reClsAllow && !reClsAllow->test(nm))
        {
          log.addMessage(ILogWriter::ERROR, "%s: sub-mat[%d] '%s' in node '%s' is not allowed", dag_fn, materialNo, nm, n->name);
          ok = false;
        }
        if (reClsDeny && reClsDeny->test(nm))
        {
          log.addMessage(ILogWriter::ERROR, "%s: sub-mat[%d] '%s' in node '%s' is forbidden", dag_fn, materialNo, nm, n->name);
          ok = false;
        }
      }

    for (int i = 0; i < n->child.size(); ++i)
      if (!checkMatClasses(n->child[i], dag_fn, log))
        ok = false;
    return ok;
  }
};

extern DagorMatShaderSubstitute static_geom_mat_subst;
