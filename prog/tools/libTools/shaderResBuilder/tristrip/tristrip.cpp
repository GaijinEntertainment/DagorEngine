
#include "stripobj.h"
#include "tristrip.h"
#include <util/dag_globDef.h>
#include <memory/dag_regionMemAlloc.h>
using namespace TriStripifier;

////////////////////////////////////////////////////////////////////////////////////////
// private data
static unsigned int cacheSize = PublicStripifier::CACHESIZE_GEFORCE1_2;
static bool bStitchStrips = true;
static unsigned int minStripSize = 0;
static bool bListsOnly = true;
static unsigned int experimentsNum = 10;


////////////////////////////////////////////////////////////////////////////////////////
// SetListsOnly()
//
// If set to true, will return an optimized list, with no strips at all.
//
// Default value: false
//

void PublicStripifier::SetListsOnly(const bool _bListsOnly) { bListsOnly = _bListsOnly; }

////////////////////////////////////////////////////////////////////////////////////////
// SetExperimentsNumber()
//
// Sets number of experiments to run
//
// Default value: 10
//
void PublicStripifier::SetExperimentsNumber(const unsigned int _experimentsNum) { experimentsNum = _experimentsNum; }


////////////////////////////////////////////////////////////////////////////////////////
// SetCacheSize()
//
// Sets the cache size which the stripfier uses to optimize the data.
// Controls the length of the generated individual strips.
// This is the "actual" cache size, so 24 for GeForce3 and 16 for GeForce1/2
// You may want to play around with this number to tweak performance.
//
// Default value: 16
//
void PublicStripifier::SetCacheSize(const unsigned int _cacheSize)
{
  cacheSize = _cacheSize;
  if (cacheSize > CACHESIZE_MAXIMUM)
    cacheSize = CACHESIZE_MAXIMUM;
}


////////////////////////////////////////////////////////////////////////////////////////
// SetStitchStrips()
//
// bool to indicate whether to stitch together strips into one huge strip or not.
// If set to true, you'll get back one huge strip stitched together using degenerate
//  triangles.
// If set to false, you'll get back a large number of separate strips.
//
// Default value: true
//
void PublicStripifier::SetStitchStrips(const bool _bStitchStrips) { bStitchStrips = _bStitchStrips; }


////////////////////////////////////////////////////////////////////////////////////////
// SetMinStripSize()
//
// Sets the minimum acceptable size for a strip, in triangles.
// All strips generated which are shorter than this will be thrown into one big, separate list.
//
// Default value: 0
//
void PublicStripifier::SetMinStripSize(const unsigned int _minStripSize) { minStripSize = _minStripSize; }


static WordVec tempIndices;
static NvStripInfoVec tempStrips;
static NvFaceInfoVec tempFaces;
static NvStripifier stripifier;

namespace TriStripifier
{
IMemAlloc *stripmem = NULL;
};

void PublicStripifier::Init()
{
  if (!stripmem)
  {
    stripmem = new RegionMemAlloc(2 << 20, 1 << 20);
  }
}
void PublicStripifier::FinalCleanUp()
{
  if (auto *rm_alloc = static_cast<RegionMemAlloc *>(stripmem))
    debug_ctx("stripmem used %dK (of %dK allocated in %d pools)", rm_alloc->getPoolUsedSize() >> 10,
      rm_alloc->getPoolAllocatedSize() >> 10, rm_alloc->getPoolsCount());
  stripmem->destroy();
  stripmem = NULL;
}

static void CleanUp()
{
  // clean up everything

  // delete strips
  int i;
  for (i = 0; i < tempStrips.size(); i++)
  {
    for (int j = 0; j < tempStrips[i]->m_faces.size(); j++)
    {
      delete tempStrips[i]->m_faces[j];
      tempStrips[i]->m_faces[j] = NULL;
    }
    tempStrips[i]->m_faces.resize(0);
    delete tempStrips[i];
    tempStrips[i] = NULL;
  }

  // delete faces
  for (i = 0; i < tempFaces.size(); i++)
  {
    delete tempFaces[i];
    tempFaces[i] = NULL;
  }
}

////////////////////////////////////////////////////////////////////////////////////////
// GenerateStrips()
//
// in_indices: input index list, the indices you would use to render
// in_numIndices: number of entries in in_indices
// primGroups: array of optimized/stripified PrimitiveGroups
// numGroups: number of groups returned
//
// Be sure to call delete[] on the returned primGroups to avoid leaking mem
//
void PublicStripifier::GenerateStrips(const unsigned short *in_indices, const unsigned int in_numIndices, PrimitiveGroup **primGroups,
  unsigned short *numGroups)
{
  // put data in format that the stripifier likes
  if (bListsOnly)
  {
    *numGroups = 1;
    (*primGroups) = new PrimitiveGroup[*numGroups];
    OptimizeList(in_indices, in_numIndices, **primGroups);
    return;
  }
  Init();
  tempIndices.resize(in_numIndices);
  unsigned short maxIndex = 0;
  intptr_t i;
  for (i = 0; i < in_numIndices; i++)
  {
    tempIndices[i] = in_indices[i];
    if (in_indices[i] > maxIndex)
      maxIndex = in_indices[i];
  }

  // do actual stripification
  stripifier.Stripify(tempIndices, cacheSize, minStripSize, maxIndex, tempStrips, tempFaces);

  {
    // stitch strips together
    IntVec stripIndices;
    unsigned int numSeparateStrips = 0;

    stripifier.CreateStrips(tempStrips, stripIndices, bStitchStrips, numSeparateStrips, false, 0);

    // if we're stitching strips together, we better get back only one strip from CreateStrips()
    assert((bStitchStrips && (numSeparateStrips == 1)) || !bStitchStrips);

    // convert to output format
    *numGroups = numSeparateStrips; // for the strips
    if (tempFaces.size() != 0)
      (*numGroups)++; // we've got a list as well, increment
    (*primGroups) = new PrimitiveGroup[*numGroups];

    PrimitiveGroup *primGroupArray = *primGroups;

    // first, the strips
    intptr_t startingLoc = 0;
    for (int stripCtr = 0; stripCtr < numSeparateStrips; stripCtr++)
    {
      intptr_t stripLength = 0;

      if (!bStitchStrips)
      {
        // if we've got multiple strips, we need to figure out the correct length
        for (i = startingLoc; i < stripIndices.size(); i++)
        {
          if (stripIndices[i] == -1)
            break;
        }

        stripLength = i - startingLoc;
      }
      else
        stripLength = stripIndices.size();

      primGroupArray[stripCtr].type = PT_STRIP;
      primGroupArray[stripCtr].indices.resize(stripLength);

      int indexCtr = 0;
      for (int i = startingLoc; i < stripLength + startingLoc; i++)
        primGroupArray[stripCtr].indices[indexCtr++] = stripIndices[i];

      // we add 1 to account for the -1 separating strips
      // this doesn't break the stitched case since we'll exit the loop
      startingLoc += stripLength + 1;
    }

    // next, the list
    if (tempFaces.size() != 0)
    {
      int faceGroupLoc = (*numGroups) - 1; // the face group is the last one
      primGroupArray[faceGroupLoc].type = PT_LIST;
      primGroupArray[faceGroupLoc].indices.resize(tempFaces.size() * 3);
      int indexCtr = 0;
      for (int i = 0; i < tempFaces.size(); i++)
      {
        primGroupArray[faceGroupLoc].indices[indexCtr++] = tempFaces[i]->m_v0;
        primGroupArray[faceGroupLoc].indices[indexCtr++] = tempFaces[i]->m_v1;
        primGroupArray[faceGroupLoc].indices[indexCtr++] = tempFaces[i]->m_v2;
      }
    }
  }

  CleanUp();
}

void PublicStripifier::OptimizeList(const unsigned short *in_indices, const unsigned int in_numIndices, PrimitiveGroup &primGroup)
{
  // put data in format that the stripifier likes
  Init();
  tempIndices.resize(in_numIndices);
  unsigned short maxIndex = 0;
  int i;
  for (i = 0; i < in_numIndices; i++)
  {
    tempIndices[i] = in_indices[i];
    if (in_indices[i] > maxIndex)
      maxIndex = in_indices[i];
  }
  // do actual stripification
  stripifier.Stripify(tempIndices, cacheSize, minStripSize, maxIndex, tempStrips, tempFaces);

  // stitch strips together
  IntVec stripIndices;
  unsigned int numSeparateStrips = 0;

  // if we're outputting only lists, we're done
  PrimitiveGroup *primGroupArray = &primGroup;

  // count the total number of indices
  intptr_t numIndices = 0;
  for (i = 0; i < tempStrips.size(); i++)
  {
    numIndices += tempStrips[i]->m_faces.size() * 3;
  }

  // add in the list
  numIndices += tempFaces.size() * 3;

  primGroupArray[0].type = PT_LIST;
  primGroupArray[0].indices.resize(numIndices);
  // do strips
  unsigned int indexCtr = 0;
  for (i = 0; i < tempStrips.size(); i++)
  {
    for (int j = 0; j < tempStrips[i]->m_faces.size(); j++)
    {
      // degenerates are of no use with lists
      if (!NvStripifier::IsDegenerate(tempStrips[i]->m_faces[j]))
      {
        primGroupArray[0].indices[indexCtr++] = tempStrips[i]->m_faces[j]->m_v0;
        primGroupArray[0].indices[indexCtr++] = tempStrips[i]->m_faces[j]->m_v1;
        primGroupArray[0].indices[indexCtr++] = tempStrips[i]->m_faces[j]->m_v2;
      }
      else
      {
        // we've removed a tri, reduce the number of indices
        primGroupArray[0].indices.resize(primGroupArray[0].indices.size() - 3);
      }
    }
  }
  // do lists
  for (i = 0; i < tempFaces.size(); i++)
  {
    primGroupArray[0].indices[indexCtr++] = tempFaces[i]->m_v0;
    primGroupArray[0].indices[indexCtr++] = tempFaces[i]->m_v1;
    primGroupArray[0].indices[indexCtr++] = tempFaces[i]->m_v2;
  }

  // clean up everything
  CleanUp();
}


////////////////////////////////////////////////////////////////////////////////////////
// RemapIndices()
//
// Function to remap your indices to improve spatial locality in your vertex buffer.
//
// in_primGroups: array of PrimitiveGroups you want remapped
// numGroups: number of entries in in_primGroups
// numVerts: number of vertices in your vertex buffer, also can be thought of as the range
//  of acceptable values for indices in your primitive groups.
// remappedGroups: array of remapped PrimitiveGroups
//
// Note that, according to the remapping handed back to you, you must reorder your
//  vertex buffer.
//
void PublicStripifier::RemapIndices(const PrimitiveGroup *in_primGroups, const unsigned short numGroups,
  const unsigned short startVert, const unsigned short numVerts, int *const vertind, PrimitiveGroup *remappedGroups)
{
  // caches oldIndex --> newIndex conversion
  int *const indexCache = vertind;
  memset(indexCache, -1, sizeof(int) * numVerts);

  // loop over primitive groups
  unsigned int indexCtr = startVert;
  for (int i = 0; i < numGroups; i++)
  {
    unsigned int numIndices = in_primGroups[i].indices.size();

    // init remapped group
    (remappedGroups)[i].type = in_primGroups[i].type;
    (remappedGroups)[i].indices.resize(numIndices);

    for (int j = 0; j < numIndices; j++)
    {
      int in_index = in_primGroups[i].indices[j] - startVert;
      int cachedIndex = indexCache[in_index];
      if (cachedIndex == -1) // we haven't seen this index before
      {
        // point to "last" vertex in VB
        (remappedGroups)[i].indices[j] = indexCtr;

        // add to index cache, increment
        indexCache[in_index] = indexCtr++;
      }
      else
      {
        // we've seen this index before
        (remappedGroups)[i].indices[j] = cachedIndex;
      }
    }
  }

  // delete[] indexCache;
}

PublicStripifier basestripifier;
