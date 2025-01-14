#include <PowerOfTwoBinPack.h>

namespace rbp {

Rect PowerOfTwoBinPack::insertLogSize(unsigned logSize)
{
  if (logSize >= sizeBucket.size())
    return emptyRect();
  if (sizeBucket[logSize].size())//there is required size already
  {
    //ideally we need to find bucket from greater quad which is occipied as most, as possible, and use it. i.e. last one out of 4, etc
    //but just pop any is faster
    //return popBucketUnsafe(logSize);
    if (logSize == sizeBucket.size()-1)//it is biggest possible, nothing can be done to optimize
    {
      Rect rect = popBucketUnsafe(logSize);
      rbp_assert((rect.x&((1<<logSize)-1)) == 0 && (rect.y&((1<<logSize)-1)) == 0);
      return rect;
    }
    const uint32_t mask = ~((1<<(logSize+1))-1);
    int bestScore = 4, bestIndex = -1;
    for (int i = (int)sizeBucket[logSize].size()-1; i >= 0 ; --i)
    {
      const uint32_t x = (sizeBucket[logSize][i].x&mask), y = (sizeBucket[logSize][i].y&mask);
      int j = 1;
      for (; j < 3 && j <= i; ++j)
        if ((sizeBucket[logSize][i-j].x&mask) != x || ((sizeBucket[logSize][i-j].y&mask) != y))
          break;

      if (j==1)
      {
        Rect rect = sizeBucket[logSize][i];
        rbp_assert((rect.x&((1<<logSize)-1)) == 0 && (rect.y&((1<<logSize)-1)) == 0);
        sizeBucket[logSize].erase(sizeBucket[logSize].begin()+i);
        return rect;
      }
      if (j<bestScore)
      {
        bestScore = j;
        bestIndex = i;
      }
      i -= j-1;
    }
    rbp_assert(bestIndex>=0);
    Rect rect = sizeBucket[logSize][bestIndex];
    rbp_assert((rect.x&((1<<logSize)-1)) == 0 && (rect.y&((1<<logSize)-1)) == 0);
    sizeBucket[logSize].erase(sizeBucket[logSize].begin()+bestIndex);
    return rect;
  }
  int firstToSplit = logSize+1;
  for (; firstToSplit < sizeBucket.size(); firstToSplit++)//we have to split boxes before,
  {
    if (sizeBucket[firstToSplit].size())
      break;
  }
  if (firstToSplit >= sizeBucket.size())// no empty boxes
    return emptyRect();
  for (; firstToSplit>logSize; firstToSplit--)
    splitBucket(firstToSplit);
  Rect rect = popBucketUnsafe(logSize);
  rbp_assert((rect.x&((1<<logSize)-1)) == 0 && (rect.y&((1<<logSize)-1)) == 0);
  return rect;
}

void PowerOfTwoBinPack::init(int widthBuckets, int heightBuckets, int maxBucket)
{
  rbp_assert(is_pow_of2(maxBucket));
  int bigger = get_bigger_pow2(maxBucket);
  if (bigger>maxBucket)
    maxBucket = bigger>>1;
  int logSize = get_log2i(maxBucket);
  sizeBucket.resize(0);
  sizeBucket.resize(logSize+1);
  Rect rect; rect.width = rect.height = maxBucket;
  sizeBucket[logSize].reserve(heightBuckets*widthBuckets);
  for (int y = 0; y < heightBuckets; ++y)
    for (int x = 0; x < widthBuckets; ++x)
    {
      rect.x = x<<logSize; rect.y = y<<logSize;
      sizeBucket[logSize].push_back(rect);
    }
}

void PowerOfTwoBinPack::freeUsedRect(const Rect &rect)
{
  if (rect.width == 0)
    return;
  #if RBP_DEBUG_OVERLAP
    int i = 0;
    for (; i < allocated.size(); ++i)
      if (allocated[i].width == rect.width && allocated[i].x == rect.x && allocated[i].y == rect.y)
        break;
    rbp_assert(i < allocated.size());
    allocated.erase(allocated.begin() + i);
  #endif
  rbp_assert(rect.width == rect.height);
  rbp_assert(is_pow_of2(rect.width));
  int logSize = get_log2i(rect.width);
  rbp_assert((rect.x&((1<<logSize)-1)) == 0 && (rect.y&((1<<logSize)-1)) == 0);
  rbp_assert(logSize<sizeBucket.size());
  freeUsedRectInternal(rect, logSize);
}

void PowerOfTwoBinPack::freeUsedRectInternal(Rect rect, uint32_t logSize)
{
  if (logSize == sizeBucket.size()-1)//don't coalesce biggest chunk
  {
    rbp_assert((rect.x&((1<<logSize)-1)) == 0 && (rect.y&((1<<logSize)-1)) == 0);
    sizeBucket[logSize].push_back(rect);//
    return;
  }
  rbp_assert((rect.x&((1<<logSize)-1)) == 0 && (rect.y&((1<<logSize)-1)) == 0);

  const uint32_t mask = ~((1<<(logSize+1))-1);
  const uint32_t x = rect.x&mask, y = rect.y&mask;
  for (int i = 0; i < sizeBucket[logSize].size(); ++i)
  {
    if ((sizeBucket[logSize][i].x&mask) == x && (sizeBucket[logSize][i].y&mask) == y)
    {
      int j = 1;
      for (; j < 3 && j < sizeBucket[logSize].size()-i; ++j)
        if ((sizeBucket[logSize][i+j].x&mask) != x || ((sizeBucket[logSize][i+j].y&mask) != y))
          break;
      if (j<3)//we can't coalesce rect
      {
        sizeBucket[logSize].insert(sizeBucket[logSize].begin()+i+j, rect);//insert as late, as possible to reduce mem moves
        return;
      }
      //coalesce box
      sizeBucket[logSize].erase(sizeBucket[logSize].begin()+i, sizeBucket[logSize].begin()+i+3);
      rect.x = x; rect.y = y; rect.width <<= 1;rect.height <<= 1;
      freeUsedRectInternal(rect, logSize+1);//coalesce upper box
      return;
    }
  }
  sizeBucket[logSize].push_back(rect);//hadn't found such quad. insert as late, as possible to reduce mem moves
}

bool PowerOfTwoBinPack::intersectsWithFree(const Rect &rect, Rect &with, int ci, int id) const
{
  for (int i = 0; i < sizeBucket.size(); ++i)
  {
    for (int j = 0; j < sizeBucket[i].size(); ++j)
      if (i != ci || j != id)
      {
        if (!DisjointRectCollection::Disjoint(rect, sizeBucket[i][j]))
        {
          with = sizeBucket[i][j];
          return true;
        }
      }
  }
  return false;
}

bool PowerOfTwoBinPack::validate()
{
  #if RBP_DEBUG_OVERLAP
  for (int i = 0; i < allocated.size(); ++i)
  {
    Rect with;
    G_ASSERTF(!intersectsWithFree(allocated[i], with), "%d (%dx%d %dx%d) vs (%dx%d) (%dx%d)",
      i, allocated[i].x, allocated[i].y, allocated[i].width, allocated[i].height,
      with.x, with.y, with.width, with.height);
  }
  #endif
  for (int i = 0; i < sizeBucket.size(); ++i)
  {
    for (int j = 0; j < sizeBucket[i].size(); ++j)
    {
      rbp_assert(sizeBucket[i][j].width == (1<<i));
      rbp_assert(sizeBucket[i][j].width == sizeBucket[i][j].height);
      rbp_assert(sizeBucket[i][j].x % sizeBucket[i][j].width == 0);
      rbp_assert(sizeBucket[i][j].y % sizeBucket[i][j].width == 0);
      Rect with;
      if (intersectsWithFree(sizeBucket[i][j], with, i, j))
      {
        rbp_assert(0);
        return false;
      }
    }
  }
  return true;
}

bool PowerOfTwoBinPack::intersectsWithFree(const Rect &rect, Rect &with) const
{
  return intersectsWithFree(rect, with, -1, -1);
}

}