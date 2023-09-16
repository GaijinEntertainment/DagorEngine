#ifndef __DAGOR_TRISTRIP_H__
#define __DAGOR_TRISTRIP_H__

#include <generic/dag_tab.h>

namespace TriStripifier
{

enum PrimType
{
  PT_LIST,
  PT_STRIP,
  PT_FAN
};

};

class PublicStripifier
{
public:
  // GeForce1 and 2 cache size
  static const int CACHESIZE_GEFORCE1_2 = 16;
  static const int CACHESIZE_GEFORCE3 = 24;
  static const int CACHESIZE_MAXIMUM = 1024;

  struct PrimitiveGroup
  {
    TriStripifier::PrimType type;
    Tab<unsigned short> indices;

    ////////////////////////////////////////////////////////////////////////////////////////

    PrimitiveGroup(IMemAlloc *m = tmpmem) : type(TriStripifier::PT_STRIP), indices(m) {}
    ~PrimitiveGroup() {}
  };

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
  static void SetCacheSize(const unsigned int cacheSize);

  // SetExperimentsNumber()
  //
  // Sets the number of experiments to run
  //
  // Default value: 10
  //
  static void SetExperimentsNumber(const unsigned int N_exper);

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
  static void SetStitchStrips(const bool bStitchStrips);


  ////////////////////////////////////////////////////////////////////////////////////////
  // SetMinStripSize()
  //
  // Sets the minimum acceptable size for a strip, in triangles.
  // All strips generated which are shorter than this will be thrown into one big, separate list.
  //
  // Default value: 0
  //
  static void SetMinStripSize(const unsigned int minSize);


  ////////////////////////////////////////////////////////////////////////////////////////
  // SetListsOnly()
  //
  // If set to true, will return an optimized list, with no strips at all.
  //
  // Default value: true
  //
  static void SetListsOnly(const bool bListsOnly);


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
  static void GenerateStrips(const unsigned short *in_indices, const unsigned int in_numIndices, PrimitiveGroup **primGroups,
    unsigned short *numGroups);

  static void OptimizeList(const unsigned short *in_indices, const unsigned int in_numIndices, PrimitiveGroup &primGroups);

  ////////////////////////////////////////////////////////////////////////////////////////
  // RemapIndices()
  //
  // Function to remap your indices to improve spatial locality in your vertex buffer.
  //
  // in_primGroups: array of PrimitiveGroups you want remapped
  // numGroups: number of entries in in_primGroups
  // numVerts: number of vertices in your vertex buffer, also can be thought of as the range
  //  of acceptable values for indices( [startVert,startVert+numVerts)) in your primitive groups.
  // remappedGroups: array of remapped PrimitiveGroups could be the same as input
  // vertind: array of vertmap must have numVerts the size
  //
  // Note that, according to the remapping handed back to you, you must reorder your
  //  vertex buffer(with vertind).
  //
  // Credit goes to the MS Xbox crew for the idea for this interface.
  //
  static void RemapIndices(const PrimitiveGroup *in_primGroups, const unsigned short numGroups, const unsigned short startVert,
    const unsigned short numVerts, int *const vertind, PrimitiveGroup *remappedGroups);

  // No need to call Init - it will be called automatically

  static void Init();

  // Call FinalCleanUp when you want to free memory
  static void FinalCleanUp();
};

extern PublicStripifier basestripifier;

#endif // __DAGOR_TRISTRIP_H__
