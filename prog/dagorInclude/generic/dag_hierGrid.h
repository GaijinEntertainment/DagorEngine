//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <generic/dag_tab.h>
#include <math/dag_wooray2d.h>
#include <util/dag_globDef.h>

// 2D grid data //
template <class T, int LP>
struct GridDataLeaf2
{
  DAG_DECLARE_NEW(midmem)

  typedef T DataType;
  static constexpr int LPWR = LP;
  static constexpr int LSZ = 1 << LP;
  T data[LSZ][LSZ];
};

template <class Leaf>
class HierGrid2LeafCB
{

public:
  virtual void leaf_callback(Leaf *, int uo, int vo) = 0;
  virtual bool leaf_callback_stop(Leaf *l, int uo, int vo)
  {
    leaf_callback(l, uo, vo);
    return false;
  }
};

template <class LT, int LP>
class HierGrid2
{
public:
  DAG_DECLARE_NEW(midmem)

  static constexpr int LPWR = LP;
  static constexpr int LSZ = 1 << LP;
  typedef LT Leaf;

protected:
  struct Node
  {
    DAG_DECLARE_NEW(midmem)

    union
    {
      Node *sub[2][2];
      Node *sub_linear[4];
      Leaf *leaf[2][2];
      Leaf *leaf_linear[4];
    };
    Node() { memset(sub, 0, sizeof(sub)); }
    void copy(const Node &n, int sz)
    {
      sz >>= 1;
      if (sz == LSZ)
      {
        for (int i = 0; i < 4; ++i)
          if (n.leaf_linear[i])
          {
            if (leaf_linear[i])
              leaf_linear[i] = new Leaf(*n.leaf_linear[i]);
            else
              *leaf_linear[i] = *n.leaf_linear[i];
          }
          else
          {
            if (leaf_linear[i])
            {
              delete (leaf_linear[i]);
              leaf_linear[i] = NULL;
            }
          }
      }
      else
      {
        for (int i = 0; i < 4; ++i)
          if (n.sub_linear[i])
          {
            if (!sub_linear[i])
              sub_linear[i] = new Node;
            sub_linear[i]->copy(*n.sub_linear[i], sz);
          }
          else
          {
            if (sub_linear[i])
            {
              sub_linear[i]->destroy(sz);
              sub_linear[i] = NULL;
            }
          }
      }
    }

    void destroy(int sz)
    {
      sz >>= 1;
      if (sz == LSZ)
      {
        for (int i = 0; i < 4; ++i)
          if (leaf_linear[i])
            delete (leaf_linear[i]);
      }
      else
      {
        for (int i = 0; i < 4; ++i)
          if (sub_linear[i])
            sub_linear[i]->destroy(sz);
      }

      delete this;
    }

    Leaf *create_leaf(int u, int v, int sz)
    {
      sz >>= 1;
      if (sz == 1)
      {
        int su = u & 1, sv = v & 1;
        if (!leaf[sv][su])
          leaf[sv][su] = new Leaf;
        return leaf[sv][su];
      }
      else
      {
        int su = (u & sz ? 1 : 0), sv = (v & sz ? 1 : 0);
        if (!sub[sv][su])
          sub[sv][su] = new Node;
        return sub[sv][su]->create_leaf(u, v, sz);
      }
    }

    Leaf *get_leaf(int u, int v, int sz)
    {
      sz >>= 1;
      if (sz == 1)
      {
        return leaf[v & 1][u & 1];
      }
      else
      {
        int su = (u & sz ? 1 : 0), sv = (v & sz ? 1 : 0);
        if (!sub[sv][su])
          return NULL;
        return sub[sv][su]->get_leaf(u, v, sz);
      }
    }

    void enum_all_leaves(HierGrid2LeafCB<Leaf> &cb, int u, int v, int sz)
    {
      sz >>= 1;
      if (sz == LSZ)
      {
        if (leaf[0][0])
          cb.leaf_callback(leaf[0][0], u, v);
        if (leaf[0][1])
          cb.leaf_callback(leaf[0][1], u + LSZ, v);
        if (leaf[1][0])
          cb.leaf_callback(leaf[1][0], u, v + LSZ);
        if (leaf[1][1])
          cb.leaf_callback(leaf[1][1], u + LSZ, v + LSZ);
      }
      else
      {
        if (sub[0][0])
          sub[0][0]->enum_all_leaves(cb, u, v, sz);
        if (sub[0][1])
          sub[0][1]->enum_all_leaves(cb, u + sz, v, sz);
        if (sub[1][0])
          sub[1][0]->enum_all_leaves(cb, u, v + sz, sz);
        if (sub[1][1])
          sub[1][1]->enum_all_leaves(cb, u + sz, v + sz, sz);
      }
    }

    bool enum_all_leaves_ibbox2(HierGrid2LeafCB<Leaf> &cb, int u, int v, int sz, const IBBox2 &bx)
    {
      if (!(bx & IBBox2(IPoint2(u, v), IPoint2(u + sz - 1, v + sz - 1))))
        return false;
      sz >>= 1;
      if (sz == LSZ)
      {
        if (leaf[0][0])
          if (cb.leaf_callback_stop(leaf[0][0], u, v))
            return true;
        if (leaf[0][1])
          if (cb.leaf_callback_stop(leaf[0][1], u + LSZ, v))
            return true;
        if (leaf[1][0])
          if (cb.leaf_callback_stop(leaf[1][0], u, v + LSZ))
            return true;
        if (leaf[1][1])
          if (cb.leaf_callback_stop(leaf[1][1], u + LSZ, v + LSZ))
            return true;
      }
      else
      {
        if (sub[0][0])
          if (sub[0][0]->enum_all_leaves_ibbox2(cb, u, v, sz, bx))
            return true;
        if (sub[0][1])
          if (sub[0][1]->enum_all_leaves_ibbox2(cb, u + sz, v, sz, bx))
            return true;
        if (sub[1][0])
          if (sub[1][0]->enum_all_leaves_ibbox2(cb, u, v + sz, sz, bx))
            return true;
        if (sub[1][1])
          if (sub[1][1]->enum_all_leaves_ibbox2(cb, u + sz, v + sz, sz, bx))
            return true;
      }

      return false;
    }
  };
  struct TopNode : Node
  {
    int uo, vo;
    TopNode(int u, int v)
    {
      uo = u;
      vo = v;
    }
  };

  Tab<TopNode *> node;
  int clev, topsz;

public:
  HierGrid2(int cl) : node(midmem_ptr())
  {
    clev = 1;
    topsz = 1 << (1 + LP);
    set_levels(cl);
  }

  ~HierGrid2() { clear(); }
  void clear()
  {
    for (int i = 0; i < node.size(); ++i)
      if (node[i])
        node[i]->destroy(topsz);
    clear_and_shrink(node);
  }

  void move(int u, int v)
  {
    for (int i = 0; i < node.size(); ++i)
      if (node[i])
      {
        node[i]->uo += u;
        node[i]->vo += v;
      }
  }

  void copy(const HierGrid2<LT, LP> &s)
  {
    if (clev != s.clev)
    {
      clear();
      set_levels(s.clev);
    }

    int i;
    for (i = 0; i < node.size(); ++i)
      if (node[i])
      {
        int uo = node[i]->uo, vo = node[i]->vo;
        int j;
        for (j = 0; j < s.node.size(); ++j)
          if (s.node[j])
            if (s.node[j]->uo == uo && s.node[j]->vo == vo)
              break;
        if (j < s.node.size())
          continue;
        node[i]->destroy(topsz);
        node[i] = NULL;
      }

    for (i = 0; i < s.node.size(); ++i)
      if (s.node[i])
      {
        int uo = s.node[i]->uo, vo = s.node[i]->vo;
        int j;
        for (j = 0; j < node.size(); ++j)
          if (node[j])
            if (node[j]->uo == uo && node[j]->vo == vo)
              break;
        if (j >= node.size())
        {
          for (j = 0; j < node.size(); ++j)
            if (!node[j])
              break;
          if (j >= node.size())
          {
            j = append_items(node, 1);
            node[j] = NULL;
          }

          node[j] = new TopNode(uo, vo);
        }

        node[j]->copy(*s.node[i], topsz);
      }
  }

  void set_levels(int cl)
  {
    if (cl < 1)
      cl = 1;
    if (cl + LP > 30)
      cl = 30 - LP;
    if (clev == cl)
      return;
    clear(); //== could cut/grow tree to save data in leaves
    clev = cl;
    topsz = 1 << (clev + LP);
  }

  // (u,v) is some grid point of the leaf
  Leaf *create_leaf(int u, int v)
  {
    int uc = u & ~(topsz - 1), vc = v & ~(topsz - 1), i;
    for (i = 0; i < node.size(); ++i)
      if (node[i])
        if (node[i]->uo == uc && node[i]->vo == vc)
          break;
    if (i >= node.size())
    {
      for (i = 0; i < node.size(); ++i)
        if (!node[i])
          break;
      if (i >= node.size())
      {
        i = append_items(node, 1);
        node[i] = NULL;
      }

      node[i] = new TopNode(uc, vc);
    }

    return node[i]->create_leaf((u & (topsz - 1)) >> LP, (v & (topsz - 1)) >> LP, 1 << clev);
  }

  // (u,v) is some grid point of the leaf
  Leaf *get_leaf(int u, int v) const
  {
    int uc = u & ~(topsz - 1), vc = v & ~(topsz - 1), i;
    for (i = 0; i < node.size(); ++i)
      if (node[i])
        if (node[i]->uo == uc && node[i]->vo == vc)
          break;
    if (i >= node.size())
      return NULL;
    return node[i]->get_leaf((u & (topsz - 1)) >> LP, (v & (topsz - 1)) >> LP, 1 << clev);
  }

  void enum_all_leaves(HierGrid2LeafCB<Leaf> &cb)
  {
    for (int i = 0; i < node.size(); ++i)
      if (node[i])
        node[i]->enum_all_leaves(cb, node[i]->uo, node[i]->vo, topsz);
  }

  bool enum_all_leaves_ibbox2(HierGrid2LeafCB<Leaf> &cb, const IBBox2 &bx)
  {
    for (int i = 0; i < node.size(); ++i)
      if (node[i])
        if (node[i]->enum_all_leaves_ibbox2(cb, node[i]->uo, node[i]->vo, topsz, bx))
          return true;
    return false;
  }

  bool traceRay(const Point2 &cellSize, const Point2 &pt, const Point2 &dir, real &mint, HierGrid2LeafCB<Leaf> &cb)
  {
    IBBox2 boxlimits;
    for (int i = 0; i < node.size(); ++i)
      if (node[i])
        boxlimits += IBBox2(IPoint2(node[i]->uo, node[i]->vo), IPoint2(node[i]->uo + topsz - 1, node[i]->vo + topsz - 1));
    WooRay2d ray(Point2::xz(pt), Point2::xz(dir), mint, cellSize, boxlimits);

    IPoint2 diff = ray.getEndCell() - ray.currentCell();
    int n = 2 * ((diff.x >= 0 ? diff.x : -diff.x) + (diff.y >= 0 ? diff.y : -diff.y)) + 1;
    double t = 0;

    for (; n > 0; --n)
    {

      Leaf *leaf = get_leaf(ray.currentCell().x, ray.currentCell().y);
      if (leaf)
        if (cb.leaf_callback_stop(leaf, ray.currentCell().x, ray.currentCell().y))
          return true;

      if (!ray.nextCell(t))
        break;
    }

    return false;
  }
};

template <class Leaf>
class GridData2 : public HierGrid2<Leaf, Leaf::LPWR>
{
public:
  typedef HierGrid2<Leaf, Leaf::LPWR> BASE;
  typedef typename Leaf::DataType DataType;

  GridData2(int cl) : HierGrid2<Leaf, Leaf::LPWR>(cl) {}

  DataType *get_pt(int u, int v) const
  {
    Leaf *l = BASE::get_leaf(u, v);
    return l ? &l->data[v & (BASE::LSZ - 1)][u & (BASE::LSZ - 1)] : NULL;
  }

  void set_pt(int u, int v, const DataType &pt) { BASE::create_leaf(u, v)->data[v & (BASE::LSZ - 1)][u & (BASE::LSZ - 1)] = pt; }
};

// 3D grid data //
template <class T, int LP>
struct GridDataLeaf3
{
  DAG_DECLARE_NEW(midmem)

  typedef T DataType;
  static constexpr int LPWR = LP;
  static constexpr int LSZ = 1 << LP;
  T data[LSZ][LSZ][LSZ];
};

template <class Leaf>
class HierGrid3LeafCB
{
public:
  virtual void leaf_callback(Leaf *, int uo, int vo, int wo) = 0;
};

template <class LT, int LP>
class HierGrid3
{
public:
  DAG_DECLARE_NEW(midmem)

  static constexpr int LPWR = LP;
  static constexpr int LSZ = 1 << LP;
  typedef LT Leaf;

protected:
  IMemAlloc *mem;
  struct Node
  {
    DAG_DECLARE_NEW(midmem)

    union
    {
      Node *sub[2][2][2];
      Leaf *leaf[2][2][2];
    };
    Node() { memset(sub, 0, sizeof(sub)); }
    void copy(const Node &n, int sz, IMemAlloc *mem)
    {
      sz >>= 1;
      if (sz == LSZ)
      {
        for (int i = 0; i < 8; ++i)
          if (n.leaf[0][0][i])
          {
            if (!leaf[0][0][i])
              leaf[0][0][i] = new (mem) Leaf(*n.leaf[0][0][i]);
            else
              *leaf[0][0][i] = *n.leaf[0][0][i];
          }
          else
          {
            if (leaf[0][0][i])
            {
              delete (leaf[0][0][i]);
              leaf[0][0][i] = NULL;
            }
          }
      }
      else
      {
        for (int i = 0; i < 8; ++i)
          if (n.sub[0][0][i])
          {
            if (!sub[0][0][i])
              sub[0][0][i] = new (mem) Node;
            sub[0][0][i]->copy(*n.sub[0][0][i], sz);
          }
          else
          {
            if (sub[0][0][i])
            {
              sub[0][0][i]->destroy(sz);
              sub[0][0][i] = NULL;
            }
          }
      }
    }

    void destroy(int sz)
    {
      sz >>= 1;
      if (sz == LSZ)
      {
        for (int i = 0; i < 8; ++i)
          if (leaf[0][0][i])
            delete (leaf[0][0][i]);
      }
      else
      {
        for (int i = 0; i < 8; ++i)
          if (sub[0][0][i])
            sub[0][0][i]->destroy(sz);
      }

      delete this;
    }

    Leaf *create_leaf(int u, int v, int w, int sz, IMemAlloc *m)
    {
      sz >>= 1;
      if (sz == 1)
      {
        int su = u & 1, sv = v & 1, sw = w & 1;
        if (!leaf[sw][sv][su])
          leaf[sw][sv][su] = new (m) Leaf;
        return leaf[sw][sv][su];
      }
      else
      {
        int su = (u & sz ? 1 : 0), sv = (v & sz ? 1 : 0), sw = (w & sz ? 1 : 0);
        if (!sub[sw][sv][su])
          sub[sw][sv][su] = new (m) Node;
        return sub[sw][sv][su]->create_leaf(u, v, w, sz, m);
      }
    }

    Leaf *get_leaf(int u, int v, int w, int sz)
    {
      sz >>= 1;
      if (sz == 1)
      {
        return leaf[w & 1][v & 1][u & 1];
      }
      else
      {
        int su = (u & sz ? 1 : 0), sv = (v & sz ? 1 : 0), sw = (w & sz ? 1 : 0);
        if (!sub[sw][sv][su])
          return NULL;
        return sub[sw][sv][su]->get_leaf(u, v, w, sz);
      }
    }

    void enum_all_leaves(HierGrid3LeafCB<Leaf> &cb, int u, int v, int w, int sz)
    {
      sz >>= 1;
      if (sz == LSZ)
      {
        if (leaf[0][0][0])
          cb.leaf_callback(leaf[0][0][0], u, v, w);
        if (leaf[0][0][1])
          cb.leaf_callback(leaf[0][0][1], u + LSZ, v, w);
        if (leaf[0][1][0])
          cb.leaf_callback(leaf[0][1][0], u, v + LSZ, w);
        if (leaf[0][1][1])
          cb.leaf_callback(leaf[0][1][1], u + LSZ, v + LSZ, w);
        if (leaf[1][0][0])
          cb.leaf_callback(leaf[1][0][0], u, v, w + LSZ);
        if (leaf[1][0][1])
          cb.leaf_callback(leaf[1][0][1], u + LSZ, v, w + LSZ);
        if (leaf[1][1][0])
          cb.leaf_callback(leaf[1][1][0], u, v + LSZ, w + LSZ);
        if (leaf[1][1][1])
          cb.leaf_callback(leaf[1][1][1], u + LSZ, v + LSZ, w + LSZ);
      }
      else
      {
        if (sub[0][0][0])
          sub[0][0][0]->enum_all_leaves(cb, u, v, w, sz);
        if (sub[0][0][1])
          sub[0][0][1]->enum_all_leaves(cb, u + sz, v, w, sz);
        if (sub[0][1][0])
          sub[0][1][0]->enum_all_leaves(cb, u, v + sz, w, sz);
        if (sub[0][1][1])
          sub[0][1][1]->enum_all_leaves(cb, u + sz, v + sz, w, sz);
        if (sub[1][0][0])
          sub[1][0][0]->enum_all_leaves(cb, u, v, w + sz, sz);
        if (sub[1][0][1])
          sub[1][0][1]->enum_all_leaves(cb, u + sz, v, w + sz, sz);
        if (sub[1][1][0])
          sub[1][1][0]->enum_all_leaves(cb, u, v + sz, w + sz, sz);
        if (sub[1][1][1])
          sub[1][1][1]->enum_all_leaves(cb, u + sz, v + sz, w + sz, sz);
      }
    }
  };
  struct TopNode : Node
  {
    int uo, vo, wo;
    TopNode(int u, int v, int w)
    {
      uo = u;
      vo = v;
      wo = w;
    }
  };

  Tab<TopNode *> node;
  int clev, topsz;

public:
  HierGrid3(int cl, IMemAlloc *m = midmem_ptr()) : mem(m), node(m)
  {
    clev = 1;
    topsz = 1 << (1 + LP);
    set_levels(cl);
  }

  ~HierGrid3() { clear(); }

  void setmem(IMemAlloc *m)
  {
    mem = m;
    dag::set_allocator(node, m);
  }

  void clear()
  {
    for (int i = 0; i < node.size(); ++i)
      if (node[i])
        node[i]->destroy(topsz);
    clear_and_shrink(node);
  }

  void copy(const HierGrid3<LT, LP> &s)
  {
    if (clev != s.clev)
    {
      clear();
      set_levels(s.clev);
    }

    int i;
    for (i = 0; i < node.size(); ++i)
      if (node[i])
      {
        int uo = node[i]->uo, vo = node[i]->vo, wo = node[i]->wo, j;
        for (j = 0; j < s.node.size(); ++j)
          if (s.node[j])
            if (s.node[j]->uo == uo && s.node[j]->vo == vo && s.node[j]->wo == wo)
              break;
        if (j < s.node.size())
          continue;
        node[i]->destroy(topsz);
        node[i] = NULL;
      }

    for (i = 0; i < s.node.size(); ++i)
      if (s.node[i])
      {
        int uo = s.node[i]->uo, vo = s.node[i]->vo, wo = s.node[i]->wo, j;
        for (j = 0; j < node.size(); ++j)
          if (node[j])
            if (node[j]->uo == uo && node[j]->vo == vo && node[j]->wo == wo)
              break;
        if (j >= node.size())
        {
          for (j = 0; j < node.size(); ++j)
            if (!node[j])
              break;
          if (j >= node.size())
          {
            j = append_items(node, 1);
            node[j] = NULL;
          }

          node[j] = new (mem) TopNode(uo, vo, wo);
        }

        node[j]->copy(*s.node[i], topsz, mem);
      }
  }

  void set_levels(int cl)
  {
    if (cl < 1)
      cl = 1;
    if (cl + LP > 30)
      cl = 30 - LP;
    if (clev == cl)
      return;
    clear(); //== could cut/grow tree to save data in leaves
    clev = cl;
    topsz = 1 << (clev + LP);
  }

  // (u,v,w) is some grid point of the leaf
  Leaf *create_leaf(int u, int v, int w)
  {
    int uc = u & ~(topsz - 1), vc = v & ~(topsz - 1), wc = w & ~(topsz - 1), i;
    for (i = 0; i < node.size(); ++i)
      if (node[i])
        if (node[i]->uo == uc && node[i]->vo == vc && node[i]->wo == wc)
          break;
    if (i >= node.size())
    {
      for (i = 0; i < node.size(); ++i)
        if (!node[i])
          break;
      if (i >= node.size())
      {
        i = append_items(node, 1);
        node[i] = NULL;
      }

      node[i] = new (mem) TopNode(uc, vc, wc);
    }

    return node[i]->create_leaf((u & (topsz - 1)) >> LP, (v & (topsz - 1)) >> LP, (w & (topsz - 1)) >> LP, 1 << clev, mem);
  }

  // (u,v,w) is some grid point of the leaf
  Leaf *get_leaf(int u, int v, int w) const
  {
    int uc = u & ~(topsz - 1), vc = v & ~(topsz - 1), wc = w & ~(topsz - 1), i;
    for (i = 0; i < node.size(); ++i)
      if (node[i])
        if (node[i]->uo == uc && node[i]->vo == vc && node[i]->wo == wc)
          break;
    if (i >= node.size())
      return NULL;
    return node[i]->get_leaf((u & (topsz - 1)) >> LP, (v & (topsz - 1)) >> LP, (w & (topsz - 1)) >> LP, 1 << clev);
  }

  void enum_all_leaves(HierGrid3LeafCB<Leaf> &cb)
  {
    for (int i = 0; i < node.size(); ++i)
      if (node[i])
        node[i]->enum_all_leaves(cb, node[i]->uo, node[i]->vo, node[i]->wo, topsz);
  }
};

template <class Leaf>
class GridData3 : public HierGrid3<Leaf, Leaf::LPWR>
{
public:
  typedef HierGrid3<Leaf, Leaf::LPWR> BASE;
  typedef typename Leaf::DataType DataType;

  GridData3(int cl) : HierGrid3<Leaf, Leaf::LPWR>(cl) {}

  DataType *get_pt(int u, int v, int w)
  {
    Leaf *l = BASE::get_leaf(u, v, w);
    return l ? &l->data[w & (BASE::LSZ - 1)][v & (BASE::LSZ - 1)][u & (BASE::LSZ - 1)] : NULL;
  }

  void set_pt(int u, int v, int w, const DataType &pt)
  {
    BASE::create_leaf(u, v, w)->data[w & (BASE::LSZ - 1)][v & (BASE::LSZ - 1)][u & (BASE::LSZ - 1)] = pt;
  }
};
