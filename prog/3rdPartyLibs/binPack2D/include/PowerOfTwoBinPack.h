//Copyright 2023 Gaijin Games KFT
//written in style of other binpack
#pragma once

#include <EASTL/vector.h>
#include <math/dag_adjpow2.h>
#include <debug/dag_assert.h>

#include "Rect.h"
namespace rbp {
#define RBP_DEBUG_OVERLAP 0

class PowerOfTwoBinPack
{
public:
  /// The initial bin size will be (0,0). Call Init to set the bin size.
  PowerOfTwoBinPack(){}

  /// Initializes a new bin of the given size.
  PowerOfTwoBinPack(int widthBuckets, int heightBuckets, int maxBucket) {init(widthBuckets, heightBuckets, maxBucket);}

  /// (Re)initializes the packer to an empty bin of width*maxBucketSize x height*maxBucketSize units. Call whenever
  /// you need to restart with a new bin.
  void init(int widthBuckets, int heightBuckets, int maxBucketSize);

  /// Inserts a single rectangle into the bin. The packer might rotate the rectangle, in which case the returned
  Rect insert(unsigned width)
  {
    int logSize = get_bigger_log2(width);
    Rect rect = insertLogSize(logSize);//width = 1<<logSize;
    G_ASSERTF((rect.x&((1<<logSize)-1)) == 0 && (rect.y&((1<<logSize)-1)) == 0, "%dx%d %d", rect.x, rect.y, rect.width);
    #if RBP_DEBUG_OVERLAP
    if (rect.width)
    {
      Rect with;
      rbp_assert(!intersectsWithFree(rect, with));
      allocated.push_back(rect);
    }
    #endif
    return rect;
  }

  //O(N), should be used for verification only!
  bool isAllocated(const Rect &rect)
  {
    #if !RBP_DEBUG_OVERLAP
      Rect with;
      return !intersectsWithFree(rect, with);
    #else
    if (rect.width)
    {
      for (int i = 0; i < allocated.size(); ++i)
        if (allocated[i].width == rect.width && allocated[i].x == rect.x && allocated[i].y == rect.y)
          return true;
      return false;
    }
    #endif
  }

  // add one of previously allocated rects to free. beware that it is actually one of allocated!
  void freeUsedRect(const Rect &rect);

  int freeSpaceArea() const
  {
    int freeArea = 0;
    for (int i = 0, e = sizeBucket.size(); i < e; ++i)
      freeArea += (int)sizeBucket[i].size()<<(i+i);
    return freeArea;
  }
  unsigned occupiedArea(unsigned initialArea) const {return (initialArea-freeSpaceArea());}
  float occupancy(unsigned initialArea) const {return occupiedArea(initialArea)/float(initialArea);}


  bool validate();//calls assert, and returns false if N^2 checks show not disjoint rects
  bool intersectsWithFree(const Rect &rect, Rect &with) const;
private:
  Rect insertLogSize(unsigned logSize);
  bool intersectsWithFree(const Rect &rect, Rect &with, int ci, int id) const;
  void freeUsedRectInternal(Rect rect, uint32_t logSize);
  /// Stores a list of rectangles that represents the free area of the bin. This rectangles in this list are disjoint.
  //from bigger to smaller size. Biggest one is never coalesced
  eastl::vector< eastl::vector<Rect> > sizeBucket;
  #if RBP_DEBUG_OVERLAP
  eastl::vector< Rect > allocated;
  #endif
  Rect popBucketUnsafe(int logSize)
  {
    rbp_assert(logSize < sizeBucket.size() && sizeBucket[logSize].size());
    Rect ret = sizeBucket[logSize][sizeBucket[logSize].size()-1];
    sizeBucket[logSize].pop_back();
    return ret;
  }
  static inline Rect emptyRect() {Rect empty; empty.x = empty.y = empty.width = empty.height = 0;return empty;}
  void splitBucket(int logSize)
  {
    rbp_assert(logSize > 0 && logSize < sizeBucket.size() && sizeBucket[logSize].size());
    Rect rect = popBucketUnsafe(logSize); rbp_assert(rect.width == rect.height);
    rect.width >>= 1; rect.height >>= 1;
    logSize--;
    sizeBucket[logSize].push_back(rect);
    rect.x += rect.width; sizeBucket[logSize].push_back(rect);
    rect.y += rect.width; sizeBucket[logSize].push_back(rect);
    rect.x -= rect.width; sizeBucket[logSize].push_back(rect);
  }
};
}
