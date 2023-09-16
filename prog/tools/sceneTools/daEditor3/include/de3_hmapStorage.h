//
// DaEditor3
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <math/integer/dag_IBBox2.h>
#include <math/dag_e3dColor.h>
#include <generic/dag_tab.h>
#include <util/dag_globDef.h>

class IGenericProgressIndicator;

template <class T>
class SubMapStorage
{
public:
  virtual ~SubMapStorage() {}

  virtual void closeFile(bool finalize) = 0;
  virtual bool flushData() = 0;


  const T &getData(int x, int y) const
  {
    int ex, ey;
    calcElemCoords(x, y, ex, ey);

    const Element *elem = getElement(ex, ey);
    if (!elem)
      return defaultValue;

    return elem->data[y * elemSize + x];
  }


  bool setData(int x, int y, const T &value)
  {
    int ex, ey;
    calcElemCoords(x, y, ex, ey);

    Element *elem = getElement(ex, ey);
    if (!elem && value == defaultValue)
      return false;
    if (!elem)
      elem = createElement(ex, ey);
    if (!elem)
      return false;

    if (elem->data[y * elemSize + x] == value)
    {
      elem->isModified = true;
      return false;
    }
    elem->data[y * elemSize + x] = value;
    elem->isModified = true;
    return true;
  }


  int getElemSize() const { return elemSize; }
  int getMapSizeX() const { return mapSizeX; }
  int getMapSizeY() const { return mapSizeY; }


  void unloadUnchangedData(int less_than_y)
  {
    int ch = less_than_y / elemSize;
    if (ch > numElemsY)
      ch = numElemsY;

    for (int i = 0; i < ch * numElemsX; ++i)
    {
      Element &e = elems[i];
      if (e.isModified || !e.data)
        continue;
      e.release();
    }
  }

  void reset(int map_size_x, int map_size_y, int elem_size, const T &def_value)
  {
    closeFile(false);

    elemSize = elem_size;
    mapSizeX = map_size_x;
    mapSizeY = map_size_y;
    defaultValue = def_value;

    resetElems();
  }


public:
  T defaultValue;

protected:
  struct Element
  {
    T *data;

    unsigned timeStamp : 31;
    unsigned isModified : 1;

  public:
    Element() : data(NULL), isModified(false), timeStamp(0) {}
    Element(const Element &) = delete;
    ~Element() { release(); }

    void allocate(int size, IMemAlloc *mem)
    {
      G_ASSERT(!data);
      data = new (mem) T[size * size];
    }
    void release()
    {
      if (data)
      {
        delete[] data;
        data = NULL;
      }
    }
  };


  int mapSizeX, mapSizeY, elemSize;
  int numElemsX, numElemsY;

  Tab<Element> elems;


protected:
  SubMapStorage() : elems(midmem) {}

  void resetElems()
  {
    numElemsX = (mapSizeX + elemSize - 1) / elemSize;
    numElemsY = (mapSizeY + elemSize - 1) / elemSize;

    clear_and_shrink(elems);
    elems.resize(numElemsX * numElemsY);
  }

  void calcElemCoords(int &x, int &y, int &ex, int &ey) const
  {
    if (x < 0)
      x = 0;
    else if (x >= mapSizeX)
      x = mapSizeX - 1;

    if (y < 0)
      y = 0;
    else if (y >= mapSizeY)
      y = mapSizeY - 1;

    ex = x / elemSize;
    ey = y / elemSize;

    x -= ex * elemSize;
    y -= ey * elemSize;
  }


  Element *getElement(int ex, int ey)
  {
    G_ASSERT(ex >= 0 && ex < numElemsX && ey >= 0 && ey < numElemsY);

    int index = ey * numElemsX + ex;
    G_ASSERT(index < elems.size());

    if (elems[index].data)
      return &elems[index];

    return loadElement(index);
  }
  const Element *getElement(int ex, int ey) const { return const_cast<SubMapStorage<T> *>(this)->getElement(ex, ey); }


  Element *createElement(int ex, int ey)
  {
    int index = ey * numElemsX + ex;
    Element *e = &elems[index];
    e->allocate(elemSize, dag::get_allocator(elems));

    for (int i = 0; i < elemSize * elemSize; ++i)
      e->data[i] = defaultValue;

    return e;
  }

  virtual Element *loadElement(int index) = 0;
};
DAG_DECLARE_RELOCATABLE(SubMapStorage<float>::Element);
DAG_DECLARE_RELOCATABLE(SubMapStorage<uint64_t>::Element);
DAG_DECLARE_RELOCATABLE(SubMapStorage<uint32_t>::Element);
DAG_DECLARE_RELOCATABLE(SubMapStorage<uint16_t>::Element);
DAG_DECLARE_RELOCATABLE(SubMapStorage<E3DCOLOR>::Element);

template <class T>
class MapStorage
{
public:
  typedef SubMapStorage<T> SubMap;
  static const int SUBMAP_SZ = 2048;
  static const int ELEM_SZ = 256;

  virtual ~MapStorage() {}

  virtual bool createFile(const char *fname) = 0;
  virtual bool openFile(const char *fname) = 0;
  virtual void closeFile(bool finalize = false) = 0;
  virtual void eraseFile() = 0;

  virtual const char *getFileName() const = 0;
  virtual bool isFileOpened() const = 0;
  virtual bool canBeReverted() const = 0;
  virtual bool revertChanges() = 0;

  bool flushData()
  {
    bool ok = true;
    for (int i = 0; i < subMaps.size(); i++)
      if (subMaps[i])
        if (!subMaps[i]->flushData())
          ok = false;
    return ok;
  }


  const T &getData(int x, int y) const
  {
    int ex, ey;
    calcSubMapCoords(x, y, ex, ey);

    const SubMap *elem = getSubMap(ex, ey);
    if (!elem)
      return defaultValue;

    return elem->getData(x, y);
  }


  bool setData(int x, int y, const T &value)
  {
    int ex, ey;
    calcSubMapCoords(x, y, ex, ey);

    SubMap *elem = getSubMap(ex, ey);
    if (!elem && value == defaultValue)
      return false;
    if (!elem)
      elem = createSubMap(ex, ey);
    if (!elem)
      return false;

    return elem->setData(x, y, value);
  }

  void copyVal(int x, int y, MapStorage<T> &map)
  {
    int ex, ey;
    calcSubMapCoords(x, y, ex, ey);

    SubMap *s_elem = map.getSubMap(ex, ey);
    SubMap *d_elem = getSubMap(ex, ey);
    if ((!s_elem || s_elem->getData(x, y) == defaultValue) && (!d_elem || d_elem->getData(x, y) == defaultValue))
      return;

    if (!d_elem)
      d_elem = createSubMap(ex, ey);
    if (d_elem)
      d_elem->setData(x, y, s_elem ? s_elem->getData(x, y) : defaultValue);
  }

  int getElemSize() const { return ELEM_SZ; }
  int getMapSizeX() const { return mapSizeX; }
  int getMapSizeY() const { return mapSizeY; }

  void unloadUnchangedData(int less_than_y)
  {
    int ch = less_than_y / SUBMAP_SZ + 1;
    if (ch > numSubMapsY)
      ch = numSubMapsY;

    for (int i = 0; i < ch * numSubMapsX; ++i)
      if (subMaps[i])
        subMaps[i]->unloadUnchangedData(less_than_y - (i / numSubMapsX) * SUBMAP_SZ);
  }
  void unloadAllUnchangedData() { unloadUnchangedData((mapSizeY + ELEM_SZ - 1) / ELEM_SZ * ELEM_SZ); }

  void reset(int map_size_x, int map_size_y, const T &def_value)
  {
    closeFile();

    mapSizeX = map_size_x;
    mapSizeY = map_size_y;
    defaultValue = def_value;

    resetSubMaps();
  }


public:
  T defaultValue;

  int mapSizeX, mapSizeY;
  int numSubMapsX, numSubMapsY;

  Tab<SubMap *> subMaps;


protected:
  MapStorage() : subMaps(midmem) {}

  void resetSubMaps()
  {
    numSubMapsX = (mapSizeX + SUBMAP_SZ - 1) / SUBMAP_SZ;
    numSubMapsY = (mapSizeY + SUBMAP_SZ - 1) / SUBMAP_SZ;

    clear_all_ptr_items_and_shrink(subMaps);
    subMaps.resize(numSubMapsX * numSubMapsY);
    mem_set_0(subMaps);
  }

  void calcSubMapCoords(int &x, int &y, int &ex, int &ey) const
  {
    if (x < 0)
      x = 0;
    else if (x >= mapSizeX)
      x = mapSizeX - 1;

    if (y < 0)
      y = 0;
    else if (y >= mapSizeY)
      y = mapSizeY - 1;

    ex = x / SUBMAP_SZ;
    ey = y / SUBMAP_SZ;

    x -= ex * SUBMAP_SZ;
    y -= ey * SUBMAP_SZ;
  }


  SubMap *getSubMap(int ex, int ey)
  {
    G_ASSERT(ex >= 0 && ex < numSubMapsX && ey >= 0 && ey < numSubMapsY);

    int index = ey * numSubMapsX + ex;
    G_ASSERT(index < subMaps.size());

    if (subMaps[index])
      return subMaps[index];

    return loadSubMap(index);
  }
  const SubMap *getSubMap(int ex, int ey) const { return const_cast<MapStorage<T> *>(this)->getSubMap(ex, ey); }

  virtual SubMap *createSubMap(int ex, int ey) = 0;
  virtual SubMap *loadSubMap(int index) = 0;
};


class HeightMapStorage
{
public:
  void ctorClear()
  {
    hmInitial = NULL;
    hmFinal = NULL;
    heightScale = 1.0;
    heightOffset = 0.0;
    srcChanged.setEmpty();
    changed = false;
  }

  bool hasDistinctInitialAndFinalMap() const { return hmInitial != hmFinal; }

  MapStorage<float> &getInitialMap() { return *hmInitial; }
  MapStorage<float> &getFinalMap() { return *hmFinal; }

  float getInitialData(int x, int y) const { return hmInitial->getData(x, y) * heightScale + heightOffset; }
  float getFinalData(int x, int y) const { return hmFinal->getData(x, y) * heightScale + heightOffset; }
  void setInitialData(int x, int y, float data)
  {
    if (hmInitial->setData(x, y, (data - heightOffset) / heightScale))
      srcChanged += IPoint2(x, y);
  }
  void setFinalData(int x, int y, float data) { hmFinal->setData(x, y, (data - heightOffset) / heightScale); }
  void resetFinalData(int x, int y) { hmFinal->copyVal(x, y, *hmInitial); }

  void reset(int map_size_x, int map_size_y, float def_value)
  {
    hmInitial->reset(map_size_x, map_size_y, def_value);
    if (hasDistinctInitialAndFinalMap())
      hmFinal->reset(map_size_x, map_size_y, def_value);
  }
  void resetFinal()
  {
    if (!hasDistinctInitialAndFinalMap())
      return;
    hmFinal->reset(hmInitial->getMapSizeX(), hmInitial->getMapSizeY(), hmFinal->defaultValue);
    changed = true;
  }

  void clear()
  {
    hmInitial->eraseFile();
    if (hasDistinctInitialAndFinalMap())
      hmFinal->eraseFile();
  }

  bool flushData()
  {
    if (hasDistinctInitialAndFinalMap())
      hmFinal->flushData();
    return hmInitial->flushData();
  }

  void unloadUnchangedData(int less_than_y)
  {
    hmInitial->unloadUnchangedData(less_than_y);
    if (hasDistinctInitialAndFinalMap())
      hmFinal->unloadUnchangedData(less_than_y);
  }
  void unloadAllUnchangedData()
  {
    hmInitial->unloadAllUnchangedData();
    if (hasDistinctInitialAndFinalMap())
      hmFinal->unloadAllUnchangedData();
  }

  int getElemSize() const { return hmInitial->getElemSize(); }
  int getMapSizeX() const { return hmInitial->getMapSizeX(); }
  int getMapSizeY() const { return hmInitial->getMapSizeY(); }

  const char *getFileName() const { return hmInitial->getFileName(); }

  bool isFileOpened() const { return hmInitial->isFileOpened(); }

  void closeFile(bool finalize = false)
  {
    hmInitial->closeFile(finalize);
    hmFinal->closeFile(finalize);
  }

public:
  float heightScale, heightOffset;
  IBBox2 srcChanged;
  bool changed;

protected:
  MapStorage<float> *hmInitial, *hmFinal;
};
