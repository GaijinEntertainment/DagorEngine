//
// DaEditorX
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/integer/dag_IBBox2.h>
#include <math/dag_e3dColor.h>
#include <generic/dag_tab.h>
#include <util/dag_globDef.h>

class IGenericProgressIndicator;


// Flat row-major pixel storage for editor maps with a sparse backing window.
// The map has two extents:
//   * Logical extent (mapSizeX, mapSizeY) -- reported by getMapSizeX/Y for
//     caller arithmetic (cell counts, world-coord conversion, exports). Not
//     used to bound get/setData.
//   * Stored extent (storedOfsX, storedOfsY, storedW, storedH) -- the actual
//     rectangle held in the `pixels` buffer and the only region get/setData
//     touch. Reads outside it clamp to the rect boundary; writes outside it
//     are silently dropped. Invariant: 0 <= storedOfs*, storedOfs* + storedW|H
//     <= mapSize*; preserved by every mutator.
//
// Maps like det-heightmap are logically huge (e.g. 32768x32768) but only a
// detRect-sized patch actually contains user data. Allocating the full logical
// extent blew ~4 GiB of RAM per plane for an 8 GiB working set on a
// 32768x32768 detail heightmap, so the in-memory Tab shrinks to the stored
// rect. Callers pick their allocation policy via the reset overloads:
//   * reset(w, h, defv)                          -> stored = full (0,0,w,h)
//   * resetStored(w, h, ox, oy, sw, sh, defv)    -> stored = (ox,oy,sw,sh)
//
// Previous versions of this class were paged: a Tab of SUBMAP_SZ submaps,
// each a Tab of ELEM_SZ tiles, with lazy allocation and modified-bit
// tracking. The stored-rect model keeps the same flat-access API while
// preserving the old paged format's ability to skip unused regions.
template <class T>
class MapStorage
{
public:
  virtual ~MapStorage() {}

  virtual bool createFile(const char *fname) = 0;
  virtual bool openFile(const char *fname) = 0;
  virtual void closeFile(bool finalize = false) = 0;
  virtual void eraseFile() = 0;

  virtual const char *getFileName() const = 0;
  virtual bool isFileOpened() const = 0;
  virtual bool canBeReverted() const = 0;
  virtual bool revertChanges() = 0;

  virtual bool flushData() = 0;


  const T &getData(int x, int y) const
  {
    if (storedW <= 0 || storedH <= 0)
      return defaultValue;
    if (x < storedOfsX)
      x = storedOfsX;
    else if (x >= storedOfsX + storedW)
      x = storedOfsX + storedW - 1;
    if (y < storedOfsY)
      y = storedOfsY;
    else if (y >= storedOfsY + storedH)
      y = storedOfsY + storedH - 1;
    return pixels[intptr_t(y - storedOfsY) * storedW + (x - storedOfsX)];
  }


  bool setData(int x, int y, const T &value)
  {
    const int lx = x - storedOfsX, ly = y - storedOfsY;
    if (unsigned(lx) >= unsigned(storedW) || unsigned(ly) >= unsigned(storedH))
      return false; // outside stored rect -> drop
    intptr_t idx = intptr_t(ly) * storedW + lx;
    if (pixels[idx] == value)
      return false;
    pixels[idx] = value;
    return true;
  }

  void copyVal(int x, int y, MapStorage<T> &src) { setData(x, y, src.getData(x, y)); }

  // ELEM_SZ is the legacy paged tile height; callers use it as a cadence
  // for progress ticks / periodic work in scan loops. With flat storage the
  // actual value is irrelevant, but keeping a mid-sized batch preserves the
  // pacing those loops were tuned for.
  static const int ELEM_SZ = 256;
  int getElemSize() const { return ELEM_SZ; }
  int getMapSizeX() const { return mapSizeX; }
  int getMapSizeY() const { return mapSizeY; }

  // Stored-rect accessors. Callers that serialize pixels directly (file
  // save paths) need offset+size of the actual backing buffer.
  int getStoredOfsX() const { return storedOfsX; }
  int getStoredOfsY() const { return storedOfsY; }
  int getStoredWidth() const { return storedW; }
  int getStoredHeight() const { return storedH; }

  // Legacy paged-storage shims: there are no tiles to evict in the flat
  // layout. Kept so callers that periodically unload as they scan don't
  // have to be edited at every site.
  void unloadUnchangedData(int) {}
  void unloadAllUnchangedData() {}

  // Full allocation: stored rect == logical extent. Callers that expect to
  // write into every (x,y) in [0,w)x[0,h) use this.
  void reset(int map_size_x, int map_size_y, const T &def_value)
  {
    resetStored(map_size_x, map_size_y, 0, 0, map_size_x, map_size_y, def_value);
  }

  // Sparse allocation: logical extent stays (mw, mh) but only the
  // (ofs_x, ofs_y, sw, sh) rectangle is backed by pixels. Pixels outside
  // the rect read as defaultValue; writes outside it are dropped.
  void resetStored(int map_size_x, int map_size_y, int ofs_x, int ofs_y, int sw, int sh, const T &def_value)
  {
    closeFile();

    mapSizeX = map_size_x;
    mapSizeY = map_size_y;
    defaultValue = def_value;
    storedOfsX = ofs_x;
    storedOfsY = ofs_y;
    storedW = sw;
    storedH = sh;

    const intptr_t total = intptr_t(sw) * intptr_t(sh);
    clear_and_shrink(pixels);
    if (total > 0)
    {
      pixels.resize(total);
      for (intptr_t i = 0; i < total; i++)
        pixels[i] = defaultValue;
    }
  }

  // O(1) rect-move: buffer contents stay bit-identical, only the
  // (storedOfsX, storedOfsY) interpretation shifts. Size is unchanged.
  // Callers use this for the "translate data" detRect-origin edit, where
  // the user wants the terrain silhouette to move with the rect.
  //
  // Caller must validate the new rect still fits inside [0, mapSize).
  // Defensive no-op if not, to avoid coordinates outside the logical map.
  void translateStoredRect(int new_ofs_x, int new_ofs_y)
  {
    if (new_ofs_x < 0 || new_ofs_y < 0 || new_ofs_x + storedW > mapSizeX || new_ofs_y + storedH > mapSizeY)
      return;
    storedOfsX = new_ofs_x;
    storedOfsY = new_ofs_y;
  }

  // General rect-move: allocate a new pixel buffer sized (sw, sh); copy the
  // world-coord overlap of old [storedOfs, storedOfs+storedW/H) and new
  // [ox, ox+sw/sh) from the old buffer into the new one. Non-overlap fills
  // with defaultValue. Replaces the backing Tab<T>. Handles crop (new fully
  // inside old), zero-expand (old fully inside new), and partial overlap
  // (the "move rect" origin edit) uniformly.
  void reshapeStoredRect(int ox, int oy, int sw, int sh)
  {
    if (sw < 0)
      sw = 0;
    if (sh < 0)
      sh = 0;
    if (ox < 0)
      ox = 0;
    if (oy < 0)
      oy = 0;
    if (ox + sw > mapSizeX)
      sw = mapSizeX - ox;
    if (oy + sh > mapSizeY)
      sh = mapSizeY - oy;
    if (sw < 0)
      sw = 0;
    if (sh < 0)
      sh = 0;

    const int oldOx = storedOfsX, oldOy = storedOfsY, oldSw = storedW, oldSh = storedH;
    Tab<T> oldPixels{midmem};
    oldPixels.swap(pixels);

    storedOfsX = ox;
    storedOfsY = oy;
    storedW = sw;
    storedH = sh;
    const intptr_t total = intptr_t(sw) * intptr_t(sh);
    pixels.resize(total);
    for (intptr_t i = 0; i < total; i++)
      pixels[i] = defaultValue;

    if (oldSw <= 0 || oldSh <= 0 || sw <= 0 || sh <= 0)
      return;

    // World-coord overlap of [oldOx, oldOx+oldSw) x [oldOy, oldOy+oldSh) and
    // [ox, ox+sw) x [oy, oy+sh). Copy overlap pixels from old local coords
    // (wx-oldOx, wy-oldOy) to new local coords (wx-ox, wy-oy).
    const int ovxMin = oldOx > ox ? oldOx : ox;
    const int ovyMin = oldOy > oy ? oldOy : oy;
    const int ovxMaxExcl = (oldOx + oldSw) < (ox + sw) ? (oldOx + oldSw) : (ox + sw);
    const int ovyMaxExcl = (oldOy + oldSh) < (oy + sh) ? (oldOy + oldSh) : (oy + sh);
    if (ovxMaxExcl <= ovxMin || ovyMaxExcl <= ovyMin)
      return;

    for (int wy = ovyMin; wy < ovyMaxExcl; wy++)
    {
      const int srcY = wy - oldOy;
      const int dstY = wy - oy;
      for (int wx = ovxMin; wx < ovxMaxExcl; wx++)
      {
        const int srcX = wx - oldOx;
        const int dstX = wx - ox;
        pixels[intptr_t(dstY) * sw + dstX] = oldPixels[intptr_t(srcY) * oldSw + srcX];
      }
    }
  }

  // Grow stored rect to at least cover [ox, ox+sw) x [oy, oy+sh), preserving
  // existing pixels at their logical (x, y) positions. Newly covered pixels
  // are filled with defaultValue. No-op if the request is already contained.
  //
  // Needed so legacy-migration loads (whose stored rect is the union bbox of
  // persisted submap files, potentially smaller than the caller's detRectC)
  // and detRect-expansion edits don't silently drop writes at the caller's
  // new coverage area.
  void expandStoredRect(int ox, int oy, int sw, int sh)
  {
    if (sw <= 0 || sh <= 0)
      return;
    const int reqEx = ox + sw, reqEy = oy + sh;
    if (storedW > 0 && storedH > 0 && ox >= storedOfsX && oy >= storedOfsY && reqEx <= storedOfsX + storedW &&
        reqEy <= storedOfsY + storedH)
      return;

    int newOx, newOy, newEx, newEy;
    if (storedW > 0 && storedH > 0)
    {
      newOx = storedOfsX < ox ? storedOfsX : ox;
      newOy = storedOfsY < oy ? storedOfsY : oy;
      newEx = (storedOfsX + storedW) > reqEx ? (storedOfsX + storedW) : reqEx;
      newEy = (storedOfsY + storedH) > reqEy ? (storedOfsY + storedH) : reqEy;
    }
    else
    {
      newOx = ox;
      newOy = oy;
      newEx = reqEx;
      newEy = reqEy;
    }
    if (newOx < 0)
      newOx = 0;
    if (newOy < 0)
      newOy = 0;
    if (newEx > mapSizeX)
      newEx = mapSizeX;
    if (newEy > mapSizeY)
      newEy = mapSizeY;
    const int newSw = newEx - newOx, newSh = newEy - newOy;
    if (newSw <= 0 || newSh <= 0)
      return;

    const int oldOx = storedOfsX, oldOy = storedOfsY, oldSw = storedW, oldSh = storedH;
    Tab<T> oldPixels{midmem};
    oldPixels.swap(pixels);

    storedOfsX = newOx;
    storedOfsY = newOy;
    storedW = newSw;
    storedH = newSh;
    const intptr_t total = intptr_t(newSw) * intptr_t(newSh);
    pixels.resize(total);
    for (intptr_t i = 0; i < total; i++)
      pixels[i] = defaultValue;

    if (oldSw > 0 && oldSh > 0)
    {
      const int copyDstX = oldOx - newOx;
      const int copyDstY = oldOy - newOy;
      for (int y = 0; y < oldSh; y++)
      {
        const int tgtY = copyDstY + y;
        if (unsigned(tgtY) >= unsigned(newSh))
          continue;
        for (int x = 0; x < oldSw; x++)
        {
          const int tgtX = copyDstX + x;
          if (unsigned(tgtX) >= unsigned(newSw))
            continue;
          pixels[intptr_t(tgtY) * newSw + tgtX] = oldPixels[intptr_t(y) * oldSw + x];
        }
      }
    }
  }

  // Row-major flat pixel buffer sized storedW * storedH.
  Tab<T> pixels{midmem};

public:
  T defaultValue;

  int mapSizeX = 0, mapSizeY = 0;
  int storedOfsX = 0, storedOfsY = 0;
  int storedW = 0, storedH = 0;

protected:
  MapStorage() = default;
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

  const MapStorage<float> &getInitialMap() const { return *hmInitial; }
  const MapStorage<float> &getFinalMap() const { return *hmFinal; }
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
  // Sparse variant: allocate only the (ofs_x, ofs_y, sw, sh) sub-rectangle.
  // Used for det-heightmap where the useful data lives inside detRect and
  // the logical (mapSize) extent is much larger.
  void resetStored(int map_size_x, int map_size_y, int ofs_x, int ofs_y, int sw, int sh, float def_value)
  {
    hmInitial->resetStored(map_size_x, map_size_y, ofs_x, ofs_y, sw, sh, def_value);
    if (hasDistinctInitialAndFinalMap())
      hmFinal->resetStored(map_size_x, map_size_y, ofs_x, ofs_y, sw, sh, def_value);
  }
  // Mirror initial's stored rect into hmFinal so paired lookups at matching
  // (x,y) hit the same bbox. Was reset(w,h,defv) before bbox -- that form
  // always allocated the full logical extent into hmFinal even when the
  // initial was sparse.
  void resetFinal()
  {
    if (!hasDistinctInitialAndFinalMap())
      return;
    hmFinal->resetStored(hmInitial->getMapSizeX(), hmInitial->getMapSizeY(), hmInitial->getStoredOfsX(), hmInitial->getStoredOfsY(),
      hmInitial->getStoredWidth(), hmInitial->getStoredHeight(), hmFinal->defaultValue);
    changed = true;
  }

  // Expand both stored rects to cover the requested region, preserving
  // existing pixels. Callers use this to ensure writes inside a newly
  // enlarged detRectC (or a legacy-migrated partial bbox) don't silently
  // drop at setData.
  void expandStoredRect(int ox, int oy, int sw, int sh)
  {
    hmInitial->expandStoredRect(ox, oy, sw, sh);
    if (hasDistinctInitialAndFinalMap())
      hmFinal->expandStoredRect(ox, oy, sw, sh);
    changed = true;
  }

  // Rect-move primitives for detRect-origin / detRect-size UI edits. Both
  // forward to both planes so splines baked into hmFinal stay aligned with
  // hmInitial. changed = true in both cases: translate needs a flush because
  // the on-disk interpretation of the byte payload shifts; reshape needs a
  // flush because the buffer itself was rebuilt.
  void translateStoredRect(int new_ofs_x, int new_ofs_y)
  {
    hmInitial->translateStoredRect(new_ofs_x, new_ofs_y);
    if (hasDistinctInitialAndFinalMap())
      hmFinal->translateStoredRect(new_ofs_x, new_ofs_y);
    changed = true;
  }
  void reshapeStoredRect(int ox, int oy, int sw, int sh)
  {
    hmInitial->reshapeStoredRect(ox, oy, sw, sh);
    if (hasDistinctInitialAndFinalMap())
      hmFinal->reshapeStoredRect(ox, oy, sw, sh);
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

  void unloadUnchangedData(int) {}
  void unloadAllUnchangedData() {}

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
