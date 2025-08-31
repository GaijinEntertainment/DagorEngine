LooseGrid::LooseGrid(int num_levels, bool locking_, int small_cell_size, int large_cell_size) :
  largeObjectsCount(0),
#if THREAD_SAFE_LOOSE_GRID
  locking(locking_),
#endif
  cells((LooseGridObjectNextHolder *)(this + 1), num_levels * NUM_CELLS_LEVEL)
{
#if !THREAD_SAFE_LOOSE_GRID
  G_UNREFERENCED(locking_);
#endif
  G_STATIC_ASSERT(!(NUM_CELLS_LEVEL & (NUM_CELLS_LEVEL - 1))); // must be power of two for correct mask
  G_ASSERT(is_pow_of2(small_cell_size));
  G_ASSERT(is_pow_of2(large_cell_size));

  bitShift[0] = get_log2i(small_cell_size);
  bitShift[1] = get_log2i(large_cell_size);
  cellSizeSmall = float(1 << bitShift[0]);

  reset();
}

/* static */
LooseGrid *LooseGrid::create(int num_levels /*=1*/, bool locking_ /*=false*/, int small_cell_size /*=128*/,
  int large_cell_size /*=1024*/)
{
  void *g = memalloc(sizeof(LooseGrid) + (num_levels * NUM_CELLS_LEVEL) * sizeof(LooseGridObjectNextHolder), midmem);
  return new (g, _NEW_INPLACE) LooseGrid(num_levels, locking_, small_cell_size, large_cell_size);
}

LooseGrid::~LooseGrid() {}

#if THREAD_SAFE_LOOSE_GRID
void LooseGrid::lockWrite() const
{
  if (locking)
    gridLock.lockWrite();
}

void LooseGrid::unlockWrite() const
{
  if (locking)
    gridLock.unlockWrite();
}

void LooseGrid::lockRead() const
{
  if (locking)
    gridLock.lockRead();
}

void LooseGrid::unlockRead() const
{
  if (locking)
    gridLock.unlockRead();
}
#endif

void LooseGrid::update(LooseGridObject *object, const Point3 &pos)
{
  lockWrite();

  if (object->isInGrid())
    remove(object);
  add(object, pos);

  unlockWrite();
}

void LooseGrid::update(LooseGridObject *object, const Point3 &pos, float radius)
{
  lockWrite();

  if (object->isInGrid())
    remove(object);
  add(object, pos, radius);

  unlockWrite();
}

static inline unsigned pos_to_cell(const Point3 &pos, int bitShift, int gridLevel)
{
  int x = ((int)pos.x >> bitShift) & GRID_SIZE_MASK;
  int z = ((int)pos.z >> bitShift) & GRID_SIZE_MASK;
  return (unsigned int)(z * GRID_SIZE_CELLS + x + gridLevel * NUM_CELLS_LEVEL);
}

void LooseGrid::add(LooseGridObject *object, const Point3 &pos, float radius)
{
  if (radius < cellSizeSmall)
  {
    addObjectToCell(object, pos_to_cell(pos, bitShift[0], GLEV_SMALL_OBJECTS));
    object->setLarge(false);
  }
  else
  {
    G_ASSERT(radius < float(1 << bitShift[1]));
    addObjectToCell(object, pos_to_cell(pos, bitShift[1], GLEV_LARGE_OBJECTS));
    object->setLarge(true);
    largeObjectsCount++;
  }
}

void LooseGrid::add(LooseGridObject *object, const Point3 &pos)
{
  addObjectToCell(object, pos_to_cell(pos, bitShift[0], GLEV_SMALL_OBJECTS));
}

void LooseGrid::addObjectToCell(LooseGridObject *object, unsigned cell_no)
{
  G_ASSERT(!object->prevInCell);
  G_ASSERT(!object->nextInCell);

  LooseGridObjectNextHolder &cell = cells[cell_no];

#if VERIFY_LOOSE_GRID_INTEGRITY
  for (LooseGridObject *holder = cell.nextInCell; holder; holder = holder->nextInCell) // check for loop
    G_ASSERT(holder != object);
#endif

  if (cell.nextInCell) // occupied?
  {
    G_ASSERT(cellsBitMap[cell_no / CBT_SIZE] & (uintptr_t(1) << (cell_no & CBT_MASK))); // bit must be set
    G_ASSERT(cell.nextInCell->prevInCell == &cell);
    cell.nextInCell->prevInCell = object;
    object->nextInCell = cell.nextInCell;
  }
  else
    G_ASSERT((cellsBitMap[cell_no / CBT_SIZE] & (uintptr_t(1) << (cell_no & CBT_MASK))) == 0); // bit must be free

  cell.nextInCell = object;
  object->prevInCell = &cell;

  // mark this cell in bitmap as well
  cellsBitMap[cell_no / CBT_SIZE] |= uintptr_t(1) << (cell_no & CBT_MASK);
}

void LooseGrid::remove(LooseGridObject *object)
{
  G_ASSERT(object->prevInCell);
  G_ASSERT(object->prevInCell->nextInCell == object);
  object->prevInCell->nextInCell = object->nextInCell;

  if (object->nextInCell)
  {
    G_ASSERT(object->nextInCell->prevInCell == object);
    object->nextInCell->prevInCell = object->prevInCell;
  }

  if (object->nextInCell == NULL && object->prevInCell >= cells.cbegin() && object->prevInCell < cells.cend())
  {
    unsigned cellIndex = object->prevInCell - cells.data();
    G_ASSERT(cellsBitMap[cellIndex / CBT_SIZE] & (uintptr_t(1) << (cellIndex & CBT_MASK)));
    cellsBitMap[cellIndex / CBT_SIZE] &= ~(uintptr_t(1) << (cellIndex & CBT_MASK)); // clear bit (cell is now empty)
  }

  object->prevInCell = NULL;
  object->nextInCell = NULL;

  if (object->isLarge())
  {
    G_ASSERT(largeObjectsCount > 0);
    largeObjectsCount--;
  }
  object->setLarge(false);
}

LooseGridObject::~LooseGridObject()
{
  if (ownerGrid() && prevInCell)
    ownerGrid()->remove(this);
}

void LooseGridObject::setOwnerGrid(LooseGrid *owner_)
{
  G_ASSERT(owner_);
  gridPtr_LB = uintptr_t(owner_) | (gridPtr_LB & 1);
}

// NOTE: static analysis cannot understand that ownerGrid == grid_
void LooseGridIterator::init(const LooseGrid *grid_) DAG_TS_NO_THREAD_SAFETY_ANALYSIS
{
  ownerGrid = grid_;
  cur = NULL;
  cellIndex = 0;
  ownerGrid->lockRead();

  nextCell();
}

LooseGridIterator::~LooseGridIterator() { ownerGrid->unlockRead(); }

void LooseGridIterator::next()
{
  G_ASSERT(cur);

  LooseGridObject *n = cur->nextInCell;
  if (n)
    cur = n;
  else // continue search
    nextCell();
}

inline void LooseGridIterator::nextCell()
{
  // move working vars into registers
  cur = NULL;
  while (!cur && cellIndex < ownerGrid->cells.size())
    cur = ownerGrid->cells[cellIndex++].nextInCell;
}

#define STI(var) var = l##var // store back to inst

// NOTE: static analysis cannot understand that ownerGrid == grid_
void LooseGridBoxIterator::init(const LooseGrid *grid_, int bminX, int bminZ, int bmaxX, int bmaxZ,
  const bool cutLimit) DAG_TS_NO_THREAD_SAFETY_ANALYSIS
{
  ownerGrid = grid_;
  cur = NULL;
  currentLevel = 0;
#if VERIFY_LOOSE_GRID_INTEGRITY
  nextCount = 0;
#endif
  G_ASSERT(grid_);

  levelsUsed = grid_->largeObjectsCount ? GLEV_MAX : GLEV_LARGE_OBJECTS;

  for (int i = 0; i < levelsUsed; ++i)
  {
    const int lminX = (bminX >> grid_->bitShift[i]) - 1;
    const int lminZ = (bminZ >> grid_->bitShift[i]) - 1;
    int lmaxX = (bmaxX >> grid_->bitShift[i]) + 1;
    int lmaxZ = (bmaxZ >> grid_->bitShift[i]) + 1;
    if (cutLimit)
    {
      lmaxX = min(lmaxX, lminX + GRID_SIZE_CELLS);
      lmaxZ = min(lmaxZ, lminZ + GRID_SIZE_CELLS);
    }

    LooseGridBoxIteratorLevel &data = levels[i];
    data.x = lminX - 1;
    data.z = lminZ;
    data.minX = lminX;
    data.minZ = lminZ;
    data.maxX = lmaxX;
    data.maxZ = lmaxZ;
  }

  ownerGrid->lockRead();

  nextCell();
}

LooseGridBoxIterator::~LooseGridBoxIterator() { ownerGrid->unlockRead(); }

#if DAGOR_DBGLEVEL > 0
void LooseGridBoxIterator::debugGridState()
{
#if VERIFY_LOOSE_GRID_INTEGRITY
  LooseGridObject *lcur = cur;
  debug("currentLevel=%d levelsUsed=%d", currentLevel, levelsUsed);
  if (currentLevel < GLEV_MAX)
  {
    debug("x=%d z=%d cur=0x%p minX=%d maxX=%d minZ=%d maxZ=%d\ndump cur iterator:", levels[currentLevel].x, levels[currentLevel].z,
      cur, levels[currentLevel].minX, levels[currentLevel].maxX, levels[currentLevel].minZ, levels[currentLevel].maxZ);
    for (int i = 0; lcur && i <= 4096; ++i, lcur = lcur->nextInCell)
      debug("%d 0x%p", i, lcur);
    debug("dump whole grid:");
    for (int i = 0; i < ownerGrid->cells.size(); ++i)
    {
      LooseGridObjectNextHolder *h = ownerGrid->cells[i].nextInCell;
      if (!h)
        continue;
      debug("%d:", i);
      for (int j = 0; h && j < 4096; ++j, h = h->nextInCell)
        debug("\t%d 0x%p", j, h);
    }

    for (int i = 0; i < ownerGrid->cells.size(); ++i)
    {
      const LooseGridObjectNextHolder *h0 = ownerGrid->cells[i].nextInCell;
      for (int k0 = 0; k0 < 4096 && h0; ++k0, h0 = h0->nextInCell)
        for (int j = 0; j < ownerGrid->cells.size(); ++j)
        {
          const LooseGridObjectNextHolder *h1 = ownerGrid->cells[j].nextInCell;
          for (int k1 = 0; k1 < 4096 && h1; ++k1, h1 = h1->nextInCell)
            G_ASSERTF((i == j && k0 == k1) || h0 != h1, "grid iterator - loop detected %d %d 0x%p", i, j, h0);
        }
    }
  }
  debug("loops not found");
  G_ASSERTF(0, "sanity check on grid iterator failed, iterator and grid dumped");
#endif
}
#endif

void LooseGridBoxIterator::next()
{
  G_ASSERT(cur);

#if VERIFY_LOOSE_GRID_INTEGRITY
  if (nextCount++ >= (128 << 10))
    debugGridState();
#endif

  LooseGridObject *n = cur->nextInCell;
  if (n)
    cur = n;
  else // continue search
    nextCell();
}

inline void LooseGridBoxIterator::nextCell()
{
  // move working vars into registers
  LooseGridObject *new_cur = NULL;
  for (; currentLevel < levelsUsed; ++currentLevel)
  {
    const LooseGridObjectNextHolder *__restrict cells = ownerGrid->cells.data() + currentLevel * NUM_CELLS_LEVEL;
    const uintptr_t *bitMap = &ownerGrid->cellsBitMap[currentLevel * NUM_CELLS_LEVEL / LooseGrid::CBT_SIZE];
    LooseGridBoxIteratorLevel &levelData = levels[currentLevel];
    int lx = levelData.x, lz = levelData.z;
    const int lminX = levelData.minX, lmaxX = levelData.maxX, lmaxZ = levelData.maxZ;
    int zrow = (lz & GRID_SIZE_MASK) * GRID_SIZE_CELLS;
    unsigned int cellNo;

    for (;;)
    {
      if (++lx > lmaxX)
      {
        if (++lz > lmaxZ)
          goto no_cur_;
        zrow = (lz & GRID_SIZE_MASK) * GRID_SIZE_CELLS;
        lx = lminX;
      }
      unsigned cellIndex = (zrow + (lx & GRID_SIZE_MASK));
      if ((bitMap[cellIndex / LooseGrid::CBT_SIZE] & (uintptr_t(1) << (cellIndex & LooseGrid::CBT_MASK))) == 0) // early reject by
                                                                                                                // bitMap
      {
        G_ASSERT(cells[cellIndex].nextInCell == NULL);
        continue;
      }

      new_cur = cells[cellIndex].nextInCell;
      G_ASSERT(new_cur);
      break;
    }
    cur = new_cur;

    levelData.x = lx;
    levelData.z = lz;
    return;
  no_cur_:;
  }
  cur = NULL;
  return;
}


LooseGridRayIterator::~LooseGridRayIterator() {}

void LooseGridRayIterator::clampBoxByRay()
{
  for (int i = 0; i < levelsUsed; ++i)
  {
    LooseGridBoxIteratorLevel &levelData = levels[i];
    if (dir[0] < 0.f)
      levelData.minX = max(levelData.minX, levelData.maxX - (GRID_SIZE_CELLS << 3));
    else
      levelData.maxX = min(levelData.maxX, levelData.minX + (GRID_SIZE_CELLS << 3));
    if (dir[2] < 0.f)
      levelData.minZ = max(levelData.minZ, levelData.maxZ - (GRID_SIZE_CELLS << 3));
    else
      levelData.maxZ = min(levelData.maxZ, levelData.minZ + (GRID_SIZE_CELLS << 3));
  }
}

void LooseGridRayIterator::next()
{
  G_ASSERT(cur);

#if VERIFY_LOOSE_GRID_INTEGRITY
  if (nextCount++ >= (128 << 10))
    debugGridState();
#endif

  LooseGridObject *n = cur->nextInCell;
  if (n)
    cur = n;
  else // continue search
    nextCell();
}

inline void LooseGridRayIterator::nextCell()
{
  LooseGridBoxIterator::nextCell();
  skipIrrelevantCells();
}

void LooseGridRayIterator::skipIrrelevantCells()
{
  if (currentLevel >= levelsUsed)
    return;
  Point2 start(from[0], from[2]);
  Point2 ldir(dir[0], dir[2]);
  ldir = normalize(ldir);
  ldir.x = fabsf(ldir.x) > VERY_SMALL_NUMBER ? 1.0f / ldir.x : 1e20;
  ldir.y = fabsf(ldir.y) > VERY_SMALL_NUMBER ? 1.0f / ldir.y : 1e20;

  bool valid = false;
  while (cur && !valid)
  {
    int bitShift = ownerGrid->bitShift[currentLevel];
    LooseGridBoxIteratorLevel &levelData = levels[currentLevel];
    valid = false;
    unsigned cellIndex =
      (((levelData.z & GRID_SIZE_MASK) * GRID_SIZE_CELLS) + (levelData.x & GRID_SIZE_MASK)) + currentLevel * NUM_CELLS_LEVEL;
    if ((usingBitMap[cellIndex / LooseGrid::CBT_SIZE] & (uintptr_t(1) << (cellIndex & LooseGrid::CBT_MASK))) == 0)
    {
      BBox2 objBox;
      objBox.lim[0] = Point2((levelData.x - 1) << bitShift, (levelData.z - 1) << bitShift);
      objBox.lim[1] = Point2((levelData.x + 2) << bitShift, (levelData.z + 2) << bitShift);
      Point2 l1 = objBox.lim[0] - start;
      Point2 l2 = objBox.lim[1] - start;
      valid = l1.x <= 0 && l2.x >= 0 && l1.y <= 0 && l2.y >= 0;
      l1 = Point2(l1.x * ldir.x, l1.y * ldir.y);
      l2 = Point2(l2.x * ldir.x, l2.y * ldir.y);
      Point2 pmin(min(l1.x, l2.x), min(l1.y, l2.y));
      Point2 pmax(max(l1.x, l2.x), max(l1.y, l2.y));
      float lmin = max(pmin.x, pmin.y);
      float lmax = min(pmax.x, pmax.y);
      valid = ((lmax > 0.f) && (lmax >= lmin) && (lmin >= 0.f)) || valid;

      if (valid)
        markAsUsed(cellIndex);
    }
    if (!valid)
      LooseGridBoxIterator::nextCell();
  }
}
