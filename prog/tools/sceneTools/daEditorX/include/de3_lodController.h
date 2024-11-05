//
// DaEditorX
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

class GeomNodeTree;

class ILodController
{
public:
  static constexpr unsigned HUID = 0xBF220CC9u; // ILodController

  //! returns lod count
  virtual int getLodCount() = 0;
  //! sets currently lod to @n (if -1 use auto choose)
  virtual void setCurLod(int n) = 0;
  //! returns distance of visibility for @lod_n
  virtual real getLodRange(int lod_n) = 0;

  //! returns number of TEX quality levels
  virtual int getTexQLCount() const = 0;
  //! sets currently texture quality level
  virtual void setTexQL(int ql) = 0;
  //! sets currently texture quality level
  virtual int getTexQL() const = 0;

  //! returns number of named nodes
  virtual int getNamedNodeCount() = 0;
  //! returns name of node by index
  virtual const char *getNamedNode(int idx) = 0;
  //! returns named node index by name
  virtual int getNamedNodeIdx(const char *nm) = 0;
  //! returns current visibility of named node
  virtual bool getNamedNodeVisibility(int idx) = 0;
  //! sets current visibility of named node
  virtual void setNamedNodeVisibility(int idx, bool vis) = 0;

  //! returns name of node by its index (more wide than getNamedNode)
  virtual const char *getNodeName(int idx) { return nullptr; }
  //! returns original skeleton (cannot be modified)
  virtual const GeomNodeTree *getSkeleton() { return nullptr; }
  //! sets external pointer to skeleton to be used for render
  virtual void setSkeletonForRender(GeomNodeTree *skel) {}
};
