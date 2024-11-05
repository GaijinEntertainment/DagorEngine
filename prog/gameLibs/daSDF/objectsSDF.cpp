// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <perfMon/dag_statDrv.h>
#include <drv/3d/dag_rwResource.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_tex3d.h>
#include <3d/dag_resPtr.h>
#include <ioSys/dag_genIo.h>
#include <ioSys/dag_zstdIo.h>
#include <shaders/dag_postFxRenderer.h>
#include <shaders/dag_computeShaders.h>
#include <memory/dag_framemem.h>
#include <math/dag_Point3.h>
#include <math/integer/dag_IPoint3.h>
#include <math/integer/dag_IBBox3.h>
#include <math/dag_hlsl_floatx.h>
#include <osApiWrappers/dag_files.h>
#include <generic/dag_staticTab.h>
#include <generic/dag_carray.h>
#include <memory/dag_linearHeapAllocator.h>
#include <shaders/dag_linearSbufferAllocator.h>
#include <EASTL/string.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/bit.h>
#include <util/dag_fastNameMapTS.h>
#include <generic/dag_fixedVectorMap.h>
#include <generic/dag_fixedVectorSet.h>
#include <generic/dag_sort.h>
#include <util/dag_convar.h>
#include <hash/xxh3.h>
#include <daSDF/sparseSDFMip.h>
#include <daSDF/objects_sdf.hlsli>
#include <daSDF/objectsSDF.h>
#include <osApiWrappers/dag_threads.h>
#include <osApiWrappers/dag_events.h>
#include <osApiWrappers/dag_spinlock.h>

#define GLOBAL_VARS_LIST               \
  VAR(object_sdf_mips_indirection_lut) \
  VAR(sdf_inv_atlas_size_in_blocks)    \
  VAR(sdf_internal_block_size_tc)      \
  VAR(sdf_inv_atlas_size)              \
  VAR(globtm_psf_0)                    \
  VAR(globtm_psf_1)                    \
  VAR(globtm_psf_2)                    \
  VAR(globtm_psf_3)

#define VAR(a) static int a##VarId = -1;
GLOBAL_VARS_LIST
#undef VAR

// static inline int volume(const IPoint3 &v) {return v.x*v.y*v.z;}
static IPoint3 getIndirectionDim(const SparseSDFMip &mi)
{
  IPoint3 dim;
  mi.getIndirectionDim(dim.x, dim.y, dim.z);
  return dim;
}

class MippedMeshSDFRef
{
public:
  Point3 boundsCenter() const { return center; }
  Point3 boundsWidth() const { return ext * (maxExt + maxExt); }
  // Point3 getScale() const {return div(boundsWidth(), ext*2);}
  float getScale() const { return maxExt; }
  Point3 getExt() const { return ext; }

  void setBounds(const BBox3 &localBounds)
  {
    ext = localBounds.width() * .5f;
    maxExt = max(max(ext.x, ext.y), max(1e-6f, ext.z));
    ext = ext / maxExt;
    center = localBounds.center();
  }

  Point3 center, ext;
  float maxExt = 0;
  // BBox3 localBounds;
  uint16_t fileRef = 0;
  uint8_t mipCountTwoSided = 0;
  uint8_t mipCount() const { return mipCountTwoSided & 0x3; }
  bool mostlyTwoSided() const { return bool(mipCountTwoSided >> 1); }

  carray<SparseSDFMip, SDF_NUM_MIPS> mipInfo;
};

struct PackedObjectMipCompare
{
  bool operator()(const PackedObjectMip &a, const PackedObjectMip &b) const { return a.maxDist__object_id < b.maxDist__object_id; }
};

static PackedObjectMip pack_sdf_object_mip(uint32_t oi, const SparseSDFMip &mi, const Point3 &extent, uint32_t indirOffset)
{
  PackedObjectMip packedMip;
  packedMip.indirOffset = indirOffset;
  packedMip.indirectionDim = mi.indirectionDim;
  packedMip.maxDist__object_id = mi.maxEncodedDistFP16 | (oi << 16);
  packedMip.toIndirMul = mi.toIndirMul;
  packedMip.toIndirAdd = mi.toIndirAdd;
  packedMip.extent = extent;
  return packedMip;
}

static PackedObjectMip empty_sdf_object_mip(uint32_t oi)
{
  PackedObjectMip packedMip;
  packedMip.indirOffset = 0;
  packedMip.indirectionDim = 1 | (1 << SDF_MAX_INDIRECTION_DIM_BITS) | (1 << (2 * SDF_MAX_INDIRECTION_DIM_BITS));
  packedMip.toIndirMul = float3(0, 0, 0);
  packedMip.toIndirAdd = float3(0, 0, 0);
  packedMip.extent = float3(0, 0, 0);
  constexpr uint32_t largestHalf = 0x7bff; // 65504, largest normal half number
  packedMip.maxDist__object_id = (oi << 16) | largestHalf;
  // packedMip.maxDist = packedMip.decodeMulAdd.x + packedMip.decodeMulAdd.y;
  // to prevent missing objects located between texels
  // packedMip.extent = Point3(0,0,0);
  return packedMip;
}

struct ObjectsAtlasLru;
struct SDFStreamingThread : public DaThread
{
  ObjectsAtlasLru &lru;

  SDFStreamingThread(ObjectsAtlasLru &lru_) :
    lru(lru_),
    DaThread("sdf_streaming_thread", DEFAULT_STACK_SZ, cpujobs::DEFAULT_THREAD_PRIORITY + cpujobs::THREAD_PRIORITY_LOWER_STEP,
      WORKER_THREADS_AFFINITY_MASK)
  {}

  void execute() override;
  ~SDFStreamingThread();
};

struct OsEvent
{
  os_event_t event;
  OsEvent(const OsEvent &) = delete;
  OsEvent &operator=(const OsEvent &) = delete;
  OsEvent() { os_event_create(&event, "sdf_loading_queue"); }
  ~OsEvent()
  {
    os_event_destroy(&event);
    debug("deleted event");
  }
  int signal() { return os_event_set(&event); }
  int wait(unsigned ms = OS_WAIT_INFINITE) { return os_event_wait(&event, ms); }
};

struct StreamedMip
{
  uint16_t id, mip; // we have 32 bits left in padding
  eastl::unique_ptr<uint8_t[]> decodedData;
};
namespace dag
{
template <>
struct is_type_relocatable<StreamedMip>
{
  static constexpr bool value = true;
};
}; // namespace dag

struct ObjectsAtlasLru
{
  typedef LinearHeapAllocator<NullHeapManager> AtlasAllocator;
  OsEvent queueEvent;
  SDFStreamingThread loadingThread; // has to be destroyed before queueEvent, so has to be declared later!
  LinearHeapAllocatorSbuffer::RegionId invalidIndirection;
  struct AtlasObjectMipInfo
  {
    dag::RelocatableFixedVector<AtlasAllocator::RegionId, 3> blocks;
    LinearHeapAllocatorSbuffer::RegionId indirection;
    // todo: remove padding here!
  };
  struct AtlasObjectInfo
  {
    // todo: probably better to have one RelocatableFixedVector for blocks of 9 predefined size, than 3*3.
    carray<AtlasObjectMipInfo, 3> mips;
  };

  // AoS, tuple_vector
  typedef uint16_t lru_time_t;
  dag::Vector<MippedMeshSDFRef> objectsRef;
  typedef uint8_t encoded_type_t;
  dag::Vector<encoded_type_t> currentObjectMipsEncoded;
  dag::Vector<lru_time_t> objectsLRU;
  dag::Vector<AtlasObjectInfo> atlasObjects;
  // AoS, tuple_vector

  lru_time_t lastLRUTick = 0;
  uint32_t currentBlocksUsed = 0;
  int atlas_bw = 0, atlas_bh = 0, atlas_bd = 0; // in blocks
  uint32_t maxBlocksSpace = 0;
  uint32_t currentMaxObjectsCnt = 0;
  UniqueTexHolder object_sdf_mip_atlas;
  FastNameMapTS<> files;
  UniqueBufHolder object_sdf_mips;
  uint16_t addFileRefSafe(const char *fileName)
  {
    auto id = files.addNameId(fileName);
    G_ASSERT(uint32_t(id) <= StreamMipCommand::max_file_ref);
    return uint16_t(id);
  }
  const char *getFileNameSafe(uint16_t id) const { return files.getName(id); }

  LinearHeapAllocatorSbuffer indirectionAllocator;
  uint32_t lastUpdatedIndirectionGeneration = ~0u;
  AtlasAllocator atlasManager;

  ObjectsAtlasLru() :
    indirectionAllocator(
      SbufferHeapManager("sdf_indirection", sizeof(uint32_t), SBCF_MISC_ALLOW_RAW | SBCF_BIND_SHADER_RES | SBCF_BIND_UNORDERED)),
    loadingThread(*this)
  {
    loadingThread.start();
  }

  void initAtlas()
  {
    // indirection can and should be bigger than fully allocated (due to invalid indirection)
    maxBlocksSpace = atlas_bw * atlas_bh * atlas_bd;

    indirectionAllocator.clear();
    indirectionAllocator.reserve(maxBlocksSpace * sizeof(uint32_t) * 4);
    invalidIndirection = indirectionAllocator.allocateInHeap(sizeof(uint32_t));
    auto invalidIndirectionRegion = indirectionAllocator.get(invalidIndirection);
    G_UNUSED(invalidIndirectionRegion);
    G_ASSERT(invalidIndirectionRegion.offset == 0 && invalidIndirectionRegion.size == 4);
    lastUpdatedIndirectionGeneration = indirectionAllocator.getManager().getHeapGeneration() - 1;
    atlasManager = AtlasAllocator();
    atlasManager.resize(maxBlocksSpace);
    currentBlocksUsed = 0;
    for (auto &atlasObject : atlasObjects)
      for (auto &mip : atlasObject.mips)
      {
        mip.blocks.clear();
        mip.indirection = LinearHeapAllocatorSbuffer::RegionId();
      }
    for (int oid = 0; oid < currentObjectMipsEncoded.size(); ++oid)
      updateBestMip(oid, SDF_NUM_MIPS);
  }
  uint32_t addObject(MippedMeshSDFRef &&meshSDF)
  {
    const uint32_t ind = objectsRef.size();
    objectsRef.push_back(eastl::move(meshSDF));
    atlasObjects.push_back();
    objectsLRU.push_back(0);
    currentObjectMipsEncoded.push_back(encodeEmptyMipCount(meshSDF.mipCount()));
    return ind;
  }

  void initAtlasObjects(uint32_t objects)
  {
    currentMaxObjectsCnt = objects;
    atlasObjects.clear();
    atlasObjects.reserve(objects);
    objectsRef.clear();
    objectsRef.reserve(objects);
    objectsLRU.clear();
    objectsLRU.push_back(objects);
    currentObjectMipsEncoded.clear();
    currentObjectMipsEncoded.reserve(objects);
    object_sdf_mips.close();
    object_sdf_mips = dag::create_sbuffer(sizeof(float) * 4, currentMaxObjectsCnt * SDF_PACKED_MIP_SIZE,
      SBCF_MISC_STRUCTURED | SBCF_BIND_SHADER_RES | SBCF_BIND_UNORDERED, 0, "object_sdf_mips");
  }
  int size() const { return objectsRef.size(); }
  void updateLRUtick()
  {
    if (++lastLRUTick == 0)
      lruTickOverflow();
  }

  DAGOR_NOINLINE
  void lruTickOverflow()
  {
    G_ASSERT(lastLRUTick == 0);
    constexpr int lastLruTick = ~lru_time_t(0);
    constexpr int minLruTick = 1024;
    lastLRUTick = minLruTick;
    G_UNUSED(lastLruTick);
    for (auto &cTime : objectsLRU) // preserve some history. Update will happen each 65536-minLruTick = 64535 ticks
      cTime = clamp<int>(int(cTime) - lastLruTick + minLruTick, 0, minLruTick);
  }
  enum
  {
    SDF_MIPS_BITS = 2,
    SDF_MIPS_MASK = (1 << SDF_MIPS_BITS) - 1,
  };

  struct StreamMipCommand
  {
    uint16_t id, mip__fileRef;
    static constexpr uint32_t max_file_ref = (1 << (sizeof(mip__fileRef) * 8 - SDF_MIPS_BITS)) - 1;
    uint32_t streamOffset, compressedSize, decompressedSize;
    static uint16_t make_mip_fileRef(uint8_t mip, uint16_t fileRef) { return (fileRef << SDF_MIPS_BITS) | mip; }
    uint16_t getMip() const { return mip__fileRef & SDF_MIPS_MASK; }
    uint16_t getFileRef() const { return mip__fileRef >> SDF_MIPS_BITS; }
  };
  dag::RelocatableFixedVector<StreamMipCommand, 7> streamMipsCommandBuffer;
  OSSpinlock streamMipsCommandBufferLock;

  typedef dag::RelocatableFixedVector<StreamedMip, 7> streamed_mips_queue_t;
  streamed_mips_queue_t streamedMipsQueue;
  OSSpinlock streamedMipsQueueLock;

  struct GpuUpdateCommands
  {
    eastl::vector_set<PackedObjectMip, PackedObjectMipCompare, EASTLAllocatorType, dag::Vector<PackedObjectMip, EASTLAllocatorType>>
      objectInfoCommands;
    struct IndirectionCommandBlock
    {
      dag::RelocatableFixedVector<IndirectionCommand, 1> commands;
      dag::RelocatableFixedVector<uint32_t, 7> indirections;
    };
    dag::Vector<IndirectionCommandBlock> indirectionCommands;
    struct BlockCommand
    {
      uint32_t to;
      char data[SDF_BLOCK_VOLUME];
    };
    dag::Vector<BlockCommand> blocksCommands; // block + posititon in atlas
    void clear()
    {
      blocksCommands.clear();
      indirectionCommands.clear();
      objectInfoCommands.clear();
    }
    bool empty() const { return blocksCommands.empty() && indirectionCommands.empty() && objectInfoCommands.empty(); }
  };

  enum
  {
    LOADING_MIPS_SHIFT = SDF_MIPS_BITS,
    LOADING_MIPS_BITS = SDF_NUM_MIPS,
    LOADING_MIPS_MASK = (1 << LOADING_MIPS_BITS) - 1,
    BEST_MIP_MASK = SDF_MIPS_MASK,
    TOTAL_MIP_COUNT_SHIFT = LOADING_MIPS_SHIFT + LOADING_MIPS_BITS
  };
  void updateBestMip(uint16_t oid, uint8_t mip)
  {
    G_STATIC_ASSERT(SDF_MIPS_MASK >= SDF_NUM_MIPS);
    G_STATIC_ASSERT(BEST_MIP_MASK >= SDF_NUM_MIPS);
    G_STATIC_ASSERT((encoded_type_t(~0) >> TOTAL_MIP_COUNT_SHIFT) >= SDF_NUM_MIPS); // enough bits
    auto &e = currentObjectMipsEncoded[oid];
    e = (e & ~BEST_MIP_MASK) | mip;
  }
  static encoded_type_t loading_mask(uint8_t mip) { return 1 << (mip + LOADING_MIPS_SHIFT); }
  void removeLoadingMip(uint16_t oid, uint8_t mip) { currentObjectMipsEncoded[oid] &= ~loading_mask(mip); }
  static uint8_t encodeEmptyMipCount(uint8_t mipCount) { return SDF_NUM_MIPS | (mipCount << TOTAL_MIP_COUNT_SHIFT); }
  uint8_t getBestLoadedMip(uint16_t oid) const { return currentObjectMipsEncoded[oid] & BEST_MIP_MASK; }
  void startLoading(uint16_t oid, uint8_t needed_mip)
  {
    const encoded_type_t mask = loading_mask(needed_mip);
    G_ASSERT(needed_mip < LOADING_MIPS_BITS && (currentObjectMipsEncoded[oid] & mask) == 0);
    currentObjectMipsEncoded[oid] |= mask;
  }
  uint8_t loadingMipsMask(uint16_t oid) const { return (currentObjectMipsEncoded[oid] >> LOADING_MIPS_SHIFT) & LOADING_MIPS_MASK; }
  bool needToLoadBetterMip(uint16_t oid, uint8_t needed_mip) const
  {
    const encoded_type_t loadedMips = (currentObjectMipsEncoded[oid] >> LOADING_MIPS_SHIFT) & LOADING_MIPS_MASK;
    const encoded_type_t betterMipsMask = (1 << (needed_mip + 1)) - 1;
    return (betterMipsMask & loadedMips) == 0;
  }
  uint8_t getTotalMipCount(uint16_t oid) const { return currentObjectMipsEncoded[oid] >> TOTAL_MIP_COUNT_SHIFT; }

  void scheduleUpdateObjectInfoBase(uint16_t oid, uint8_t mip, GpuUpdateCommands &commands) const
  {
    const AtlasObjectInfo &oi = atlasObjects[oid];
    auto &destMipInfo = oi.mips[mip];
    G_ASSERT(destMipInfo.indirection);
    auto indirectionRegion = indirectionAllocator.get(destMipInfo.indirection);
    auto &meshSDF = objectsRef[oid];

    auto packedMip = pack_sdf_object_mip(oid, meshSDF.mipInfo[mip], meshSDF.getExt(), indirectionRegion.offset);
    commands.objectInfoCommands.emplace(packedMip);
  }

  void scheduleUpdateObjectInfo(uint16_t oid, uint8_t mip, GpuUpdateCommands &commands)
  {
    scheduleUpdateObjectInfoBase(oid, mip, commands);
    updateBestMip(oid, mip);
  }
  void scheduleUpdateObjectFreeInfo(uint16_t oid, GpuUpdateCommands &commands)
  {
    auto packedMip = empty_sdf_object_mip(oid);
    commands.objectInfoCommands.emplace(packedMip);
    updateBestMip(oid, SDF_NUM_MIPS);
  }
  void increaseIndirection(uint32_t additional_size, GpuUpdateCommands &commands)
  {
    logwarn("we had to increase indirection size from %d, biggest %d (+%d)", indirectionAllocator.getHeapSize(),
      indirectionAllocator.biggestContigousChunkLeft(), additional_size);
    uint32_t nextSize = indirectionAllocator.getHeapSize();
    if (indirectionAllocator.freeMemLeft() >= indirectionAllocator.getHeapSize() / 2)
    {
      logwarn("just defragment");
    }
    else
    {
      static constexpr uint32_t indirection_page_size = 65536;
      if (nextSize < (2 << 20))
        nextSize = max(nextSize + additional_size, nextSize + nextSize);
      else
        nextSize = max(nextSize + additional_size, nextSize + nextSize / 2);
      nextSize = (nextSize + indirection_page_size - 1) & ~(indirection_page_size - 1);
    }
    Tab<uint32_t> oldOffsets(framemem_ptr());
    oldOffsets.resize(size());
    for (uint16_t oi = 0, oe = size(); oi < oe; ++oi)
    {
      auto mip = getBestLoadedMip(oi);
      if (mip < getTotalMipCount(oi))
        oldOffsets[oi] = indirectionAllocator.get(atlasObjects[oi].mips[mip].indirection).offset;
      else
        oldOffsets[oi] = 0u;
    }
    indirectionAllocator.resize(nextSize);
    auto invalidIndirectionRegion = indirectionAllocator.get(invalidIndirection);
    G_UNUSED(invalidIndirectionRegion);
    G_ASSERT(invalidIndirectionRegion.offset == 0 && invalidIndirectionRegion.size == 4);
    // check if offsetRegion differs, and if it is - update mipinfo again
    // todo: we actually need to update ONLY lut offset! we can make explicit command for that (much smaller)
    // but it has to keep consistency with mip update command (i.e. we don't update it then back to incorrect)
    // so, if commands ALREADY have mipInfo update for oi - we change offset in it, and if not, we can go with smaller command
    // but than we would need to check mipInfo commands same way
    // too much of a hassle, I keep it as is
    for (uint16_t oi = 0, oe = size(); oi < oe; ++oi)
    {
      if (oldOffsets[oi])
      {
        auto mip = getBestLoadedMip(oi);
        if (indirectionAllocator.get(atlasObjects[oi].mips[mip].indirection).offset != oldOffsets[oi])
          scheduleUpdateObjectInfoBase(oi, mip, commands);
      }
    }
  }
  bool scheduleUpdate(const StreamedMip &streamed_mip, GpuUpdateCommands &commands)
  {
    debug("schedule update %d:%d", streamed_mip.id, streamed_mip.mip);
    G_ASSERT(streamed_mip.mip < getBestLoadedMip(streamed_mip.id));
    AtlasObjectInfo &oi = atlasObjects[streamed_mip.id];
    auto &destMipInfo = oi.mips[streamed_mip.mip];
    G_ASSERTF(destMipInfo.blocks.empty() && !destMipInfo.indirection, "%d: %d", streamed_mip.id, streamed_mip.mip);
    auto &oRef = objectsRef[streamed_mip.id];
    auto &mi = oRef.mipInfo[streamed_mip.mip];
    if (currentBlocksUsed + mi.sdfBlocksCount > maxBlocksSpace) // not enough space to fit
      return false;

    const uint32_t indirSize = mi.getVolume();

    const uint8_t *curBlocks = streamed_mip.decodedData.get() + indirSize * sizeof(uint32_t);
    // G_ASSERT(streamed_mip.decodedData.size() == indirSize*sizeof(uint32_t) + mi.sdfBlocksCount*SDF_BLOCK_VOLUME);
    Tab<uint32_t> blocksRemapped(framemem_ptr());
    blocksRemapped.resize(mi.sdfBlocksCount);
    if (!indirectionAllocator.canAllocate(indirSize * sizeof(uint32_t)))
      increaseIndirection(indirSize * sizeof(uint32_t), commands);
    destMipInfo.indirection = indirectionAllocator.allocateInHeap(indirSize * sizeof(uint32_t));
    G_ASSERT(destMipInfo.indirection);
    auto indirectionRegion = indirectionAllocator.get(destMipInfo.indirection);

    const uint32_t blocks_slice = (atlas_bw * atlas_bh);
    for (int blocksAllocated = 0; blocksAllocated < mi.sdfBlocksCount;)
    {
      const uint32_t blockSize = min<uint32_t>(atlasManager.biggestContigousChunkLeft(), mi.sdfBlocksCount - blocksAllocated);
      auto allocated = atlasManager.allocateInHeap(blockSize);
      if (!allocated)
      {
        logerr("can't allocate blocks in manager blockSize = %d, contiguous %d total %d - current %d", blockSize,
          atlasManager.biggestContigousChunkLeft(), mi.sdfBlocksCount, blocksAllocated);
      }
      G_ASSERT(allocated);
      destMipInfo.blocks.push_back(allocated);
      auto blocksRegion = atlasManager.get(allocated);
      G_ASSERT(blocksRegion.offset + blocksRegion.size <= maxBlocksSpace);
      for (uint32_t bi = 0; bi < blocksRegion.size; ++bi)
      {
        const uint32_t to = blocksRegion.offset + bi;
        // can be replaced with bit shifts if atlas_bw & atlas_bh is pow-of-2
        //  also we can perform arithmetics in shader
        const uint32_t ind = (to % atlas_bw) | (((to / atlas_bw) % atlas_bh) << SDF_INDIRECTION_BLOCK_SHIFT_XY) |
                             ((to / blocks_slice) << (2 * SDF_INDIRECTION_BLOCK_SHIFT_XY));
        blocksRemapped[blocksAllocated + bi] = ind;
        auto &blockCommand = commands.blocksCommands.push_back();
        blockCommand.to = to;
        memcpy(blockCommand.data, curBlocks, SDF_BLOCK_VOLUME);
        curBlocks += SDF_BLOCK_VOLUME;
      }
      blocksAllocated += blocksRegion.size;
      G_ASSERT(blocksAllocated <= mi.sdfBlocksCount);
    }
    currentBlocksUsed += mi.sdfBlocksCount;

    const uint32_t *lutIndir = (const uint32_t *)streamed_mip.decodedData.get();
    auto &indirCommands = commands.indirectionCommands;
    if (indirCommands.empty() || indirCommands.back().indirections.size() + indirSize > indirectionsUpdateBufSize ||
        indirCommands.back().commands.size() + 1 > indirectionUpdateCommandsBufSize)
      indirCommands.push_back();

    auto &commandBlock = indirCommands.back();


    uint32_t indirSourceAt = commandBlock.indirections.size();
    auto destLutIndir = commandBlock.indirections.append_default(indirSize);
    G_ASSERT(indirectionRegion.size == indirSize * 4);

    uint32_t lI = 0;
    for (auto e = lutIndir + indirSize; lutIndir != e; lutIndir++, destLutIndir++, ++lI)
    {
      const uint32_t lutInd = *lutIndir;
      G_ASSERT(lutInd == SDF_INVALID_LUT_INDEX || lutInd < mi.sdfBlocksCount);
      *destLutIndir = (lutInd == SDF_INVALID_LUT_INDEX) ? SDF_INVALID_LUT_INDEX : blocksRemapped[lutInd];
    }
    commandBlock.commands.push_back(
      IndirectionCommand{indirectionRegion.offset, indirSourceAt * uint32_t(sizeof(uint32_t)), indirectionRegion.size});
    scheduleUpdateObjectInfo(streamed_mip.id, streamed_mip.mip, commands);
    return true;
  }
  bool freeEnough(const uint16_t *to_remove, uint32_t to_remove_cnt, const uint8_t *new_mips, uint32_t new_mips_cnt,
    uint32_t blocks_needed, GpuUpdateCommands &commands)
  {
    if (currentBlocksUsed + blocks_needed <= maxBlocksSpace)
      return true;
    for (auto r = to_remove, re = to_remove + to_remove_cnt; r != re; ++r)
    {
      auto oid = *r;
      const uint8_t currentMip = getBestLoadedMip(oid);
      const uint8_t neededMip = getClampedMip(oid, new_mips, new_mips_cnt);
      if (currentMip < neededMip)
      {
        freeObject(oid, neededMip, commands);
        if (currentBlocksUsed + blocks_needed <= maxBlocksSpace)
          return true;
      }
    }
    return false;
  }
  bool loadStreamedData(const uint8_t *new_mips, uint32_t new_mips_cnt, streamed_mips_queue_t &streamedMips,
    uint32_t streamedMipsBlocks, GpuUpdateCommands &commands)
  {
    debug("load %d", streamedMips.size());
    const bool enoughSpaceToLoadAll = currentBlocksUsed + streamedMipsBlocks <= maxBlocksSpace;
    // we can also limit amount of time spent in updating data
    bool updatedAll = true;
    for (auto &streamedMip : streamedMips)
    {
      removeLoadingMip(streamedMip.id, streamedMip.mip); // remove loading bit anyway. this is basically destructor of streamedMip
      auto oid = streamedMip.id;
      const uint8_t currentMipForStreamedObject = getBestLoadedMip(oid);
      if (currentMipForStreamedObject <= streamedMip.mip) // we already have finer mip loaded
        continue;
      const uint8_t neededMipForStreamedObject = getClampedMip(oid, new_mips, new_mips_cnt);
      if (neededMipForStreamedObject <= streamedMip.mip || enoughSpaceToLoadAll)
      {
        // Option 1.
        //  neededMipForStreamedObject is same or finer than just streamed object. We need it!
        // Option 2.
        //  we don't need such fine quality, however, currently loaded mip is even worser (mip 2), than required
        //  this mean we do have in upcoming pipeline some worser quality (mip 1)
        //  but there is enough space, so we better load this one anyway, cause it allows us to render everything
        updatedAll &= scheduleUpdate(streamedMip, commands);
      }
    }
    streamedMips.clear();
    if (!updatedAll)
    {
      logerr("hadn't loadded all required data, not enough space");
    }
    return updatedAll;
  }
  bool processStreamedData(uint16_t *to_remove, uint32_t to_remove_cnt, const uint8_t *new_mips, uint32_t new_mips_cnt,
    streamed_mips_queue_t &streamedMips, uint32_t streamedMipsBlocks, GpuUpdateCommands &commands)
  {
    debug("streamed %d", streamedMips.size());
    if (currentBlocksUsed + streamedMipsBlocks > maxBlocksSpace)
    {
      // sort with lru, and remove just enough to fit
      stlsort::sort(to_remove, to_remove + to_remove_cnt, [&](uint16_t a, uint16_t b) { return objectsLRU[a] < objectsLRU[b]; });
      freeEnough(to_remove, to_remove_cnt, new_mips, new_mips_cnt, streamedMipsBlocks, commands);
    }

    const bool enoughSpaceToLoadAll = currentBlocksUsed + streamedMipsBlocks <= maxBlocksSpace;

    if (!enoughSpaceToLoadAll)
    {
      uint32_t requiredBlocks = 0;
      for (auto &streamedMip : streamedMips)
      {
        auto oid = streamedMip.id;
        const uint8_t currentMipForStreamedObject = getBestLoadedMip(oid);
        if (currentMipForStreamedObject <= streamedMip.mip) // we already have finer mip loaded
          continue;
        const uint8_t neededMipForStreamedObject = getClampedMip(oid, new_mips, new_mips_cnt);
        if (neededMipForStreamedObject <= streamedMip.mip)
          requiredBlocks += getBlocksNeeded(oid, streamedMip.mip);
      }
      if (currentBlocksUsed + requiredBlocks > maxBlocksSpace)
        stlsort::sort(streamedMips.begin(), streamedMips.end(), [&](auto &a, auto &b) { return a.mip > b.mip; });
    }
    return loadStreamedData(new_mips, new_mips_cnt, streamedMips, streamedMipsBlocks, commands);
  }

  // todo:
  // gpu commands are not really optimal, as we do several memory copies.
  // one in scheduleUpdate to command
  // one in submitGPUCommands in updateData
  // this allows us to perform everything besides actual dispatches in not main thread (even updateData can be done in other thread)
  // however, we don't use it now, so if there is enough space in dest buffer we can do just one (with lock discard), saving one memcpy
  UniqueBufHolder indirectionsUpdateBuf, indirectionCommandsBuf, objectInfoCommandsBuf, blocksCommandsBuf;
  enum
  {
    DEFAULT_INDIR_COMMAND_SIZE = 1024,
    DEFAULT_INDIRECTIONS_SIZE = 4096
  };
  static constexpr uint32_t page_size = 4096;
  uint32_t updateInfoBufSize = 0, updateBlocksBufSize = 0, indirectionsUpdateBufSize = DEFAULT_INDIRECTIONS_SIZE,
           indirectionUpdateCommandsBufSize = DEFAULT_INDIR_COMMAND_SIZE;
  eastl::unique_ptr<ComputeShaderElement> sdf_indirection_update_cs, sdf_info_update_cs, sdf_blocks_update_cs, sdf_info_clear_cs;

  void allocateIndirectionsSize(uint32_t in_sz)
  {
    if (indirectionsUpdateBufSize != in_sz)
      indirectionsUpdateBuf.close();

    // todo : SBCF_FRAMEMEM
    if (!indirectionsUpdateBuf)
      indirectionsUpdateBuf = dag::create_sbuffer(sizeof(uint32_t), indirectionsUpdateBufSize = in_sz,
        SBCF_MISC_ALLOW_RAW | SBCF_BIND_SHADER_RES, 0, "sdf_indirection_update_buf");
  }
  void allocateGpuUpdateCommandsBufs(uint32_t info_sz, uint32_t blocks_sz, uint32_t in_commands_sz, uint32_t in_sz)
  {
    // todo : SBCF_FRAMEMEM
    if (updateInfoBufSize != info_sz)
      objectInfoCommandsBuf.close();
    if (!objectInfoCommandsBuf)
      objectInfoCommandsBuf = dag::create_sbuffer(sizeof(PackedObjectMip), updateInfoBufSize = info_sz,
        SBCF_MISC_STRUCTURED | SBCF_BIND_SHADER_RES, 0, "sdf_info_update_commands_buf");
    if (updateBlocksBufSize != blocks_sz)
      blocksCommandsBuf.close();
    if (!blocksCommandsBuf)
      blocksCommandsBuf = dag::create_sbuffer(sizeof(uint32_t),
        (updateBlocksBufSize = blocks_sz) * sizeof(GpuUpdateCommands::BlockCommand) / sizeof(uint32_t),
        SBCF_MISC_ALLOW_RAW | SBCF_BIND_SHADER_RES, 0, "sdf_block_update_commands_buf");
    if (indirectionUpdateCommandsBufSize != in_commands_sz)
      indirectionCommandsBuf.close();
    if (!indirectionCommandsBuf)
      indirectionCommandsBuf = dag::create_sbuffer(sizeof(IndirectionCommand), indirectionUpdateCommandsBufSize = in_commands_sz,
        SBCF_MISC_STRUCTURED | SBCF_BIND_SHADER_RES, 0, "sdf_indirection_update_commands_buf");
    allocateIndirectionsSize(in_sz);
  }
  void initGpuUpdateCommands()
  {
    sdf_indirection_update_cs.reset(new_compute_shader("sdf_indirection_update_cs"));
    sdf_info_update_cs.reset(new_compute_shader("sdf_info_update_cs"));
    sdf_blocks_update_cs.reset(new_compute_shader("sdf_blocks_update_cs"));
    sdf_info_clear_cs.reset(new_compute_shader("sdf_info_clear_cs"));
    allocateGpuUpdateCommandsBufs(4096 / sizeof(PackedObjectMip), 131072 / sizeof(GpuUpdateCommands::BlockCommand),
      DEFAULT_INDIR_COMMAND_SIZE, DEFAULT_INDIRECTIONS_SIZE);
  }

  void submitGPUCommands(const GpuUpdateCommands &commands)
  {
    ShaderGlobal::set_buffer(object_sdf_mips_indirection_lutVarId, indirectionAllocator.getHeap().getBufId());
    // todo: analyze buffer sizes, so we fit in p99
    if (!objectInfoCommandsBuf)
    {
      initGpuUpdateCommands();
      ShaderGlobal::set_int(get_shader_variable_id("object_sdf_mips_count"), objectsRef.size());
      d3d::set_rwbuffer(STAGE_CS, 0, object_sdf_mips.getBuf());
      sdf_info_clear_cs->dispatchThreads(objectsRef.size(), 1, 1);
      d3d::set_rwbuffer(STAGE_CS, 0, nullptr);
    }

    G_ASSERT(sdf_indirection_update_cs);
#if DAGOR_DBGLEVEL > 0
    for (auto &indirCmd : commands.indirectionCommands)
    {
      for (uint32_t j = 0, end = indirCmd.commands.size(); j < end; ++j)
      {
        auto &c1 = indirCmd.commands[j];
        auto b1 = c1.destPosition, e1 = c1.destPosition + c1.count - 1;
        for (uint32_t i = j + 1; i < end; ++i)
        {
          auto &c2 = indirCmd.commands[i];
          auto b2 = c2.destPosition, e2 = c2.destPosition + c2.count - 1;
          if (b1 <= e2 && e1 >= b2)
          {
            logerr("intersecting indirection update, %d [%d, %d] with %d [%d, %d] ", j, b1, e1, i, b2, e2);
          }
        }
      }
    }
#endif
    d3d::set_rwbuffer(STAGE_CS, 0, indirectionAllocator.getHeap().getBuf());
    // todo: analyze indirections buffer allocation
    uint32_t totalIndirections = 0, totalInCommands = 0;
    for (auto &indirCmd : commands.indirectionCommands)
    {
      if (indirectionsUpdateBufSize < indirCmd.indirections.size())
        allocateIndirectionsSize(
          (indirCmd.indirections.size() + page_size / sizeof(uint32_t) - 1) & ~(page_size / sizeof(uint32_t) - 1));
      indirectionsUpdateBuf->updateData(0, indirCmd.indirections.size() * sizeof(uint32_t), indirCmd.indirections.data(),
        VBLOCK_WRITEONLY);
      totalIndirections += indirCmd.indirections.size();
      totalInCommands += indirCmd.commands.size();
      for (uint32_t copied = 0, end = indirCmd.commands.size(); copied < end;)
      {
        const uint32_t currentSize = min<uint32_t>(end - copied, indirectionUpdateCommandsBufSize);
        ShaderGlobal::set_int(get_shader_variable_id("sdf_indirection_update_commands_buf_size"), currentSize);
        indirectionCommandsBuf->updateData(0, currentSize * sizeof(IndirectionCommand), indirCmd.commands.data() + copied,
          VBLOCK_WRITEONLY);
        sdf_indirection_update_cs->dispatchThreads(1, currentSize, 1);
        copied += currentSize;
      }
    }
    // todo: analyze indirections buffer allocation, and if we are often not fitting in one block and SBCF_FRAMEMEM not supported -
    // reallocate buffers
    d3d::resource_barrier({indirectionAllocator.getHeap().getBuf(), RB_RO_SRV | RB_STAGE_COMPUTE});

    G_ASSERT(objectInfoCommandsBuf && sdf_info_update_cs);
    d3d::set_rwbuffer(STAGE_CS, 0, object_sdf_mips.getBuf());
    for (uint32_t copied = 0, end = commands.objectInfoCommands.size(); copied < end;)
    {
      const uint32_t currentSize = min<uint32_t>(end - copied, updateInfoBufSize);
      ShaderGlobal::set_int(get_shader_variable_id("sdf_info_update_commands_buf_size"), currentSize);
      objectInfoCommandsBuf->updateData(0, currentSize * sizeof(PackedObjectMip), commands.objectInfoCommands.data() + copied,
        VBLOCK_WRITEONLY);
      sdf_info_update_cs->dispatchThreads(currentSize, 1, 1);
      copied += currentSize;
    }
    d3d::resource_barrier({object_sdf_mips.getBuf(), RB_RO_SRV | RB_STAGE_COMPUTE});

    d3d::set_rwtex(STAGE_CS, 0, object_sdf_mip_atlas.getVolTex(), 0, 0);
    G_ASSERT(blocksCommandsBuf && sdf_blocks_update_cs);
    for (uint32_t copied = 0, end = commands.blocksCommands.size(); copied < end;)
    {
      const uint32_t currentSize = min<uint32_t>(end - copied, updateBlocksBufSize);
      blocksCommandsBuf->updateData(0, currentSize * sizeof(commands.blocksCommands[0]), commands.blocksCommands.data() + copied,
        VBLOCK_WRITEONLY);
      sdf_blocks_update_cs->dispatch(currentSize, 1, 1);
      copied += currentSize;
    }
    d3d::set_rwbuffer(STAGE_CS, 0, nullptr);
    d3d::resource_barrier({object_sdf_mip_atlas.getVolTex(), RB_RO_SRV | RB_STAGE_COMPUTE, 0, 0});
  }
  uint8_t getClampedMip(uint16_t oi, const uint8_t *new_mips, uint32_t cnt) const
  {
    if (oi > cnt)
      return SDF_NUM_MIPS;
    const uint8_t maxMip = getTotalMipCount(oi) - 1;
    auto newMip = new_mips[oi];
    return newMip < SDF_NUM_MIPS ? min(newMip, maxMip) : SDF_NUM_MIPS;
  }
  UpdateSDFQualityStatus scheduleUpdateObjects(const uint8_t *new_mips, uint32_t cnt, UpdateSDFQualityRequest q,
    GpuUpdateCommands &commands)
  {
    decltype(streamedMipsQueue) streamedMips;
    {
      OSSpinlockScopedLock lock(streamedMipsQueueLock);
      streamedMipsQueue.swap(streamedMips);
    }
    debug("streamed in %d", streamedMips.size());
    uint32_t streamedMipsBlocks = 0;
    for (auto &streamedMip : streamedMips)
      streamedMipsBlocks += getBlocksNeeded(streamedMip.id, streamedMip.mip);

    if (streamedMipsBlocks && currentBlocksUsed + streamedMipsBlocks <= maxBlocksSpace) // process all immediately, there is enough
                                                                                        // space, skip some checks later
    {
      debug("pre load");
      loadStreamedData(new_mips, cnt, streamedMips, streamedMipsBlocks, commands);
    }

    FRAMEMEM_REGION;
    dag::Vector<uint16_t, framemem_allocator> toRemoveFine, toCheck;
    toRemoveFine.reserve(cnt);
    toCheck.reserve(cnt);
    dag::RelocatableFixedVector<StreamMipCommand, 7, true, framemem_allocator> toAddFine;
    toAddFine.reserve(cnt);
    uint32_t requiredBlocks = 0, additionalFineBlocksNeeded = 0;
    uint32_t freeingFineBlocks = 0;
    for (uint16_t oi = 0; oi < cnt; ++oi)
    {
      const uint8_t newMip = getClampedMip(oi, new_mips, cnt);
      const uint8_t curMip = getBestLoadedMip(oi);
      const uint8_t mipCount = getTotalMipCount(oi);
      // debug("oi=%d, new %d cur %d", oi, newMip, curMip);
      const uint32_t newBlocks = newMip < mipCount ? getBlocksNeeded(oi, newMip) : 0;
      const uint32_t curBlocks = curMip < mipCount ? getBlocksNeeded(oi, curMip) : 0;
      requiredBlocks += newBlocks;
      if (q != UpdateSDFQualityRequest::ASYNC_REQUEST_PRECACHE && newBlocks != 0)
        objectsLRU[oi] = lastLRUTick;
      if (newMip < curMip)
      {
        additionalFineBlocksNeeded += newBlocks;
        if (q == UpdateSDFQualityRequest::SYNC || needToLoadBetterMip(oi, newMip)) // not already loading or needed immediately
        {
          auto &oRef = objectsRef[oi];
          auto &mi = oRef.mipInfo[newMip];

          toAddFine.emplace_back(StreamMipCommand{oi, StreamMipCommand::make_mip_fileRef(newMip, oRef.fileRef), mi.streamOffset,
            mi.compressedSize, (uint32_t)decompressedObjectMipSized(mi)});
          startLoading(oi, newMip);
        }
        else
          toCheck.emplace_back(oi);
      }
      else if (newMip > curMip && curMip < mipCount - 1) // we never release last mip
      {
        freeingFineBlocks += curBlocks;
        toRemoveFine.emplace_back(oi);
      }
    }
    for (uint32_t oi = cnt, e = size(); oi < e; ++oi)
    {
      const uint8_t curMip = getBestLoadedMip(oi);
      const uint8_t totalMipCount = getTotalMipCount(oi);
      if (curMip < totalMipCount)
      {
        freeingFineBlocks += getBlocksNeeded(oi, curMip);
        toRemoveFine.emplace_back(oi);
      }
    }

    if (currentBlocksUsed + additionalFineBlocksNeeded - freeingFineBlocks > maxBlocksSpace)
    {
      logerr("not enough space for all required sdf blocks. Current %d needed %d, freeing %d. We change quality now",
        currentBlocksUsed, additionalFineBlocksNeeded, freeingFineBlocks);
      // return UpdateSDFQualityStatus::NOT_READY;
      //  todo: change required mip quality and restart process.
    }

    if (streamedMips.empty() && additionalFineBlocksNeeded == 0) // we have all objects loaded (or scheduled to load) with required or
                                                                 // better quality, or scheduled
    {
      if (!toRemoveFine.empty())
      {
        // todo: check if we exhaust space too much, may be we need to start freeing fine objects anyway
      }
      debug("nothing to do %d", additionalFineBlocksNeeded);
      return UpdateSDFQualityStatus::ALL_OK;
    }
    // const uint32_t freeSpace = maxBlocksSpace - currentBlocksUsed;
    const uint32_t comfortableAllocatedSpace = maxBlocksSpace * 3 / 4; // todo: heuerestics
    G_UNUSED(comfortableAllocatedSpace);

    if (!toAddFine.empty())
    {
      if (q == UpdateSDFQualityRequest::SYNC)
        do_load(toAddFine.begin(), toAddFine.end());
      else
        scheduleStreaming(toAddFine.begin(), toAddFine.end());
    }
    if (!processStreamedData(toRemoveFine.begin(), toRemoveFine.size(), new_mips, cnt, streamedMips, streamedMipsBlocks, commands))
    {
      logwarn("not enough space in sdf atlas, reducing quality!");
    }
    if (q != UpdateSDFQualityRequest::SYNC)
    {
      for (auto &added : toAddFine)
      {
        if (getBestLoadedMip(added.id) > added.getMip())
        {
          debug("add total %d, %d:%d > %d", toAddFine.size(), added.id, getBestLoadedMip(added.id), added.getMip());
          return UpdateSDFQualityStatus::NOT_READY;
        }
      }
      for (auto oi : toCheck)
      {
        if (getBestLoadedMip(oi) > getClampedMip(oi, new_mips, cnt))
        {
          debug("check failed %d, %d:%d > %d, need %d, loading mask %d", toCheck.size(), oi, getBestLoadedMip(oi),
            getClampedMip(oi, new_mips, cnt), needToLoadBetterMip(oi, getClampedMip(oi, new_mips, cnt)), loadingMipsMask(oi));
          return UpdateSDFQualityStatus::NOT_READY;
        }
      }
    }
    return UpdateSDFQualityStatus::ALL_OK;
  }

  UpdateSDFQualityStatus scheduleUpdateObjects(const uint8_t *new_mips, uint32_t cnt, UpdateSDFQualityRequest q)
  {
    GpuUpdateCommands gpuCommands;
    if (lastUpdatedIndirectionGeneration != indirectionAllocator.getManager().getHeapGeneration())
    {
      auto invalidIndirectionRegion = indirectionAllocator.get(invalidIndirection);
      G_UNUSED(invalidIndirectionRegion);
      G_ASSERT(invalidIndirectionRegion.offset == 0 && invalidIndirectionRegion.size == 4);
      auto &commandBlock = gpuCommands.indirectionCommands.push_back();
      commandBlock.indirections.push_back(~0u);
      commandBlock.commands.push_back(IndirectionCommand{0, 0, 4}); // fill with ~0u empty indirection
      lastUpdatedIndirectionGeneration = indirectionAllocator.getManager().getHeapGeneration();
    }

    UpdateSDFQualityStatus ret = scheduleUpdateObjects(new_mips, cnt, q, gpuCommands);
    if (!gpuCommands.empty())
    {
      debug("loaded something");
      addGeneration();
    }
    submitGPUCommands(gpuCommands);
    gpuCommands.clear();
    return ret;
  }

  void freeObjectMip(AtlasObjectInfo &atlasObject, int8_t mip)
  {
    for (auto a : atlasObject.mips[mip].blocks)
    {
      currentBlocksUsed -= atlasManager.get(a).size;
      atlasManager.free(a);
    }
    atlasObject.mips[mip].blocks.clear();
    if (atlasObject.mips[mip].indirection)
      indirectionAllocator.free(atlasObject.mips[mip].indirection);
  }
  void freeObject(uint16_t i, GpuUpdateCommands &commands)
  {
    auto &atlasObject = atlasObjects[i];
    for (auto mip = 0; mip < SDF_NUM_MIPS; ++mip)
      freeObjectMip(atlasObject, mip);
    debug("free Object completely %d", i);
    scheduleUpdateObjectFreeInfo(i, commands);
  }
  void freeObject(uint16_t i, int8_t needed_mip, GpuUpdateCommands &commands)
  {
    int8_t from_mip = 0;
    auto &atlasObject = atlasObjects[i];
    G_ASSERT(getBestLoadedMip(i) < atlasObject.mips.size() && !atlasObject.mips[getBestLoadedMip(i)].blocks.empty() &&
             atlasObject.mips[getBestLoadedMip(i)].indirection);
    // ensure that quality is not worser than needed mip
    for (needed_mip = min<int8_t>(needed_mip, getTotalMipCount(i) - 1); needed_mip >= from_mip; --needed_mip)
      if (atlasObjects[i].mips[needed_mip].indirection)
        break;
    if (needed_mip <= from_mip) // we can't do anything
      return;
    auto mip = from_mip;
    for (; mip < needed_mip; ++mip)
      freeObjectMip(atlasObject, mip);

    debug("free Object %d:%d to %d, now %d", i, from_mip, needed_mip, mip);
    scheduleUpdateObjectInfo(i, mip, commands);
  }

  uint32_t getBlocksNeeded(uint16_t i, uint16_t mip) const { return objectsRef[i].mipInfo[mip].sdfBlocksCount; }
  void init(int abw, int abh, int abd)
  {
    if (atlas_bw == abw && atlas_bh == abh && atlas_bd == abd)
      return;
#define VAR(a) a##VarId = get_shader_variable_id(#a);
    GLOBAL_VARS_LIST
#undef VAR
    atlas_bw = abw;
    atlas_bh = abh;
    atlas_bd = abd;
    int atlas_w = atlas_bw * SDF_BLOCK_SIZE, atlas_h = atlas_bh * SDF_BLOCK_SIZE, atlas_d = atlas_bd * SDF_BLOCK_SIZE; // in texels
    object_sdf_mip_atlas.close();
    object_sdf_mip_atlas = dag::create_voltex(atlas_w, atlas_h, atlas_d, TEXCF_UNORDERED | TEXFMT_L8, 1,
      "object_sdf_mip_atlas"); // TEXCF_UPDATE_DESTINATION
    // object_sdf_mip_atlasCopy.close();
    // object_sdf_mip_atlasCopy = dag::create_voltex(atlas_w, atlas_h, atlas_d,
    //                                           TEXFMT_L8|TEXCF_SYSMEM|TEXCF_DYNAMIC, 1,
    //                                           "object_sdf_mip_atlas_copy");//TEXCF_UPDATE_DESTINATION
    // resptr_detail::ResPtrFactory<VolTexture>(object_sdf_mip_atlas->makeTmpTexResCopy(atlas_w, atlas_h, atlas_d,1, true));
    ShaderGlobal::set_color4(sdf_inv_atlas_size_in_blocksVarId, 1. / atlas_bw, 1. / atlas_bh, 1. / atlas_bd, 0);
    ShaderGlobal::set_color4(sdf_internal_block_size_tcVarId, float(SDF_INTERNAL_BLOCK_SIZE) / atlas_w,
      float(SDF_INTERNAL_BLOCK_SIZE) / atlas_h, float(SDF_INTERNAL_BLOCK_SIZE) / atlas_d, 0);
    ShaderGlobal::set_color4(sdf_inv_atlas_sizeVarId, 1. / atlas_w, 1. / atlas_h, 1. / atlas_d, 0);
    initAtlas();
  }

  static size_t decompressedObjectMipSized(const SparseSDFMip &mi)
  {
    return mi.getVolume() * sizeof(uint32_t) + mi.sdfBlocksCount * SDF_BLOCK_VOLUME;
  }
  static void decompressObjectMip(const uint8_t *compressedData, uint32_t compressed_size, uint8_t *decompressedData,
    uint32_t decompressed_size)
  {
    zstd_decompress(decompressedData, decompressed_size, compressedData, compressed_size);
  }

  void do_load(StreamMipCommand *commands, StreamMipCommand *commands_end)
  {
    debug("streaming in %d", commands_end - commands);
    // we sort by fileId + offset
    stlsort::sort(commands, commands_end, [&](auto &a, auto &b) {
      const intptr_t difFile = intptr_t(a.getFileRef()) - intptr_t(b.getFileRef());
      if (difFile != 0)
        return difFile < 0;
      return intptr_t(a.streamOffset) < intptr_t(b.streamOffset);
    });
    uint16_t curFileRef = ~0;
    file_ptr_t fp = 0;
    int fileLen = 0;
    const uint8_t *fileData = 0;

    for (auto cmdI = commands; cmdI != commands_end; cmdI++)
    {
      auto &cmd = *cmdI;

      // todo: replace with FastSeqReader
      // only for dfmap, otherwise use streaming
      auto fileRef = cmd.getFileRef();
      if (fileRef != curFileRef)
      {
        if (fileData)
          df_unmap(fileData, fileLen);
        if (fp)
          df_close(fp);
        const char *curFileName = getFileNameSafe(fileRef);
        curFileRef = fileRef;
        fp = df_open(curFileName, DF_REALFILE_ONLY | DF_READ);
        fileData = (const uint8_t *)(fp ? df_mmap(fp, &fileLen) : nullptr);
      }
      if (fileData)
      {
        StreamedMip streamed;
        streamed.id = cmd.id;
        streamed.mip = cmd.getMip();
        streamed.decodedData = eastl::make_unique<uint8_t[]>(cmd.decompressedSize);
        decompressObjectMip(fileData + cmd.streamOffset, cmd.compressedSize, streamed.decodedData.get(), cmd.decompressedSize);

        {
          OSSpinlockScopedLock lock(streamedMipsQueueLock);
          streamedMipsQueue.push_back(eastl::move(streamed));
        }
        addGeneration(); // todo: special command issuance for 'add generation'
        debug("streamed %d mip %d", streamed.id, streamed.mip);
      }
      // todo: unlock objectsRef
    }
    if (fileData)
      df_unmap(fileData, fileLen);
    if (fp)
      df_close(fp);
  }
  uint32_t current_generation = 0;
  void addGeneration() { interlocked_add(current_generation, 1); }
  uint32_t getGeneration() const { return interlocked_relaxed_load(current_generation); }
  void scheduleStreaming(const StreamMipCommand *s, const StreamMipCommand *e)
  {
    {
      OSSpinlockScopedLock lock(streamMipsCommandBufferLock);
      streamMipsCommandBuffer.insert(streamMipsCommandBuffer.end(), s, e);
    }
    queueEvent.signal();
  }
  bool stream()
  {
    decltype(streamMipsCommandBuffer) commands;
    {
      OSSpinlockScopedLock lock(streamMipsCommandBufferLock);
      commands.swap(streamMipsCommandBuffer);
    }
    if (commands.empty())
      return false;
    do_load(commands.begin(), commands.end());
    return true;
  }
};

void SDFStreamingThread::execute()
{
  do
  {
    lru.queueEvent.wait(2000);
    while (!isThreadTerminating() && lru.stream()) {}
  } while (!isThreadTerminating());
}
SDFStreamingThread::~SDFStreamingThread()
{
  debug("deleted thread");
  terminate(true, -1, &lru.queueEvent.event);
}

struct ObjectsSDFImpl : public ObjectsSDF
{
  PostFxRenderer sdf_object_debug;
  ObjectsAtlasLru atlasLRU;
  uint32_t maxObjectsBlocks = 0, allObjectsLowBlocks = 0;
  carray<float, SDF_NUM_MIPS> mipVoxelSizes, mipVoxelMaxSizes;
  float getMipVoxelSize(int i) const { return mipVoxelSizes[i] / atlasLRU.size(); }
  float getMipVoxelMaxSize(int i) const { return mipVoxelMaxSizes[i]; }
  float getMipSize(int mip) const override { return 0.5 * (1 << mip); } // todo: fixme: we read approximate average

  UniqueBufHolder object_sdf_instances;

  uint32_t instancesCount = 0, instancesBufferSize = 0;
  uint32_t getInstances() const { return instancesCount; }

  uint32_t addObjectRef(IGenLoad &cb)
  {
    MippedMeshSDFRef meshSDF;
    cb.read(&meshSDF.mipCountTwoSided, sizeof(meshSDF.mipCountTwoSided));
    BBox3 localBounds;
    cb.read(&localBounds, sizeof(localBounds));
    meshSDF.setBounds(localBounds);
    const Point3 boundsWidth = meshSDF.boundsWidth();

    cb.read(meshSDF.mipInfo.data(), meshSDF.mipCount() * sizeof(meshSDF.mipInfo[0]));
    const uint32_t ofs = cb.tell() + sizeof(int);
    float maxVoxelDim = 0;
    for (int i = 0; i < meshSDF.mipCount(); ++i)
    {
      auto &mi = meshSDF.mipInfo[i];
      Point3 voxelDim = div(boundsWidth, point3(getIndirectionDim(mi) * SDF_BLOCK_SIZE));
      // float voxelSize = length(voxelDim)/sqrt(3.f);
      maxVoxelDim = max(voxelDim.x, max(voxelDim.y, voxelDim.z));
      mipVoxelSizes[i] += maxVoxelDim;
      mipVoxelMaxSizes[i] = max(maxVoxelDim, mipVoxelMaxSizes[i]);
      // maxObjectsBlocks += mi.sdfBlocksCount;
      mi.streamOffset += ofs;
    }
    for (int i = meshSDF.mipCount(); i < SDF_NUM_MIPS; ++i)
      mipVoxelSizes[i] += maxVoxelDim;

    maxObjectsBlocks += meshSDF.mipInfo.front().sdfBlocksCount;
    allObjectsLowBlocks += meshSDF.mipInfo.back().sdfBlocksCount;
    meshSDF.fileRef = atlasLRU.addFileRefSafe(cb.getTargetName());
    return atlasLRU.addObject(eastl::move(meshSDF));
  }

  void setAtlasSize(int atlas_bw, int atlas_bh, int atlas_bd) { atlasLRU.init(atlas_bw, atlas_bh, atlas_bd); }
  void init(IGenLoad &cb)
  {
    sdf_object_debug.init("sdf_object_debug");
    setAtlasSize(512 / SDF_BLOCK_SIZE, 256 / SDF_BLOCK_SIZE, 256 / SDF_BLOCK_SIZE);
    memset(&mipVoxelSizes[0], 0, sizeof(mipVoxelSizes));
    memset(&mipVoxelMaxSizes[0], 0, sizeof(mipVoxelMaxSizes));
    int cnt = cb.readInt();
    {
      atlasLRU.initAtlasObjects(cnt);
      auto at = cb.tell();
      for (int i = 0; i < cnt; ++i)
      {
        addObjectRef(cb);
        int compressed = cb.readInt();
        cb.seekrel(compressed);
      }
      cb.seekto(at);
      debug("all ObjectsBlocks %d (low %d) out of %d objects", maxObjectsBlocks, allObjectsLowBlocks, cnt);
      debug("mipSizes %f %f %f", getMipVoxelSize(0), getMipVoxelSize(1), getMipVoxelSize(2));
      debug("mipSizesMax %f %f %f", getMipVoxelMaxSize(0), getMipVoxelMaxSize(1), getMipVoxelMaxSize(2));
    }
    G_STATIC_ASSERT(sizeof(PackedObjectMip) == sizeof(float) * 4 * SDF_PACKED_MIP_SIZE);
    G_STATIC_ASSERT(sizeof(ObjectSDFInstancePacked) == sizeof(float) * 4 * SDF_PACKED_INSTANCE_SIZE);
  }
  struct CachedState
  {
    uint64_t objects_mips = 0;
    uint32_t cnt = 0;
    uint32_t generation = 0;
    UpdateSDFQualityStatus status = UpdateSDFQualityStatus::NOT_READY;
  } cState;

  bool shouldUpdate(const uint8_t *new_mips, uint32_t cnt)
  {
    CachedState nState;
    nState.cnt = cnt;
    nState.objects_mips = XXH3_64bits(new_mips, sizeof(*new_mips) * cnt);
    nState.generation = atlasLRU.getGeneration();
    if (nState.cnt == cState.cnt && nState.objects_mips == cState.objects_mips && nState.generation == cState.generation)
      return false;
    cState = nState;
    return true;
  }
  UpdateSDFQualityStatus checkObjects(UpdateSDFQualityRequest q, const uint8_t *new_mips, uint32_t cnt) override
  {
    if (!cnt)
      return UpdateSDFQualityStatus::ALL_OK;
    atlasLRU.updateLRUtick();
    if (!shouldUpdate(new_mips, cnt) && cState.generation == atlasLRU.getGeneration())
    {
      debug("already scheduled all with cache status %d q = %d", (int)cState.status, (int)q);
      return cState.status;
    }
    cState.status = atlasLRU.scheduleUpdateObjects(new_mips, cnt, q);
    debug("scheduleUpdateObjects = %d with quality %d", (int)cState.status, (int)q);
    return cState.status;
  }

  void debugRender() override
  {
    TIME_D3D_PROFILE(objects_sdf);
    TMatrix4 globtm;
    d3d::getglobtm(globtm);
    globtm = globtm.transpose();
    ShaderGlobal::set_color4(globtm_psf_0VarId, Color4(globtm[0]));
    ShaderGlobal::set_color4(globtm_psf_1VarId, Color4(globtm[1]));
    ShaderGlobal::set_color4(globtm_psf_2VarId, Color4(globtm[2]));
    ShaderGlobal::set_color4(globtm_psf_3VarId, Color4(globtm[3]));
    sdf_object_debug.render();
  }
  int getMipsCount() const override { return SDF_NUM_MIPS; }

  int uploadInstances(const uint16_t *obj_ids, const mat43f *instance, int cnt) override
  {
    mat44f rescaleTm;
    dag::Vector<ObjectSDFInstancePacked, framemem_allocator, false> instancesData;
    instancesData.reserve(cnt);
    dag::Vector<vec4f, framemem_allocator, false> instancesBounds;
    instancesBounds.reserve(cnt);
    for (auto e = obj_ids + cnt; obj_ids != e; obj_ids++, instance++)
    {
      auto i = *obj_ids;
      // actually almost all of it can be done in gpu. we need just instance info and matrix
      // todo: once
      G_ASSERTF(atlasLRU.getBestLoadedMip(i) < atlasLRU.getTotalMipCount(i), "%d: %d < %d", i, atlasLRU.getBestLoadedMip(i),
        atlasLRU.getTotalMipCount(i));
      if (atlasLRU.getBestLoadedMip(i) >= atlasLRU.getTotalMipCount(i)) // not loaded
        continue;
      const auto &meshSDF = atlasLRU.objectsRef[i];
      const float scale = meshSDF.getScale();
      const Point3 ofs = meshSDF.boundsCenter();
      rescaleTm.col0 = v_make_vec4f(scale, 0, 0, 0);
      rescaleTm.col1 = v_make_vec4f(0, scale, 0, 0);
      rescaleTm.col2 = v_make_vec4f(0, 0, scale, 0);
      rescaleTm.col3 = v_make_vec4f(ofs.x, ofs.y, ofs.z, 1);

      mat44f localToWorldInst;
      v_mat43_transpose_to_mat44(localToWorldInst, *instance);
      mat44f localToWorld;
      v_mat44_mul43(localToWorld, localToWorldInst, rescaleTm);

#if SDF_SUPPORT_OBJECT_TRACING
      v_mat44_transpose_to_mat43(inst.localToWorld, localToWorld);
#endif
      ObjectSDFInstancePacked inst;
      mat44f worldToLocal;
      v_mat44_inverse43(worldToLocal, localToWorld);
      v_mat44_transpose_to_mat43(inst.worldToLocal, worldToLocal);
      inst.objectId = i;
      inst.extent = meshSDF.getExt();
      vec4f vtws = v_perm_xaxa(v_length3_x(localToWorld.col0), v_length3_x(localToWorld.col1));
      vtws = v_perm_xyab(vtws, v_length3_x(localToWorld.col2));
      vtws = v_perm_xyzd(vtws, v_min(v_splat_x(vtws), v_min(v_splat_y(vtws), v_splat_z(vtws))));
      v_st(&inst.volumeToWorldScale.x, vtws);
      // world space bounding sphere
      bbox3f destBox;
      destBox.bmax = v_ldu(&inst.extent.x);
      destBox.bmin = v_neg(destBox.bmax);
      v_bbox3_init(destBox, localToWorld, destBox);
      vec4f bound = v_perm_xyzd(v_bbox3_center(destBox), v_splat_x(v_bbox3_outer_rad(destBox)));
      instancesData.push_back(inst);
      instancesBounds.push_back(bound);
    }
    // todo: replace with discard less fence-based ring buffer
    const size_t requiredSz = instancesBounds.size() + instancesData.size() * SDF_PACKED_INSTANCE_SIZE;
    if (requiredSz > instancesBufferSize)
    {
      constexpr int page_size = 4096; // in float4, so it is 64k bytes
      instancesBufferSize = (requiredSz + page_size - 1) & (~(page_size - 1));
      object_sdf_instances.close();
      object_sdf_instances = dag::create_sbuffer(sizeof(float) * 4, instancesBufferSize, SBCF_MISC_STRUCTURED | SBCF_BIND_SHADER_RES,
        0, "object_sdf_instances");
    }

    G_ASSERT(instancesBounds.size() == instancesData.size());
    if (instancesBounds.size())
    {
#if 1
      object_sdf_instances->updateData(0, instancesBounds.size() * sizeof(instancesBounds[0]), instancesBounds.data(),
        VBLOCK_WRITEONLY);
      object_sdf_instances->updateData(instancesBounds.size() * sizeof(instancesBounds[0]),
        instancesData.size() * sizeof(instancesData[0]), instancesData.data(), VBLOCK_WRITEONLY);
#else
      instancesBounds.resize(instancesBounds.size() * (1 + sizeof(ObjectSDFInstancePacked) / sizeof(vec4f)));
      memcpy(instancesBounds.data() + instancesData.size(), instancesData.data(),
        instancesData.size() * sizeof(ObjectSDFInstancePacked));
      object_sdf_instances->updateData(0, instancesData.size() * (sizeof(instancesData[0]) + sizeof(instancesBounds[0])),
        instancesBounds.data(), VBLOCK_WRITEONLY);
#endif
    }
    instancesCount = instancesData.size();
    debug("uploadInstances %d", instancesCount);
    ShaderGlobal::set_int(get_shader_variable_id("object_sdf_instances_count", true), instancesData.size());
    ShaderGlobal::set_int(get_shader_variable_id("object_sdf_instances_data_offset", true), instancesData.size()); // currently it is
                                                                                                                   // same
    return instancesCount;
  }
};

ObjectsSDF *new_objects_sdf(IGenLoad &cb)
{
  auto r = new ObjectsSDFImpl;
  r->init(cb);
  return r;
}