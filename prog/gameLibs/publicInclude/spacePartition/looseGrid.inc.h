#ifndef THREAD_SAFE_LOOSE_GRID
#error "Do not include this file directly, use dag_looseGrid.h instead"
#endif

class LooseGrid;
class LooseGridObject;

class LooseGridObjectNextHolder
{
public:
  LooseGridObject *nextInCell;
  LooseGridObjectNextHolder() : nextInCell(NULL) {}
};

class LooseGridObject : public LooseGridObjectNextHolder
{
public:
  LooseGridObjectNextHolder *prevInCell;
  uintptr_t gridPtr_LB;

  LooseGridObject() : prevInCell(NULL), gridPtr_LB(0) {}
  ~LooseGridObject();

  void setOwnerGrid(LooseGrid *owner_);
  LooseGrid *ownerGrid() const { return (LooseGrid *)(gridPtr_LB & ~uintptr_t(1)); }

  void setLarge(bool large) { large ? gridPtr_LB |= 1 : gridPtr_LB &= ~uintptr_t(1); }
  bool isLarge() const { return gridPtr_LB & 1; }

  bool isInGrid() const { return prevInCell != NULL; }
};

class LooseGrid
{
  friend class LooseGridIterator;
  friend class LooseGridBoxIterator;
  friend class LooseGridRayIterator;
  LooseGrid(int num_levels, bool locking_, int small_cell_size, int large_cell_size);

public:
  static LooseGrid *create(int num_levels = 1, bool locking_ = false, int small_cell_size = 128, int large_cell_size = 1024);
  ~LooseGrid();

  void add(LooseGridObject *object, const Point3 &pos);
  void add(LooseGridObject *object, const Point3 &pos, float radius);
  void remove(LooseGridObject *object);
  void reset()
  {
    mem_set_0(cells);
    mem_set_0(cellsBitMap);
  } // Warning: this won't remove already added cells from grid.

  void update(LooseGridObject *object, const Point3 &pos);
  void update(LooseGridObject *object, const Point3 &pos, float radius);

  int getLargeCount() const { return largeObjectsCount; }

#if THREAD_SAFE_LOOSE_GRID
  void lockRead() const;
  void unlockRead() const;
  void lockWrite() const;
  void unlockWrite() const;
#else
  void lockRead() const {}
  void unlockRead() const {}
  void lockWrite() const {}
  void unlockWrite() const {}
#endif

protected:
  void addObjectToCell(LooseGridObject *object, unsigned cell_no);

  dag::Span<LooseGridObjectNextHolder> cells; // points to this+1
  int largeObjectsCount;
  carray<int, GLEV_MAX> bitShift;
  float cellSizeSmall; // inverted & converted to float bitShift[GLEV_SMALL_OBJECTS]

#if THREAD_SAFE_LOOSE_GRID
  bool locking;
  mutable ReadWriteFifoLock gridLock;
#endif

  enum
  {
    CBT_SIZE = sizeof(uintptr_t) * 8,
    CBT_MASK = CBT_SIZE - 1
  };
  carray<uintptr_t, NUM_CELLS_TOTAL / CBT_SIZE> cellsBitMap;
};

struct LooseGridBoxIteratorLevel
{
  int x;
  int z;
  int minX;
  int minZ;
  int maxX;
  int maxZ;
};

class LooseGridIterator
{
public:
  void init(const LooseGrid *grid_);
  LooseGridIterator(const LooseGrid *grid_) { init(grid_); }
  ~LooseGridIterator();
  LooseGridObject *current() { return cur; }
  void next();

protected:
  const LooseGrid *ownerGrid = nullptr;
  LooseGridObject *cur = nullptr;
  int cellIndex = 0;
  inline void nextCell();
};

class LooseGridBoxIterator
{
public:
  void init(const LooseGrid *grid_, int bminX, int bminZ, int bmaxX, int bmaxZ, const bool cutLimit = true);
  LooseGridBoxIterator(const LooseGrid *grid_, int bminX, int bminZ, int bmaxX, int bmaxZ, const bool cutLimit = true)
  {
    init(grid_, bminX, bminZ, bmaxX, bmaxZ, cutLimit);
  }
  LooseGridBoxIterator(const LooseGrid *grid_, const BBox3 &box)
  {
    init(grid_, (int)box[0].x, (int)box[0].z, (int)box[1].x, (int)box[1].z);
  }
  ~LooseGridBoxIterator();
  LooseGridObject *current() { return cur; }
  void next();

protected:
  const LooseGrid *ownerGrid;
  LooseGridObject *cur;
  carray<LooseGridBoxIteratorLevel, GLEV_MAX> levels;
  int currentLevel;
  int levelsUsed;
#if DAGOR_DBGLEVEL > 0
  int nextCount;
  void debugGridState();
#endif
  inline void nextCell();
};

class LooseGridRayIterator : public LooseGridBoxIterator
{
public:
  void clampBoxByRay();
  LooseGridRayIterator(const LooseGrid *grid_, int bminX, int bminZ, int bmaxX, int bmaxZ, const float *p0_len,
    const float *norm_dir) :
    LooseGridBoxIterator(grid_, bminX, bminZ, bmaxX, bmaxZ, false), from(p0_len), dir(norm_dir)
  {
    clampBoxByRay();
    mem_set_0(usingBitMap);
    skipIrrelevantCells();
  }
  ~LooseGridRayIterator();
  void next();
  inline void markAsUsed(unsigned cellIndex)
  {
    usingBitMap[cellIndex / LooseGrid::CBT_SIZE] |= (uintptr_t(1) << (cellIndex & LooseGrid::CBT_MASK));
  }

protected:
  const float *from;
  const float *dir;
  inline void nextCell();
  void skipIrrelevantCells();
  carray<uintptr_t, NUM_CELLS_TOTAL / LooseGrid::CBT_SIZE> usingBitMap;
};
