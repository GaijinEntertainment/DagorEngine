#include "device.h"

#include <util/dag_watchdog.h>

using namespace drv3d_dx12;

#if _TARGET_XBOX
#include "device_queue_xbox.h"
#endif

DeviceQueue::TileMapperAccel DeviceQueue::tileMapperAccel;

bool DeviceQueueGroup::init(ID3D12Device *device, bool give_names)
{
  ComPtr<ID3D12CommandQueue> graphicsQueue, computeQueue, uploadQueue, readBackQueue;
#if _TARGET_PC
  D3D12_COMMAND_QUEUE_DESC cqd = {};
  cqd.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;

  cqd.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
  if (!DX12_CHECK_OK(device->CreateCommandQueue(&cqd, COM_ARGS(&graphicsQueue))))
  {
    return false;
  }

  cqd.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
  if (!DX12_CHECK_OK(device->CreateCommandQueue(&cqd, COM_ARGS(&computeQueue))))
  {
    return false;
  }

  cqd.Type = D3D12_COMMAND_LIST_TYPE_COPY;
  if (!DX12_CHECK_OK(device->CreateCommandQueue(&cqd, COM_ARGS(&uploadQueue))))
  {
    return false;
  }

  cqd.Type = D3D12_COMMAND_LIST_TYPE_COPY; // -V1048
  if (!DX12_CHECK_OK(device->CreateCommandQueue(&cqd, COM_ARGS(&readBackQueue))))
  {
    return false;
  }
#else
  if (!xbox_create_queues(device, graphicsQueue, computeQueue, uploadQueue, readBackQueue))
    return false;
#endif

  if (!uploadProgress.reset(device, give_names ? L"UploadProgress" : nullptr))
  {
    return false;
  }
  if (!graphicsProgress.reset(device, give_names ? L"GraphicsProgress" : nullptr))
  {
    return false;
  }
  if (!frameProgress.reset(device, give_names ? L"FrameProgress" : nullptr))
  {
    return false;
  }

  if (give_names)
  {
    DX12_SET_DEBUG_OBJ_NAME(uploadQueue, L"UploadQueue");
    DX12_SET_DEBUG_OBJ_NAME(readBackQueue, L"ReadBackQueue");
    DX12_SET_DEBUG_OBJ_NAME(computeQueue, L"ComputeQueue");
    DX12_SET_DEBUG_OBJ_NAME(graphicsQueue, L"GraphicsQueue");
  }

  group[static_cast<uint32_t>(DeviceQueueType::GRAPHICS)] = DeviceQueue(eastl::move(graphicsQueue));
  group[static_cast<uint32_t>(DeviceQueueType::COMPUTE)] = DeviceQueue(eastl::move(computeQueue));
  group[static_cast<uint32_t>(DeviceQueueType::UPLOAD)] = DeviceQueue(eastl::move(uploadQueue));
  group[static_cast<uint32_t>(DeviceQueueType::READ_BACK)] = DeviceQueue(eastl::move(readBackQueue));

  return true;
}

void DeviceQueueGroup::shutdown()
{
  for (auto &&q : group)
    q = DeviceQueue{};
  uploadProgress.reset();
  graphicsProgress.reset();
  frameProgress.reset();
}

DeviceQueue::~DeviceQueue() {}

void DeviceQueue::TileMapperAccel::clear()
{
#if _TARGET_PC_WIN
  trcs.clear();
  trss.clear();
  offsets.clear();
  counts.clear();
  rangeFlags.clear();
#else
  trBatches.clear();
  trRanges.clear();
  pTexComputer.Reset();
#endif
}

#if _TARGET_PC_WIN
void DeviceQueue::beginTileMapping(ID3D12Resource *tex, ID3D12Heap *heap, size_t heap_base, size_t mapping_count)
{
  G_ASSERT(tileMapperAccel.trcs.empty() && tileMapperAccel.trss.empty() && tileMapperAccel.offsets.empty() &&
           tileMapperAccel.rangeFlags.empty());

  tileMapperAccel.trcs.reserve(mapping_count);
  tileMapperAccel.trss.reserve(mapping_count);
  tileMapperAccel.offsets.reserve(mapping_count);
  tileMapperAccel.counts.reserve(mapping_count);

  tileMapperAccel.rangeFlags.resize(mapping_count, heap ? D3D12_TILE_RANGE_FLAG_NONE : D3D12_TILE_RANGE_FLAG_NULL);

  tileMapperAccel.tex = tex;
  tileMapperAccel.heap = heap;
  tileMapperAccel.heapBase = heap_base;
}
#else
void DeviceQueue::beginTileMapping(ID3D12Resource *tex, XGTextureAddressComputer *texAddressComputer, uintptr_t address, uint32_t size,
  size_t mapping_count)
{
  G_UNUSED(tex);
  G_ASSERT(tileMapperAccel.trBatches.empty() && tileMapperAccel.trRanges.empty());

  tileMapperAccel.trBatches.reserve(mapping_count);
  tileMapperAccel.trRanges.reserve(mapping_count);
  tileMapperAccel.sourcePoolPageVA = address;
  tileMapperAccel.sourcePoolPageSize = size;

  tileMapperAccel.tex = tex;
  tileMapperAccel.pTexComputer = texAddressComputer;
}
#endif

void DeviceQueue::addTileMappings(const TileMapping *mapping, size_t mapping_count)
{
  for (auto &map : eastl::span{mapping, mapping_count})
  {
#if _TARGET_PC_WIN
    auto &trc = tileMapperAccel.trcs.emplace_back();
    trc.X = map.texX;
    trc.Y = map.texY;
    trc.Z = map.texZ;
    trc.Subresource = map.texSubresource;

    auto &trs = tileMapperAccel.trss.emplace_back();
    trs.UseBox = (map.heapTileSpan == 1) ? TRUE : FALSE;
    trs.Width = map.heapTileSpan;
    trs.Height = 1;
    trs.Depth = 1;
    trs.NumTiles = map.heapTileSpan;

    tileMapperAccel.offsets.emplace_back(map.heapTileIndex + tileMapperAccel.heapBase);

    tileMapperAccel.counts.emplace_back(map.heapTileSpan);
#else
    auto &trRange = tileMapperAccel.trRanges.emplace_back();
    trRange.RangeType = tileMapperAccel.sourcePoolPageVA ? D3D12XBOX_PAGE_MAPPING_RANGE_TYPE_INCREMENTING_PAGE_INDICES
                                                         : D3D12XBOX_PAGE_MAPPING_RANGE_TYPE_NULL_PAGES;
    trRange.StartPageIndexInPool = map.heapTileIndex;
    trRange.PageCount = map.heapTileSpan;

    auto &trBatch = tileMapperAccel.trBatches.emplace_back();
    trBatch.RangeCount = 1;
    trBatch.pRanges = &trRange;

    UINT32 numTilesForEntireResource;
    XG_PACKED_MIP_DESC PackedMipDesc;
    XG_TILE_SHAPE shape;
    UINT32 NumSubresourceTilings = 1;
    XG_SUBRESOURCE_TILING SubresourceTilingsForNonPackedMips;

    tileMapperAccel.pTexComputer->GetResourceTiling(&numTilesForEntireResource, &PackedMipDesc, &shape, &NumSubresourceTilings,
      map.texSubresource, &SubresourceTilingsForNonPackedMips);

#if _TARGET_XBOXONE
    UINT64 tileOffset = tileMapperAccel.pTexComputer->GetTexelElementOffsetBytes(0, map.texSubresource, map.texX * shape.WidthInTexels,
      map.texY * shape.HeightInTexels, map.texZ * shape.DepthInTexels, 0);
#else
    UINT32 bitPosition = 0;
    UINT64 tileOffset = tileMapperAccel.pTexComputer->GetTexelElementOffsetBytes(0, map.texSubresource, map.texX * shape.WidthInTexels,
      map.texY * shape.HeightInTexels, map.texZ * shape.DepthInTexels, 0, &bitPosition);
#endif

    trBatch.DestinationAddress = tileMapperAccel.tex->GetGPUVirtualAddress() + tileOffset; //
#endif
  }
}

void DeviceQueue::endTileMapping()
{
#if _TARGET_PC_WIN
  G_ASSERT((tileMapperAccel.trcs.size() & tileMapperAccel.trss.size() & tileMapperAccel.offsets.size() &
             tileMapperAccel.counts.size() & tileMapperAccel.rangeFlags.size()) == tileMapperAccel.trcs.size());

  queue->UpdateTileMappings(tileMapperAccel.tex, tileMapperAccel.trcs.size(), tileMapperAccel.trcs.data(), tileMapperAccel.trss.data(),
    tileMapperAccel.heap, tileMapperAccel.trcs.size(), tileMapperAccel.rangeFlags.data(), tileMapperAccel.offsets.data(),
    tileMapperAccel.counts.data(), D3D12_TILE_MAPPING_FLAG_NONE);

#else
  G_ASSERT(tileMapperAccel.trBatches.size() == tileMapperAccel.trRanges.size());

  queue->CopyPageMappingsBatchX(tileMapperAccel.trBatches.size(), &tileMapperAccel.trBatches[0], tileMapperAccel.sourcePoolPageVA,
    tileMapperAccel.sourcePoolPageSize / TEXTURE_TILE_SIZE, D3D12XBOX_PAGE_MAPPING_FLAG_NONE);

#endif

  tileMapperAccel.clear();
}
