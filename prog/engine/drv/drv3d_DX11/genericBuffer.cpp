// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "genericBuffer.h"

#include "validation.h"
#include "drv_assert_defs.h"
#include "drv_log_defs.h"

#include "driver.h"
#include "render_state.h"
#include "buffers.h"
#include <drv/3d/dag_vertexIndexBuffer.h>
#include <drv/3d/dag_shaderConstants.h>
#include <memory/dag_framemem.h>
#include <perfMon/dag_graphStat.h>

using namespace drv3d_dx11;

static void set_buffer_name(ID3D11Buffer *buffer, const char *name)
{
#if DAGOR_DBGLEVEL > 0
  if (buffer)
    if (auto len = (int)strlen(name))
      buffer->SetPrivateData(WKPDID_D3DDebugObjectName, len, name);
#endif
  G_UNUSED(buffer);
  G_UNUSED(name);
}

static bool create_buffer(const D3D11_BUFFER_DESC &desc, const char *name, void *initial_data, ID3D11Buffer *&buffer)
{
  D3D11_SUBRESOURCE_DATA idata;
  idata.pSysMem = initial_data;
  idata.SysMemPitch = 0;
  idata.SysMemSlicePitch = 0;

  if (device_is_lost == S_OK)
  {
    last_hres = dx_device->CreateBuffer(&desc, initial_data ? &idata : NULL, &buffer);
    if (FAILED(last_hres))
    {
      if (!device_should_reset(last_hres, "CreateBuffer"))
      {
        DXFATAL(last_hres, "CreateBuffer");
        return false;
      }
    }
  }
  set_buffer_name(buffer, name);
  return true;
}

static bool create_zeromem_buffer(const D3D11_BUFFER_DESC &desc, const char *name, ID3D11Buffer *&buffer)
{
  dag::Vector<unsigned char, framemem_allocator> zeroes(desc.ByteWidth, 0);
  return create_buffer(desc, name, zeroes.data(), buffer);
}

namespace drv3d_dx11
{
extern void removeHandleFromList(int handle);

GenericBuffer::~GenericBuffer()
{
  removeHandleFromList(handle);
  removeFromStates();
  // release calls 'getSize' virtual function indirectly. It is not recommended from desctructor
  // but GenericBuffer is final class, so it should be fine
  static_assert(std::is_final<GenericBuffer>());
  release(); // -V1053
  setRld(NULL);
}

bool GenericBuffer::createBuf()
{
  if ((bufFlags & SBCF_STAGING_BUFFER) == SBCF_STAGING_BUFFER && !bindFlags && !miscFlags)
    return createStagingBuffer(buffer);
  D3D11_BUFFER_DESC bd = {};
  bd.ByteWidth = (UINT)bufSize;
  if ((bufFlags & SBCF_DYNAMIC) && bufSize >= (16 << 20))
    logwarn("Potential performance issue. Dynamic buffer <%s> of size bigger than 16mb", getName());

  bd.Usage = (bufFlags & SBCF_DYNAMIC) ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;
  bd.BindFlags = bindFlags;
  bd.MiscFlags = miscFlags;
  static_assert((SBCF_CPU_ACCESS_WRITE << 2) == D3D11_CPU_ACCESS_WRITE);
  static_assert((SBCF_CPU_ACCESS_READ << 2) == D3D11_CPU_ACCESS_READ);
  bd.CPUAccessFlags = (bufFlags & SBCF_CPU_ACCESS_MASK) << 2;
  if ((bufFlags & SBCF_DYNAMIC))
    bd.CPUAccessFlags |= D3D11_CPU_ACCESS_WRITE;
  else
    bd.CPUAccessFlags &= (~D3D11_CPU_ACCESS_WRITE);
  if (bufFlags & SBCF_MISC_STRUCTURED)
    bd.StructureByteStride = structSize;
  if (format)
  {
    D3D_CONTRACT_ASSERT(dxgi_format_to_bpp(dxgi_format_from_flags(format)) == structSize * 8);
  }
  D3D_CONTRACT_ASSERTF(!(bufFlags & SBCF_BIND_CONSTANT) || bufSize <= 65536,
    "Windows 7 and earlier can not create cb of size more than 64k. %s is %d", getName(), bufSize);

  D3D_CONTRACT_ASSERTF(!(bd.CPUAccessFlags & D3D11_CPU_ACCESS_WRITE) || bd.Usage == D3D11_USAGE_DYNAMIC,
    "CPU write access is only supported for dynamic buffers and staging buffers. %s has usage 0x%x", getName(), bd.Usage);

  D3D_CONTRACT_ASSERTF(!(bd.CPUAccessFlags & D3D11_CPU_ACCESS_READ),
    "CPU read access is only supported for staging buffers. %s has usage 0x%x", getName(), bd.Usage);

  if (bufFlags & SBCF_ZEROMEM)
    return create_zeromem_buffer(bd, getName(), buffer);
  else
    return create_buffer(bd, getName(), nullptr, buffer);
}

bool GenericBuffer::createSrv()
{
  int elements = bufSize / structSize;
  D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc;
  ZeroMemory(&srv_desc, sizeof(srv_desc));
  if (bufFlags & SBCF_MISC_ALLOW_RAW)
  {
    D3D_CONTRACT_ASSERT(structSize == 4);
    srv_desc.Format = DXGI_FORMAT_R32_TYPELESS;
    srv_desc.BufferEx.Flags = D3D11_BUFFER_UAV_FLAG_RAW;
    srv_desc.ViewDimension = D3D_SRV_DIMENSION_BUFFEREX;
    srv_desc.BufferEx.FirstElement = 0;
    srv_desc.BufferEx.NumElements = elements;
  }
  else
  {
    srv_desc.ViewDimension = D3D_SRV_DIMENSION_BUFFER;
    if (bufFlags & SBCF_MISC_STRUCTURED)
    {
      srv_desc.Format = DXGI_FORMAT_UNKNOWN;
      srv_desc.Buffer.FirstElement = 0;
      srv_desc.Buffer.NumElements = elements;
#if DAGOR_DBGLEVEL > 0
      if (!is_good_buffer_structure_size(structSize))
      {
        D3D_CONTRACT_ERROR("The structure size of %u of buffer %s has a hardware unfriendly size and "
                           "probably results in degraded performance. For some platforms it might requires "
                           "the shader compiler to restructure the structure type to avoid layout violations "
                           "and on other platforms it results in wasted memory as the memory manager has to "
                           "insert extra padding to align the buffer properly.",
          structSize, getName());
      }
#endif
    }
    else
    {
      srv_desc.Format = dxgi_format_from_flags(format);
      srv_desc.Buffer.FirstElement = 0;
      srv_desc.Buffer.NumElements = elements * (structSize / (dxgi_format_to_bpp(dxgi_format_from_flags(format)) / 8));
      // srv_desc.Buffer.ElementOffset = 0;
      // srv_desc.Buffer.ElementWidth = structSize/4;// /(sizeof(format)/4)
    }
  }
  if (device_is_lost == S_OK)
  {
    HRESULT hr = dx_device->CreateShaderResourceView(buffer, &srv_desc, &srv);
    if (FAILED(hr))
    {
      if (!device_should_reset(hr, "CreateShaderResourceView"))
      {
        DXFATAL(hr, "CreateShaderResourceView");
        return false;
      }
    }
  }
  return true;
}

ID3D11ShaderResourceView *GenericBuffer::getResView()
{
  if (srv)
  {
    return srv;
  }

  if (!structSize)
  {
    D3D_CONTRACT_ERROR("getSrv on unstructured buffer");
    return NULL;
  }

  createSrv();
  return srv;
}

bool GenericBuffer::createUav()
{
  D3D_CONTRACT_ASSERT(bufFlags & SBCF_BIND_UNORDERED);
  if (!(bufFlags & SBCF_BIND_UNORDERED))
  {
    return false;
  }

  D3D11_UNORDERED_ACCESS_VIEW_DESC descView;
  ZeroMemory(&descView, sizeof(descView));
  descView.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
  descView.Buffer.FirstElement = 0;
  int elements = bufSize / structSize;
  descView.Buffer.NumElements = elements;

  if (bufFlags & SBCF_MISC_ALLOW_RAW)
  {
    descView.Format = DXGI_FORMAT_R32_TYPELESS;
    descView.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_RAW;
  }
  else
  {
    if (bufFlags & SBCF_MISC_STRUCTURED)
    {
      descView.Format = DXGI_FORMAT_UNKNOWN;
#if DAGOR_DBGLEVEL > 0
      if (!is_good_buffer_structure_size(structSize))
      {
        D3D_CONTRACT_ERROR("The structure size of %u of buffer %s has a hardware unfriendly size and "
                           "probably results in degraded performance. For some platforms it might requires "
                           "the shader compiler to restructure the structure type to avoid layout violations "
                           "and on other platforms it results in wasted memory as the memory manager has to "
                           "insert extra padding to align the buffer properly.",
          structSize, getName());
      }
#endif
    }
    else
    {
      descView.Format = dxgi_format_from_flags(format); // fixme:
    }
    descView.Buffer.Flags = 0;
  }

  if (device_is_lost == S_OK)
  {
    HRESULT hr = dx_device->CreateUnorderedAccessView(buffer, &descView, &uav);
    if (FAILED(hr))
    {
      if (!device_should_reset(last_hres, "CreateUnorderedAccessView"))
      {
        DXFATAL(hr, "CreateUnorderedAccessView");
        return false;
      }
    }
  }
  return true;
}

void GenericBuffer::setBufName(const char *name) { setName(name); }

bool GenericBuffer::create(uint32_t bufsize, int buf_flags, UINT bind_flags, const char *statName_)
{
  this->bufFlags = buf_flags;
  this->bufSize = bufsize;
  this->bindFlags = bind_flags;
  this->miscFlags = (buf_flags & SBCF_MISC_MASK) >> 20;
  if (bufFlags & SBCF_MISC_ALLOW_RAW)
  {
    structSize = 4;
  }
  setBufName(statName_);
  TEXQL_ON_PERSISTENT_ALLOC(this);

  if (buf_flags & SBCF_DYNAMIC)
  {
    return createBuf();
  }

  return true; // delayed create
}

bool GenericBuffer::createStructured(uint32_t struct_size, uint32_t elements, uint32_t buf_flags, uint32_t buf_format,
  const char *statName_)
{
  D3D_CONTRACT_ASSERT(struct_size == 4 || !(buf_flags & SBCF_MISC_ALLOW_RAW));
  if (bool(buf_flags & SBCF_MISC_STRUCTURED) && bool(buf_flags & SBCF_MISC_DRAWINDIRECT))
  {
    D3D_CONTRACT_ERROR("indirect buffer can't be structured one, check <%s>", statName_);
    D3D_CONTRACT_ASSERT_FAIL("indirect buffer can't be structured one, <%s>", statName_);
  }
  D3D_CONTRACT_ASSERTF(
    buf_format == 0 || ((buf_flags & (SBCF_BIND_VERTEX | SBCF_BIND_INDEX | SBCF_MISC_STRUCTURED | SBCF_MISC_ALLOW_RAW)) == 0),
    "can't create buffer with texture format which is Structured, Vertex, Index or Raw");
  this->bufFlags = buf_flags;
  this->bufSize = struct_size * elements;
  this->structSize = struct_size;
  this->format = buf_format;
  this->bindFlags = (buf_flags & SBCF_BIND_MASK) >> 16;
  this->miscFlags = (buf_flags & SBCF_MISC_MASK) >> 20;
  setBufName(statName_);
  TEXQL_ON_PERSISTENT_ALLOC(this);

  return createBuf();
}

bool GenericBuffer::recreateBuf(Sbuffer *sb)
{
  bool result = true;
  TEXQL_ON_PERSISTENT_ALLOC(this);
  if (!(bufFlags & SBCF_DYNAMIC) && rld)
    ; // delayed recreation, don't alloc buffer here
  else if (bufFlags & (SBCF_BIND_INDEX | SBCF_BIND_VERTEX))
  {
    result |= createBuf();
  }
  else
  {
    result |= createBuf(); // Create resource before UAV.
    if (bufFlags & SBCF_BIND_UNORDERED)
      result |= createUav();
  }

  if (rld)
  {
    TEXQL_ON_PERSISTENT_ALLOC(this);
    rld->reloadD3dRes(sb);
    return true;
  }

  return true;
}

void GenericBuffer::release()
{
  if (uav)
  {
    uav->Release();
    uav = NULL;
  }

  if (srv)
  {
    srv->Release();
    srv = NULL;
  }

  if (buffer)
  {
    TEXQL_ON_PERSISTENT_RELEASE(this);
    buffer->Release();
    buffer = NULL;
  }

  releaseStagingBuffer();

  if (resetting_device_now)
  {
    return;
  }
}

void GenericBuffer::destroyObject() { delete this; }

void GenericBuffer::destroy() { destroyObject(); }

bool GenericBuffer::copyTo(Sbuffer *dest, uint32_t dst_ofs_bytes, uint32_t src_ofs_bytes, uint32_t size_bytes)
{
  if (!dest)
    return false;
  GenericBuffer *destvb = (GenericBuffer *)dest;
  if (!destvb->buffer)
    return false;
  D3D11_BOX box;
  box.left = src_ofs_bytes;
  box.right = src_ofs_bytes + (size_bytes == 0 ? bufSize - src_ofs_bytes : size_bytes);
  box.top = 0;
  box.bottom = 1;
  box.front = 0;
  box.back = 1;
  ContextAutoLock contextLock;
  VALIDATE_GENERIC_RENDER_PASS_CONDITION(!g_render_state.isGenericRenderPassActive,
    "DX11: GenericBuffer::copyTo uses CopySubresourceRegion inside a generic render pass");
  disable_conditional_render_unsafe();
  dx_context->CopySubresourceRegion(destvb->buffer, 0, dst_ofs_bytes, 0, 0, buffer, 0, &box);
  destvb->internalState = UPDATED_BY_COPYTO;
  return true;
}
bool GenericBuffer::copyTo(Sbuffer *dest)
{
  if (!dest)
    return false;
  GenericBuffer *destvb = (GenericBuffer *)dest;
  if (!destvb->buffer)
    return false;
  copyInternal(destvb->buffer, buffer);
  destvb->internalState = UPDATED_BY_COPYTO;
  return true;
}

bool GenericBuffer::updateData(uint32_t ofs_bytes, uint32_t size_bytes, const void *__restrict src, uint32_t lockFlags)
{
#ifdef ADDITIONAL_DYNAMIC_BUFFERS_VALIDATION
  updated = true;
#endif
  D3D_CONTRACT_ASSERTF_RETURN(size_bytes + ofs_bytes <= bufSize, false, "size_bytes=%d ofs_bytes=%d bufSize=%d", size_bytes, ofs_bytes,
    bufSize);
  D3D_CONTRACT_ASSERT_RETURN(size_bytes != 0, false);
  if (device_is_lost != S_OK)
  {
    if (gpuAcquireRefCount && gpuThreadId == GetCurrentThreadId())
      D3D_ERROR("buffer lock in updateData during reset %s", getName());
    return false;
  }

  if (!ensureBufferCreated())
    return false;

  bool isCb = bufFlags & SBCF_BIND_CONSTANT;

  // dynamic and immutable are not updateable,
  // https://msdn.microsoft.com/en-us/library/windows/desktop/ff476486(v=vs.85).aspx
  // also seems like CPU writable is not updateable well too, despite no notes in docs
  // so use CPU writable condition for locking path
  if (isCPUWritable() || (lockFlags & (VBLOCK_NOOVERWRITE | VBLOCK_DISCARD)) || (isCb && ofs_bytes > 0))
    return updateDataWithLock(ofs_bytes, size_bytes, src, lockFlags);

  {
    ContextAutoLock contextLock;
    D3D11_BOX box;
    box.left = ofs_bytes;
    box.right = ofs_bytes + size_bytes;
    box.top = 0;
    box.bottom = 1;
    box.front = 0;
    box.back = 1;
    disable_conditional_render_unsafe();
    VALIDATE_GENERIC_RENDER_PASS_CONDITION(!g_render_state.isGenericRenderPassActive,
      "DX11: GenericBuffer::updateData uses UpdateSubresource inside a generic render pass");

    // https://learn.microsoft.com/en-us/windows/win32/api/d3d11/nf-d3d11-id3d11devicecontext-updatesubresource
    // For a shader-constant buffer; set pDstBox to NULL.
    // It is not possible to use this method to partially update a shader-constant buffer.
    dx_context->UpdateSubresource(buffer, 0, !isCb ? &box : nullptr, src, 0, 0);
    internalState = UPDATED_BY_COPYTO;
  }

  return true;
}

D3D11_MAP GenericBuffer::mapTypeFromFlags(int flags)
{
  if (flags & VBLOCK_DISCARD)
    return D3D11_MAP_WRITE_DISCARD;
  else if (flags & VBLOCK_NOOVERWRITE)
    return D3D11_MAP_WRITE_NO_OVERWRITE;

  if (flags & VBLOCK_WRITEONLY)
    return bufFlags & SBCF_DYNAMIC ? D3D11_MAP_WRITE_NO_OVERWRITE : D3D11_MAP_WRITE;
  else
    return flags & VBLOCK_READONLY ? D3D11_MAP_READ : D3D11_MAP_READ_WRITE;
}

bool GenericBuffer::ensureStagingBuffer()
{
  if (!stagingBuffer)
  {
    TIME_PROFILE(dx11_create_buffer_staging)
    createStagingBuffer(stagingBuffer);
    if (!stagingBuffer)
    {
      D3D_ERROR("Could not create staging buffer");
      return false;
    }
  }
  return true;
}

void GenericBuffer::releaseStagingBuffer()
{
  if (stagingBuffer)
  {
    TIME_PROFILE(dx11_release_buffer_staging)
    stagingBuffer->Release();
    stagingBuffer = NULL;
  }
}

void GenericBuffer::copyInternal(ID3D11Buffer *dst, ID3D11Buffer *src)
{
  ContextAutoLock contextLock;
  disable_conditional_render_unsafe();
  VALIDATE_GENERIC_RENDER_PASS_CONDITION(!g_render_state.isGenericRenderPassActive,
    "DX11: GenericBuffer::lock uses CopyResource inside a generic render pass");
  dx_context->CopyResource(dst, src);
}

bool GenericBuffer::ensureBufferCreated()
{
  if (buffer)
    return true;

  if (createBuf())
    return true;

  D3D_ERROR("can't create delayed buffer: name %s err %s", getName(), dx11_error(last_hres));
  return false;
}

void GenericBuffer::unmapBuffer(ID3D11Buffer *target_buf)
{
  ContextAutoLock contextLock;
  G_ASSERT(lockMsr.pData != NULL);
  dx_context->Unmap(target_buf, NULL);
}

bool GenericBuffer::isCPUWritable()
{
  // SBCF_CPU_ACCESS_WRITE does not make buffer writable! only dynamic and staging does! surprise!
  return bufFlags & SBCF_DYNAMIC || ((bufFlags & SBCF_STAGING_BUFFER) == SBCF_STAGING_BUFFER);
}

//  lock buffer; returns 0 on error
//  size_bytes==0 means entire buffer
int GenericBuffer::lock(unsigned ofs_bytes, unsigned size_bytes, void **ptr, int flags)
{
  LLLOG("vb-lock (%d; +%d [%d] %08x)", handle, ofs_bytes, size_bytes, flags);
#ifdef ADDITIONAL_DYNAMIC_BUFFERS_VALIDATION
  updated = true;
#endif
  checkLockParams(ofs_bytes, size_bytes, flags, bufFlags, getName(), bufSize);
  // implication ~A || B
  D3D_CONTRACT_ASSERTF(((flags & VBLOCK_DISCARD) == 0) || ((bufFlags & (SBCF_DYNAMIC | SBCF_BIND_CONSTANT)) != 0),
    "Buffer %s is not dynamic or constant, so discard update is forbidden for the buffer", getName());
  D3D_CONTRACT_ASSERTF(((flags & VBLOCK_NOOVERWRITE) == 0) || ((bufFlags & SBCF_DYNAMIC) != 0),
    "Buffer %s is not dynamic, so nooverwrite update is forbidden for the buffer", getName());
  D3D_CONTRACT_ASSERTF((flags & (VBLOCK_DISCARD | VBLOCK_NOOVERWRITE)) != (VBLOCK_DISCARD | VBLOCK_NOOVERWRITE),
    "Buffer %s can't be locked with discard and nooverwrite at same time");
  D3D_CONTRACT_ASSERTF(((flags & (VBLOCK_DISCARD | VBLOCK_NOOVERWRITE)) == 0) || ((flags & VBLOCK_WRITEONLY) != 0),
    "Buffer %s can't be locked for writing (VBLOCK_DISCARD or VBLOCK_NOOVERWRITE) without VBLOCK_WRITEONLY flag.", getName());
  D3D_CONTRACT_ASSERTF((flags & (VBLOCK_WRITEONLY | VBLOCK_READONLY)) != (VBLOCK_WRITEONLY | VBLOCK_READONLY),
    "Buffer %s can't be locked for RW with both *ONLY flags set, use 0 flag", getName());
  D3D_CONTRACT_ASSERTF_RETURN(ofs_bytes + size_bytes <= bufSize, 0, "Lock beyond the buffer end: %d + %d > %d", ofs_bytes, size_bytes,
    bufSize);

  if (size_bytes == 0)
    size_bytes = bufSize - ofs_bytes;
  D3D_CONTRACT_ASSERTF(size_bytes != 0, "Nothing to lock");

  lockMsr.pData = nullptr;

  if (device_is_lost != S_OK)
  {
    if (gpuAcquireRefCount && gpuThreadId == GetCurrentThreadId())
      D3D_ERROR("buffer lock during reset %s", getName());
    return 0;
  }

  ID3D11Buffer *bufferToMap = buffer;
  bool cpuReadable = bufFlags & SBCF_CPU_ACCESS_READ;
  bool cpuWritable = isCPUWritable();
  bool gpuWritable = uav;
  bool needStaging =
    // common "optimized" case that GPU writable resources are living on GPU and GPU only visible
    // also copying to staging is base requirements to non-blockingly get resources from GPU
    gpuWritable ||
    // when we ask to W/R/RW and don't have CPU W/R/RW access use staging
    (!cpuWritable && (flags & VBLOCK_WRITEONLY)) ||                                             //
    (!cpuReadable && (flags & VBLOCK_READONLY)) ||                                              //
    (!(cpuReadable && cpuWritable) && ((flags & (VBLOCK_WRITEONLY | VBLOCK_READONLY)) == 0)) || //
    // if buffer is not yet allocated, use staging for simplicity
    !buffer;

  if (needStaging)
  {
    if (!ensureStagingBuffer())
      return 0;
    bufferToMap = stagingBuffer;
  }

  if (bufferToMap == stagingBuffer)
  {
    // try to avoid useless readbacks if we can,
    // but always readback gpuWritable resources on readback-start lock because we don't track GPU writes now
    // note: VBLOCK_NOSYSLOCK with valid pointer will still read stale data because we can't differentiate
    // between initial lock and tries loop
    bool nonBlockingReadbackRequest = gpuWritable && !ptr;
    if ((!(flags & VBLOCK_WRITEONLY) && ((internalState != BUFFER_COPIED) || nonBlockingReadbackRequest)) ||
        (internalState == UPDATED_BY_COPYTO))
    {
      copyInternal(stagingBuffer, buffer);
      internalState = BUFFER_COPIED;
    }
  }

  if (!ptr)
    return 0;

  D3D11_MAP mapType = mapTypeFromFlags(flags);
  ContextAutoLock contextLock;
  // D3D11_MAP_FLAG_DO_NOT_WAIT cannot be used with D3D11_MAP_WRITE_DISCARD or D3D11_MAP_WRITE_NO_OVERWRITE.
  bool allowsNonBlocking = mapType != D3D11_MAP_WRITE_NO_OVERWRITE && mapType != D3D11_MAP_WRITE_DISCARD;
  bool requestedNonBlocking = (flags & VBLOCK_NOSYSLOCK);
#if DETECT_AND_PROFILE_BLOCKING_LOCK > 0
  bool shouldNonBlock = allowsNonBlocking;
#else
  bool shouldNonBlock = allowsNonBlocking && requestedNonBlocking;
#endif
  HRESULT hr = dx_context->Map(bufferToMap, NULL, mapType, shouldNonBlock ? D3D11_MAP_FLAG_DO_NOT_WAIT : 0, &lockMsr);
  // resource is in use by GPU now, process according to provided lock flags
  if (hr == DXGI_ERROR_WAS_STILL_DRAWING)
  {
    if (requestedNonBlocking)
      return 0;

#if DETECT_AND_PROFILE_BLOCKING_LOCK > 1
    D3D_ERROR("dx11: blocking buffer lock of %p:%s", this, getName());
#endif
#if DAGOR_DBGLEVEL > 0
    TIME_PROFILE_NAME(dx11_blocking_buffer_lock, String(64, "dx11_blocking_lock of %s", getName()));
#else
    TIME_PROFILE(dx11_blocking_buffer_lock);
#endif
    hr = dx_context->Map(bufferToMap, NULL, mapType, 0, &lockMsr);
  }

  internalState = (bufferToMap == stagingBuffer)
                    ? (!(flags & VBLOCK_READONLY) ? STAGING_BUFFER_LOCKED_SHOULD_BE_COPIED : STAGING_BUFFER_LOCKED)
                    : BUFFER_LOCKED;
#if DAGOR_DBGLEVEL > 0
  DXFATAL(hr, "Map failed, <%s> mapType %d", getName(), mapType);
#endif
  if (FAILED(hr))
    return 0;
  Stat3D::updateLockVIBuf();

  bool ret = lockMsr.pData != nullptr;
  // ptr guarantied to be valid due to early exit check
  if (ret)
    *ptr = (void *)((uint8_t *)lockMsr.pData + ofs_bytes);
  return ret ? 1 : 0;
}

int GenericBuffer::unlock()
{
  LLLOG("vb-unlk (%d)", handle);

  if (!ensureBufferCreated())
  {
    G_ASSERT(internalState == STAGING_BUFFER_LOCKED || internalState == STAGING_BUFFER_LOCKED_SHOULD_BE_COPIED);
    unmapBuffer(stagingBuffer);
    return 0;
  }

  if (internalState == BUFFER_LOCKED)
    unmapBuffer(buffer);
  else if (internalState == STAGING_BUFFER_LOCKED || internalState == STAGING_BUFFER_LOCKED_SHOULD_BE_COPIED)
  {
    unmapBuffer(stagingBuffer);
    if (internalState == STAGING_BUFFER_LOCKED_SHOULD_BE_COPIED)
      copyInternal(buffer, stagingBuffer);
  }
  else
    G_ASSERTF(0, "dx11: buffer %p:%s was unlocked with ill state %u", this, getName(), internalState);
  internalState = BUFFER_INVALID;
  lockMsr.pData = NULL;

  // due to fact that staging should be consitent between various locks we keep it allocated
  // TODO: try to deallocate it using some strong logic or non-fragile heuristic, if it necessary to conserve RAM

  return 1;
}

void GenericBuffer::setRld(Sbuffer::IReloadData *_rld)
{
  if (rld && rld != _rld)
  {
    rld->destroySelf();
  }

  rld = _rld;
}

void GenericBuffer::removeFromStates()
{
  bool forceResetActiveVertexInput = false;
  for (int i = 0; i < MAX_VERTEX_STREAMS; i++)
  {
    if (g_render_state.nextVertexInput.vertexStream[i].source == this)
    {
      D3D_CONTRACT_ERROR("Deleting buffer that is still in render '%s'", getName());
      d3d::setvsrc_ex(i, nullptr, 0, 0);
    }
    if (g_render_state.currVertexInput.vertexStream[i].source == this)
    {
      d3d::setvsrc_ex(i, nullptr, 0, 0);
      forceResetActiveVertexInput = true;
    }
  }

  if (g_render_state.nextVertexInput.indexBuffer == this)
  {
    D3D_CONTRACT_ERROR("Deleting buffer that is still in render '%s'", getName());
    d3d::setind(nullptr);
  }
  if (g_render_state.currVertexInput.indexBuffer == this)
  {
    d3d::setind(nullptr);
    forceResetActiveVertexInput = true;
  }

  if (forceResetActiveVertexInput)
    flush_buffers(g_render_state);

  if (buffer && (bufFlags & SBCF_BIND_CONSTANT))
  {
    ResAutoLock resLock; // Thredsafe state.resources access.
    for (int stage = 0; stage < g_render_state.constants.customBuffers.size(); ++stage)
      for (int slot = 0; slot < g_render_state.constants.customBuffers[stage].size(); ++slot)
      {
        if (g_render_state.constants.customBuffers[stage][slot] == buffer)
        {
          d3d::set_const_buffer(stage, slot, NULL);
        }
      }
  }
  if (buffer && (bufFlags & SBCF_BIND_SHADER_RES))
  {
    ResAutoLock resLock; // Thredsafe state.resources access.
    for (int stage = 0; stage < g_render_state.texFetchState.resources.size(); ++stage)
    {
      TextureFetchState::Resources &res = g_render_state.texFetchState.resources[stage];
      for (int slot = 0; slot < res.resources.size(); ++slot)
        if (res.resources[slot].buffer == this)
          res.resources[slot].setBuffer(NULL);
    }
  }
  if (uav)
  {
    ResAutoLock resLock; // Thredsafe state.resources access.
    for (int stage = 0; stage < g_render_state.texFetchState.uavState.size(); ++stage)
    {
      if (!g_render_state.texFetchState.uavState[stage].uavsUsed)
        continue;
      for (int slot = 0; slot < g_render_state.texFetchState.uavState[stage].uavs.size(); ++slot)
        if (g_render_state.texFetchState.uavState[stage].uavs[slot] == uav)
          g_render_state.texFetchState.setUav(stage, slot, nullptr);
    }
  }
}

bool GenericBuffer::createStagingBuffer(ID3D11Buffer *&out_buf)
{
  D3D11_BUFFER_DESC bd = {};
  bd.ByteWidth = (UINT)bufSize;
  bd.Usage = D3D11_USAGE_STAGING;
  bd.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
  return create_buffer(bd, getName(), nullptr, out_buf);
}

} // namespace drv3d_dx11
