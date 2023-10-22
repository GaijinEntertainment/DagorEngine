//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <ioSys/dag_memIo.h>
#include <util/dag_globDef.h>
#include <generic/dag_patchTab.h>
#include <generic/dag_carray.h>

// 3D grid data

template <class Leaf>
class RTHierGrid3LeafCB
{
public:
  virtual void leaf_callback(Leaf **, int uo, int vo, int wo) = 0;
};

template <class LT, int LP>
class RTHierGrid3
{
public:
  DAG_DECLARE_NEW(midmem)

  static constexpr int LPWR = LP;
  static constexpr int LSZ = 1 << LP;
  typedef LT Leaf;

protected:
  union GetLeafResult
  {
    Leaf **leaf;

  private:
    int l;

  public:
    GetLeafResult(Leaf *&leaf_) : leaf(&leaf_) {}
    GetLeafResult(int l_) : l((l_ << 1) | 1) {}
    explicit operator bool() const { return (l & 1) == 0; } // return true if contains leaf (otherwise it's contains level)
    int getl() const { return l >> 1; }
  };

  struct Node
  {
    DAG_DECLARE_NEW(midmem)

    union UData
    {
      Node *node;
      Leaf *leaf;
      char *ptr;
#if !_TARGET_64BIT
      int padd[2]; // to be compatible with PatchablePtr<>
#endif
      void patchNonNull(void *base) { ptr = ptr ? ((char *)base + (ptrdiff_t)ptr) : NULL; }
    };
    carray<UData, 8> subData;

    Node *sub_linear(int i) { return subData[i].node; }
    Node *sub(int x, int y, int z) { return sub_linear(x * 4 + y * 2 + z); }
    Leaf *leaf_linear(int i) { return subData[i].leaf; }
    Leaf *leaf(int x, int y, int z) { return leaf_linear(x * 4 + y * 2 + z); }

    Node *&sub_linear_ref(int i) { return subData[i].node; }
    Node *&sub_ref(int x, int y, int z) { return sub_linear_ref(x * 4 + y * 2 + z); }
    Leaf *&leaf_linear_ref(int i) { return subData[i].leaf; }
    Leaf *&leaf_ref(int x, int y, int z) { return leaf_linear_ref(x * 4 + y * 2 + z); }

    Node() { mem_set_0(subData); }
    void destroy(int sz)
    {
      sz >>= 1;
      if (sz == LSZ)
      {
        for (int i = 0; i < 8; ++i)
          if (leaf_linear(i))
            delete leaf_linear(i);
      }
      else
      {
        for (int i = 0; i < 8; ++i)
          if (sub_linear(i))
          {
            sub_linear(i)->destroy(sz);
            delete sub_linear(i);
          }
      }
    }
    Leaf **create_leaf(int u, int v, int w, int sz, IMemAlloc *m)
    {
      sz >>= 1;
      if (sz == 1)
      {
        int su = u & 1, sv = v & 1, sw = w & 1;
        if (!leaf(sw, sv, su))
          leaf_ref(sw, sv, su) = new (m) Leaf;
        return &leaf_ref(sw, sv, su);
      }
      else
      {
        int su = (u & sz ? 1 : 0), sv = (v & sz ? 1 : 0), sw = (w & sz ? 1 : 0);
        if (!sub(sw, sv, su))
          sub_ref(sw, sv, su) = new (m) Node;
        return sub(sw, sv, su)->create_leaf(u, v, w, sz, m);
      }
    }
    GetLeafResult get_leaf(int u, int v, int w, int sz)
    {
      sz >>= 1;
      if (sz == 1)
      {
        return GetLeafResult(leaf_ref(w & 1, v & 1, u & 1));
      }
      else
      {
        int su = (u & sz ? 1 : 0), sv = (v & sz ? 1 : 0), sw = (w & sz ? 1 : 0);
        if (!sub(sw, sv, su))
        {
          int l;
          for (l = 0; sz; l++, sz >>= 1)
            ;
          return GetLeafResult(l);
        }
        return sub(sw, sv, su)->get_leaf(u, v, w, sz);
      }
    }
    void serialize(int sz, IGenSave &cb, const void *dump_base)
    {
      sz >>= 1;
      int64_t savesub[8];
      memset(savesub, 0, sizeof(savesub));
      int subTell = cb.tell();
      G_STATIC_ASSERT(sizeof(savesub) == sizeof(Node));
      cb.write(savesub, sizeof(savesub));
      if (sz == LSZ)
      {
        for (int i = 0; i < 8; ++i)
          if (leaf_linear(i))
          {
            savesub[i] = cb.tell();
            leaf_linear(i)->serialize(cb, dump_base);
          }
      }
      else
      {
        for (int i = 0; i < 8; ++i)
          if (sub_linear(i))
          {
            savesub[i] = cb.tell();
            sub_linear(i)->serialize(sz, cb, dump_base);
          }
      }
      cb.seekto(subTell);
      cb.write(savesub, sizeof(savesub));
      cb.seektoend();
    }
    template <class PatchLeaf>
    void patch(int sz, void *base, void *dump_base, const PatchLeaf &patch_leaf)
    {
      sz >>= 1;
      for (int i = 0; i < 8; ++i)
        subData[i].patchNonNull(base);
      if (sz == LSZ)
      {
        for (int i = 0; i < 8; ++i)
          if (leaf_linear(i))
            patch_leaf(*leaf_linear(i), base, dump_base);
      }
      else
      {
        for (int i = 0; i < 8; ++i)
          if (sub_linear(i))
            sub_linear(i)->patch(sz, base, dump_base, patch_leaf);
      }
    }
    void enum_all_leaves(RTHierGrid3LeafCB<Leaf> &cb, int u, int v, int w, int sz)
    {
      sz >>= 1;
      if (sz == LSZ)
      {
        if (leaf(0, 0, 0))
          cb.leaf_callback(&leaf_ref(0, 0, 0), u, v, w);
        if (leaf(0, 0, 1))
          cb.leaf_callback(&leaf_ref(0, 0, 1), u + LSZ, v, w);
        if (leaf(0, 1, 0))
          cb.leaf_callback(&leaf_ref(0, 1, 0), u, v + LSZ, w);
        if (leaf(0, 1, 1))
          cb.leaf_callback(&leaf_ref(0, 1, 1), u + LSZ, v + LSZ, w);
        if (leaf(1, 0, 0))
          cb.leaf_callback(&leaf_ref(1, 0, 0), u, v, w + LSZ);
        if (leaf(1, 0, 1))
          cb.leaf_callback(&leaf_ref(1, 0, 1), u + LSZ, v, w + LSZ);
        if (leaf(1, 1, 0))
          cb.leaf_callback(&leaf_ref(1, 1, 0), u, v + LSZ, w + LSZ);
        if (leaf(1, 1, 1))
          cb.leaf_callback(&leaf_ref(1, 1, 1), u + LSZ, v + LSZ, w + LSZ);
      }
      else
      {
        if (sub(0, 0, 0))
          sub(0, 0, 0)->enum_all_leaves(cb, u, v, w, sz);
        if (sub(0, 0, 1))
          sub(0, 0, 1)->enum_all_leaves(cb, u + sz, v, w, sz);
        if (sub(0, 1, 0))
          sub(0, 1, 0)->enum_all_leaves(cb, u, v + sz, w, sz);
        if (sub(0, 1, 1))
          sub(0, 1, 1)->enum_all_leaves(cb, u + sz, v + sz, w, sz);
        if (sub(1, 0, 0))
          sub(1, 0, 0)->enum_all_leaves(cb, u, v, w + sz, sz);
        if (sub(1, 0, 1))
          sub(1, 0, 1)->enum_all_leaves(cb, u + sz, v, w + sz, sz);
        if (sub(1, 1, 0))
          sub(1, 1, 0)->enum_all_leaves(cb, u, v + sz, w + sz, sz);
        if (sub(1, 1, 1))
          sub(1, 1, 1)->enum_all_leaves(cb, u + sz, v + sz, w + sz, sz);
      }
    }
  };
  struct TopNode : Node
  {
    int uo, vo, wo, _resv;
    TopNode(int u, int v, int w)
    {
      uo = u;
      vo = v;
      wo = w;
      _resv = 0;
    }
    void serializeChildren(int ofs, int sz, IGenSave &cb, const void *dump_base)
    {
      sz >>= 1;
      int64_t savesub[8];
      memset(savesub, 0, sizeof(savesub));
      int subTell = ofs;
      G_STATIC_ASSERT(sizeof(savesub) == sizeof(Node));
      if (sz == LSZ)
      {
        for (int i = 0; i < 8; ++i)
          if (Node::leaf_linear(i))
          {
            savesub[i] = cb.tell();
            Node::leaf_linear(i)->serialize(cb, dump_base);
          }
      }
      else
      {
        for (int i = 0; i < 8; ++i)
          if (Node::sub_linear(i))
          {
            savesub[i] = cb.tell();
            Node::sub_linear(i)->serialize(sz, cb, dump_base);
          }
      }
      cb.seekto(subTell);
      cb.write(savesub, sizeof(savesub));
      cb.seektoend();
    }
    void serialize(int sz, IGenSave &cb)
    {
      G_UNREFERENCED(sz);
      _resv = 0;
      cb.write(this, sizeof(*this));
    }
  };

  PatchableTab<TopNode> nodes;
  int clev, topsz;
  inline unsigned nodeCount() const { return nodes.size(); }
  TopNode &node(int i) { return nodes[i]; }
  const TopNode &node(int i) const { return nodes[i]; }

public:
  void serialize(IGenSave &cb, const void *dump_base)
  {
    G_STATIC_ASSERT(sizeof(*this) == 6 * sizeof(int));
    cb.writeInt(cb.tell() + sizeof(*this));
    cb.writeInt(nodes.size());
    cb.writeInt(0);
    cb.writeInt(0);
    cb.writeInt(clev);
    cb.writeInt(topsz);

    for (unsigned i = 0; i < nodeCount(); ++i)
      node(i).serialize(topsz, cb);
    for (unsigned i = 0; i < nodeCount(); ++i)
      node(i).serializeChildren(i * sizeof(TopNode) + sizeof(*this), topsz, cb, dump_base);
  }

  struct PatchLeafMember
  {
    void operator()(Leaf &leaf, void *base, void *dump_base) const { leaf.patch(base, dump_base); }
  };
  template <class PatchLeaf = PatchLeafMember>
  void patch(void *base, void *dump_base, const PatchLeaf &node_patcher = PatchLeaf())
  {
    G_ASSERTF((uintptr_t(this) % sizeof(void *)) == 0, "RTHierGrid3 bad alignment %p, level re-export required", this);
    nodes.patch(base);
    for (unsigned i = 0; i < nodeCount(); ++i)
      node(i).patch(topsz, base, dump_base, node_patcher);
  }
  RTHierGrid3(int cl)
  {
    clev = 1;
    topsz = 1 << (1 + LP);
    setLevels(cl);
  }
  ~RTHierGrid3() { clear(); }
  void clear()
  {
    for (unsigned i = 0; i < nodeCount(); ++i)
      node(i).destroy(topsz);
    nodes.init(NULL, 0);
  }
  void setLevels(int cl)
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
  const int getLevels() const { return clev; }
  // (u,v,w) is some grid point of the leaf
  TopNode *findTopNode(int uc, int vc, int wc)
  {
    TopNode *s = nodes.data();
    for (TopNode *i = s, *end = s + nodeCount(); i != end; ++i)
      if (i->uo == uc && i->vo == vc && i->wo == wc)
        return i;
    return nullptr;
  }
  // (u,v,w) is some grid point of the leaf
  GetLeafResult get_leaf(int u, int v, int w)
  {
    int uc = u & ~(topsz - 1), vc = v & ~(topsz - 1), wc = w & ~(topsz - 1);
    TopNode *tn = findTopNode(uc, vc, wc);
    if (!tn)
      return GetLeafResult(clev);
    return tn->get_leaf((u & (topsz - 1)) >> LP, (v & (topsz - 1)) >> LP, (w & (topsz - 1)) >> LP, 1 << clev);
  }
  GetLeafResult get_leaf(int u, int v, int w) const { return const_cast<RTHierGrid3<LT, LP> *>(this)->get_leaf(u, v, w); }
  void enum_all_leaves(RTHierGrid3LeafCB<Leaf> &cb)
  {
    for (unsigned i = 0; i < nodeCount(); ++i)
      node(i).enum_all_leaves(cb, node(i).uo, node(i).vo, node(i).wo, topsz);
  }
};
