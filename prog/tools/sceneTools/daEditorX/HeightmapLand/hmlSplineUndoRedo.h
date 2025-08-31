// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "hmlSplineObject.h"

void on_object_entity_name_changed(RenderableEditableObject &obj);

class UndoRefineSpline : public UndoRedoObject
{
public:
  SplineObject *s;
  Ptr<SplinePointObject> p;

  int idx, idx2, idx21;
  SplinePointObject::Props startPr1, startPr2;
  SplinePointObject::Props endPr1, endPr2;

  UndoRefineSpline(SplineObject *s_, SplinePointObject *p_, int idx_, SplinePointObject::Props &startPr1_,
    SplinePointObject::Props &startPr2_, SplinePointObject::Props &endPr1_, SplinePointObject::Props &endPr2_) :
    s(s_), idx(idx_), p(p_), startPr1(startPr1_), startPr2(startPr2_), endPr1(endPr1_), endPr2(endPr2_)
  {
    idx2 = (idx + 2) % s->points.size();
  }

  ~UndoRefineSpline() override { p = NULL; }

  void restore(bool save_redo) override
  {
    for (int i = 0; i < s->points.size(); i++)
      s->points[i]->arrId = i;

    s->points[idx]->setProps(startPr1);
    s->points[idx]->setPos(s->points[idx]->getPt());
    s->points[idx2]->setProps(startPr2);
    s->points[idx2]->setPos(s->points[idx2]->getPt());

    s->prepareSplineClassInPoints();
    s->pointChanged(-1);
    s->getSpline();
  }

  void redo() override
  {
    s->points[idx]->setProps(endPr1);
    s->points[idx]->setPos(s->points[idx]->getPt());
    s->points[idx2]->setProps(endPr2);
    s->points[idx2]->setPos(s->points[idx2]->getPt());

    s->prepareSplineClassInPoints();
    s->pointChanged(-1);
    s->getSpline();
  }

  size_t size() override { return sizeof(*this); }
  void accepted() override {}
  void get_description(String &s) override { s = "UndoRefineSpline"; }
};


class UndoSplitSpline : public UndoRedoObject
{
public:
  SplineObject *o1, *o2;
  int idx;
  HmapLandObjectEditor *ed;
  Ptr<SplinePointObject> p;

  UndoSplitSpline(SplineObject *o1_, SplineObject *o2_, int idx_, HmapLandObjectEditor *ed_) : o1(o1_), o2(o2_), ed(ed_), idx(idx_)
  {
    ed->addObject(o2);

    p = new SplinePointObject;
    p->spline = o2;

    redo();
    ed->addObject(p);
  }

  ~UndoSplitSpline() override { p = NULL; }

  void restore(bool save_redo) override
  {
    for (int i = 1; i < o2->points.size(); i++)
    {
      o2->points[i]->spline = o1;
      o2->points[i]->arrId = o1->points.size();
      o1->points.push_back(o2->points[i]);
    }

    clear_and_shrink(o2->points);

    for (int i = 0; i < o1->points.size(); i++)
      o1->points[i]->arrId = i;

    o1->prepareSplineClassInPoints();
    o1->pointChanged(-1);
    o1->getSpline();
    ed->unselectAll();
    o1->selectObject(true);
  }

  void redo() override
  {
    const char *assetBlkName = o1->getBlkGenName();
    for (int i = 1; i <= idx; i++)
      if (o1->points[i]->hasSplineClass())
        assetBlkName = o1->points[i]->getProps().blkGenName;

    p->setProps(o1->points[idx]->getProps());
    p->arrId = 0;

    p->setPos(p->getPt());

    o2->setBlkGenName(assetBlkName);

    for (int i = idx + 1; i < o1->points.size(); i++)
    {
      o1->points[i]->arrId = o2->points.size();
      o2->points.push_back(o1->points[i]);
      o1->points[i]->spline = o2;
    }

    erase_items(o1->points, idx + 1, o1->points.size() - idx - 1);

    for (int i = 0; i < o1->points.size(); i++)
      o1->points[i]->arrId = i;

    o1->prepareSplineClassInPoints();
    o1->pointChanged(-1);
    o1->getSpline();

    ed->unselectAll();

    o2->pointChanged(-1);
    o2->prepareSplineClassInPoints();
    o2->getSpline();
    o2->selectObject();
  }

  size_t size() override { return sizeof(*this); }
  void accepted() override {}
  void get_description(String &s) override { s = "UndoSplitSpline"; }
};


class UndoSplitPoly : public UndoRedoObject
{
public:
  SplineObject *o1, *o2;
  int idx1, idx2;
  HmapLandObjectEditor *ed;
  Ptr<SplinePointObject> p1, p2;

  UndoSplitPoly(SplineObject *o1_, SplineObject *o2_, int idx1_, int idx2_, HmapLandObjectEditor *ed_) :
    o1(o1_), o2(o2_), ed(ed_), idx1(idx1_), idx2(idx2_)
  {
    const char *assetBlkName1 = o1->getBlkGenName();
    for (int i = 1; i <= idx1; i++)
    {
      if (!o1->points[i]->getProps().blkGenName.empty())
        assetBlkName1 = o1->points[i]->getProps().blkGenName;
      else if (!o1->points[i]->getProps().useDefSet)
        assetBlkName1 = NULL;
    }

    const char *assetBlkName2 = assetBlkName1;
    for (int i = idx1 + 1; i <= idx2; i++)
    {
      if (!o1->points[i]->getProps().blkGenName.empty())
        assetBlkName2 = o1->points[i]->getProps().blkGenName;
      else if (!o1->points[i]->getProps().useDefSet)
        assetBlkName2 = NULL;
    }

    ed->addObject(o2);

    p1 = new SplinePointObject;
    p1->spline = o2;
    p1->setProps(o1->points[idx1]->getProps());
    p1->setPos(p1->getPt());

    p2 = new SplinePointObject;
    p2->spline = o2;
    p2->setProps(o1->points[idx2]->getProps());
    p2->setPos(p2->getPt());

    p1->arrId = 0;
    ed->addObject(p1);

    for (int i = idx1 + 1; i < idx2; i++)
    {
      o1->points[i]->arrId = o2->points.size();
      o2->points.push_back(o1->points[i]);
      o1->points[i]->spline = o2;
    }

    p2->arrId = o2->points.size();
    ed->addObject(p2);

    int eraseCnt = idx2 - idx1 - 1;
    erase_items(o1->points, idx1 + 1, eraseCnt);
    for (int i = 0; i < o1->points.size(); i++)
      o1->points[i]->arrId = i;

    o1->pointChanged(-1);
    o1->getSpline();
    o1->selectObject(false);

    o2->pointChanged(-1);
    o2->getSpline();
    o2->selectObject();

    p1->setBlkGenName(assetBlkName1);
    p2->setBlkGenName(assetBlkName2);
  }

  ~UndoSplitPoly() override
  {
    p1 = NULL;
    p2 = NULL;
  }

  void restore(bool save_redo) override
  {
    for (int i = 1; i < (int)o2->points.size() - 1; i++)
    {
      o2->points[i]->spline = o1;
      o2->points[i]->arrId = insert_items(o1->points, idx1 + i, 1, &o2->points[i]);
    }

    for (int i = 0; i < o1->points.size(); i++)
      o1->points[i]->arrId = i;

    clear_and_shrink(o2->points);

    o1->pointChanged(-1);
    o1->getSpline();
    o1->selectObject(true);

    p2->arrId = 1;
  }

  void redo() override
  {
    for (int i = idx1 + 1; i < idx2; i++)
    {
      o1->points[i]->arrId = insert_items(o2->points, i - idx1, 1, &o1->points[i]);
      o1->points[i]->spline = o2;
    }

    int eraseCnt = idx2 - idx1 - 1;
    erase_items(o1->points, idx1 + 1, eraseCnt);
    for (int i = 0; i < o1->points.size(); i++)
      o1->points[i]->arrId = i;

    for (int i = 0; i < o2->points.size(); i++)
      o2->points[i]->arrId = i;

    o1->pointChanged(-1);
    o1->getSpline();
    o1->selectObject(false);

    o2->pointChanged(-1);
    o2->getSpline();
    o2->selectObject();
  }

  size_t size() override { return sizeof(*this); }
  void accepted() override {}
  void get_description(String &s) override { s = "UndoSplitPoly"; }
};


class UndoReverse : public UndoRedoObject
{
public:
  Tab<SplineObject *> spls;

  UndoReverse(dag::ConstSpan<SplineObject *> spls_) : spls(tmpmem) { spls = spls_; }

  ~UndoReverse() override { spls.clear(); }

  void restore(bool save_redo) override
  {
    for (int i = 0; i < spls.size(); i++)
      spls[i]->reverse();
  }

  void redo() override
  {
    for (int i = 0; i < spls.size(); i++)
      spls[i]->reverse();
  }

  size_t size() override { return sizeof(*this); }
  void accepted() override {}
  void get_description(String &s) override { s = "UndoReverse"; }
};

class UndoAttachSpline : public UndoRedoObject
{
public:
  SplineObject *o1, *o2;
  HmapLandObjectEditor *ed;
  int cnt;

  UndoAttachSpline(SplineObject *o1_, SplineObject *o2_, HmapLandObjectEditor *ed_) : o1(o1_), o2(o2_), ed(ed_)
  {
    cnt = o2->points.size();
  }

  void restore(bool save_redo) override
  {
    int offs = o1->points.size() - cnt;
    for (int i = 0; i < cnt; i++)
    {
      o1->points[offs + i]->spline = o2;
      o1->points[offs + i]->arrId = o2->points.size();
      o2->points.push_back(o1->points[offs + i]);
    }
    erase_items(o1->points, offs, cnt);

    for (int i = 0; i < o1->points.size(); i++)
      o1->points[i]->arrId = i;

    for (int i = 0; i < o2->points.size(); i++)
      o2->points[i]->arrId = i;

    o1->pointChanged(-1);
    o1->getSpline();

    o2->pointChanged(-1);
    o2->getSpline();
  }

  void redo() override
  {
    append_items(o1->points, o2->points.size(), o2->points.data());

    for (int i = 0; i < o1->points.size(); i++)
    {
      o1->points[i]->spline = o1;
      o1->points[i]->arrId = i;
    }

    clear_and_shrink(o2->points);

    o1->pointChanged(-1);
    o1->getSpline();
  }

  size_t size() override { return sizeof(*this); }
  void accepted() override {}
  void get_description(String &s) override { s = "UndoAttachSpline"; }
};

class UndoChangeAsset : public UndoRedoObject
{
public:
  Ptr<SplineObject> s;
  String assetName;
  bool useDefSet;
  int idx;

  UndoChangeAsset(SplineObject *_s, int _idx) : s(_s), idx(_idx), useDefSet(false)
  {
    if (idx == -1)
      assetName = s->getProps().blkGenName;
    else
    {
      assetName = s->points[idx]->getProps().blkGenName;
      useDefSet = s->points[idx]->getProps().useDefSet;
    }
  }

  void restore(bool save_redo) override
  {
    String tmp;
    bool tmp_use = false;

    if (save_redo)
    {
      if (idx == -1)
        tmp = s->getProps().blkGenName;
      else
      {
        tmp = s->points[idx]->getProps().blkGenName;
        tmp_use = s->points[idx]->getProps().useDefSet;
      }
    }

    if (idx == -1)
    {
      s->setBlkGenName(assetName);
      if (s->isPoly())
        s->resetSplineClass();
      else if (s->points.size() > 0)
      {
        s->points[0]->resetSplineClass();
        s->markAssetChanged(0);
      }

      on_object_entity_name_changed(*s);
    }
    else
    {
      SplinePointObject::Props p = s->points[idx]->getProps();
      p.blkGenName = assetName;
      p.useDefSet = useDefSet;
      s->points[idx]->setProps(p);
      s->points[idx]->resetSplineClass();
      s->markAssetChanged(idx);
    }

    if (save_redo)
    {
      assetName = tmp;
      useDefSet = tmp_use;
    }

    s->getObjEditor()->invalidateObjectProps();
  }

  void redo() override { restore(true); }

  size_t size() override { return sizeof(*this); }
  void accepted() override {}
  void get_description(String &s) override { s = "UndoChangeAsset"; }
};
