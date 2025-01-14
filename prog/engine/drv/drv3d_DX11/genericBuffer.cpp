// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "validation.h"

#include "driver.h"
#include "buffers.h"
#include <drv/3d/dag_vertexIndexBuffer.h>
#include <drv/3d/dag_shaderConstants.h>

static void set_buffer_name(ID3D11Buffer *buffer, const char *name)
{
#if DAGOR_DBGLEVEL > 0
  if (buffer)
    if (auto len = i_strlen(name))
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
  // if (sysMemBuf)
  //   memfree(sysMemBuf, tmpmem);
  removeFromStates();
  // release calls 'ressize' virtual function indirectly. It is not recommended from desctructor
  // but GenericBuffer is final class, so it should be fine
  static_assert(std::is_final<GenericBuffer>());
  release(); // -V1053
  setRld(NULL);
}

bool GenericBuffer::createBuf()
{
  if ((bufFlags & SBCF_CPU_ACCESS_MASK) == (SBCF_CPU_ACCESS_READ | SBCF_CPU_ACCESS_WRITE) && !bindFlags && !miscFlags)
  {
    return createStagingBuffer(buffer);
  }
  D3D11_BUFFER_DESC bd = {};
  bd.ByteWidth = (UINT)bufSize;
  if ((bufFlags & SBCF_DYNAMIC) && bufSize >= (16 << 20))
    logwarn("Potential performance issue. Dynamic buffer <%s> of size bigger than 16mb", getResName());

  bd.Usage = (bufFlags & SBCF_DYNAMIC) ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;
  bd.BindFlags = bindFlags;
  bd.MiscFlags = miscFlags;
  bd.CPUAccessFlags = (bufFlags & SBCF_CPU_ACCESS_MASK) << 2;
  if ((bufFlags & SBCF_DYNAMIC))
    bd.CPUAccessFlags |= D3D11_CPU_ACCESS_WRITE;
  else
    bd.CPUAccessFlags &= (~D3D11_CPU_ACCESS_WRITE);
  if (bufFlags & SBCF_MISC_STRUCTURED)
    bd.StructureByteStride = structSize;
  if (format)
  {
    G_ASSERT(dxgi_format_to_bpp(dxgi_format_from_flags(format)) == structSize * 8);
  }
  G_ASSERTF(!(bufFlags & SBCF_BIND_CONSTANT) || bufSize <= 65536,
    "Windows 7 and earlier can not create cb of size more than 64k. %s is %d", getResName(), bufSize);

  G_ASSERTF(!(bd.CPUAccessFlags & D3D11_CPU_ACCESS_WRITE) || bd.Usage == D3D11_USAGE_DYNAMIC,
    "CPU write access is only supported for dynamic buffers and staging buffers. %s has usage 0x%x", getResName(), bd.Usage);

  G_ASSERTF(!(bd.CPUAccessFlags & D3D11_CPU_ACCESS_READ), "CPU read access is only supported for staging buffers. %s has usage 0x%x",
    getResName(), bd.Usage);

  if (bufFlags & SBCF_ZEROMEM && !sysMemBuf)
    return create_zeromem_buffer(bd, getResName(), buffer);
  else
    return create_buffer(bd, getResName(), sysMemBuf, buffer);
}

bool GenericBuffer::createSrv()
{
  int elements = bufSize / structSize;
  D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc;
  ZeroMemory(&srv_desc, sizeof(srv_desc));
  if (bufFlags & SBCF_MISC_ALLOW_RAW)
  {
    G_ASSERT(structSize == 4);
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
        D3D_ERROR("The structure size of %u of buffer %s has a hardware unfriendly size and "
                  "probably results in degraded performance. For some platforms it might requires "
                  "the shader compiler to restructure the structure type to avoid layout violations "
                  "and on other platforms it results in wasted memory as the memory manager has to "
                  "insert extra padding to align the buffer properly.",
          structSize, getResName());
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
      if (!device_should_reset(last_hres, "CreateShaderResourceView"))
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
    D3D_ERROR("getSrv on unstructured buffer");
    return NULL;
  }

  createSrv();
  return srv;
}

bool GenericBuffer::createUav()
{
  G_ASSERT(bufFlags & SBCF_BIND_UNORDERED);
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
        D3D_ERROR("The structure size of %u of buffer %s has a hardware unfriendly size and "
                  "probably results in degraded performance. For some platforms it might requires "
                  "the shader compiler to restructure the structure type to avoid layout violations "
                  "and on other platforms it results in wasted memory as the memory manager has to "
                  "insert extra padding to align the buffer properly.",
          structSize, getResName());
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

void GenericBuffer::setBufName(const char *name) { setResName(name); }

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
  TEXQL_ON_BUF_ALLOC(this);

  if (buf_flags & SBCF_DYNAMIC)
  {
    return createBuf();
  }

  return true; // delayed create
}

bool GenericBuffer::createStructured(uint32_t struct_size, uint32_t elements, uint32_t buf_flags, uint32_t buf_format,
  const char *statName_)
{
  G_ASSERT(struct_size == 4 || !(buf_flags & SBCF_MISC_ALLOW_RAW));
  if (bool(buf_flags & SBCF_MISC_STRUCTURED) && bool(buf_flags & SBCF_MISC_DRAWINDIRECT))
  {
    D3D_ERROR("indirect buffer can't be structured one, check <%s>", statName_);
    G_ASSERTF(0, "indirect buffer can't be structured one, <%s>", statName_);
  }
  G_ASSERTF(buf_format == 0 || ((buf_flags & (SBCF_BIND_VERTEX | SBCF_BIND_INDEX | SBCF_MISC_STRUCTURED | SBCF_MISC_ALLOW_RAW)) == 0),
    "can't create buffer with texture format which is Structured, Vertex, Index or Raw");
  this->bufFlags = buf_flags;
  this->bufSize = struct_size * elements;
  this->structSize = struct_size;
  this->format = buf_format;
  this->bindFlags = (buf_flags & SBCF_BIND_MASK) >> 16;
  this->miscFlags = (buf_flags & SBCF_MISC_MASK) >> 20;
  setBufName(statName_);
  TEXQL_ON_BUF_ALLOC(this);

  return createBuf();
}

bool GenericBuffer::recreateBuf(Sbuffer *sb)
{
  bool result = true;
  TEXQL_ON_BUF_ALLOC(this);
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
    TEXQL_ON_BUF_ALLOC(this);
    rld->reloadD3dRes(sb);
    return true;
  }

  if (!systemCopy)
  {
    return true;
  }

  TEXQL_ON_BUF_ALLOC(this);
  lock(0, bufSize, VBLOCK_WRITEONLY);
  unlock();

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
    TEXQL_ON_BUF_RELEASE(this);
    buffer->Release();
    buffer = NULL;
  }

  if (stagingBuffer)
  {
    stagingBuffer->Release();
    stagingBuffer = NULL;
  }

  if (!systemCopy && sysMemBuf)
  {
    delete[] sysMemBuf;
  }
  sysMemBuf = NULL;

  if (resetting_device_now)
  {
    return;
  }

  if (systemCopy)
  {
    delete[] systemCopy;
  }

  systemCopy = NULL;
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
  ContextAutoLock contextLock;
  VALIDATE_GENERIC_RENDER_PASS_CONDITION(!g_render_state.isGenericRenderPassActive,
    "DX11: GenericBuffer::copyTo uses CopyResource inside a generic render pass");
  disable_conditional_render_unsafe();
  dx_context->CopyResource(destvb->buffer, buffer);
  destvb->internalState = UPDATED_BY_COPYTO;
  return true;
}

bool GenericBuffer::updateData(uint32_t ofs_bytes, uint32_t size_bytes, const void *__restrict src, uint32_t lockFlags)
{
#ifdef ADDITIONAL_DYNAMIC_BUFFERS_VALIDATION
  updated = true;
#endif
  G_ASSERTF_RETURN(size_bytes + ofs_bytes <= bufSize, false, "size_bytes=%d ofs_bytes=%d bufSize=%d", size_bytes, ofs_bytes, bufSize);
  G_ASSERT_RETURN(size_bytes != 0, false);
  if (device_is_lost != S_OK)
  {
    if (gpuAcquireRefCount && gpuThreadId == GetCurrentThreadId())
      D3D_ERROR("buffer lock in updateData during reset %s", getResName());
    return false;
  }
  if (!buffer && (bufFlags & SBCF_DYNAMIC) == 0)
    createBuf();
  if (!buffer)
  {
    if ((bufFlags & SBCF_DYNAMIC) == 0)
      return updateDataWithLock(ofs_bytes, size_bytes, src, lockFlags); // case of delayed create.
    D3D_ERROR("buffer not created %s", getResName());
    return false;
  }
  if ((bufFlags &
        (SBCF_DYNAMIC | SBCF_CPU_ACCESS_WRITE)) || // dynamic and immutable are not updateable,
                                                   // https://msdn.microsoft.com/en-us/library/windows/desktop/ff476486(v=vs.85).aspx
      (lockFlags & (VBLOCK_NOOVERWRITE | VBLOCK_DISCARD)))
  {
    return updateDataWithLock(ofs_bytes, size_bytes, src, lockFlags);
  }
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
  dx_context->UpdateSubresource(buffer, 0, &box, src, 0, 0);
  return true;
}
int GenericBuffer::lock(unsigned ofs_bytes, unsigned size_bytes, void **ptr, int flags)
{
#ifdef ADDITIONAL_DYNAMIC_BUFFERS_VALIDATION
  updated = true;
#endif
  LLLOG("vb-lock (%d; +%d [%d] %08x)", handle, ofs_bytes, size_bytes, flags);
  checkLockParams(ofs_bytes, size_bytes, flags, bufFlags);
  if (!ptr)
  {
    lock(ofs_bytes, size_bytes, VBLOCK_DELAYED);
    return 1;
  }
  else
  {
    *ptr = lock(ofs_bytes, size_bytes, flags);
    return *ptr ? 1 : 0;
  }
}

// TODO: support VBLOCK_WRITEONLY
//  lock buffer; returns 0 on error
//  size_bytes==0 means entire buffer
void *GenericBuffer::lock(uint32_t ofs_bytes, uint32_t size_bytes, int flags)
{
#ifdef ADDITIONAL_DYNAMIC_BUFFERS_VALIDATION
  updated = true;
#endif
  // implication ~A || B
  G_ASSERT(((flags & VBLOCK_DISCARD) == 0) || ((bufFlags & (SBCF_DYNAMIC | SBCF_BIND_CONSTANT)) != 0)); // only dynamic buffers can be
                                                                                                        // locked with
                                                                                                        // VBLOCK_NOOVERWRITE, and only
                                                                                                        // dynamic or constant with
                                                                                                        // discard
  G_ASSERT(((flags & (VBLOCK_DISCARD | VBLOCK_NOOVERWRITE)) == 0) || ((flags & VBLOCK_WRITEONLY) != 0)); // it is stupid to lock for
                                                                                                         // write without writeonly
  if (bufSize == 0)
  {
    return NULL;
  }

  if (ofs_bytes + size_bytes > bufSize)
  {
    G_ASSERT_LOG(0, "[E] Lock beyond the buffer end: %d + %d > %d", ofs_bytes, size_bytes, bufSize);
    return NULL;
  }

  if (size_bytes == 0)
  {
    size_bytes = bufSize - ofs_bytes;
    G_ASSERT(size_bytes != 0); // nothing to lock
  }

  if (device_is_lost != S_OK)
  {
    if (gpuAcquireRefCount && gpuThreadId == GetCurrentThreadId())
      D3D_ERROR("buffer lock during reset %s", getResName());
    return NULL;
  }

  ID3D11Buffer *bufferToLock = buffer;
  D3D11_MAP mapType = D3D11_MAP_WRITE;
  // G_ASSERT(uav || !structSize);
  if (uav || (structSize && !(bufFlags & SBCF_DYNAMIC))) //==
  {
    if (
      ((bufFlags & SBCF_CPU_ACCESS_MASK) == SBCF_CPU_ACCESS_MASK) || ((bufFlags & SBCF_CPU_ACCESS_READ) && (flags & VBLOCK_READONLY)))
    {
      // bufferToLock = buffer;
    }
    else
    {
      if (!stagingBuffer)
      {
        createStagingBuffer(stagingBuffer);
        if (!stagingBuffer)
        {
          D3D_ERROR("Could not create staging buffer");
          return nullptr;
        }
      }
      bufferToLock = stagingBuffer;
    }
    if (flags & VBLOCK_DISCARD)
    {
      mapType = D3D11_MAP_WRITE_DISCARD;
    }
    else if (flags & VBLOCK_NOOVERWRITE)
    {
      mapType = D3D11_MAP_WRITE_NO_OVERWRITE;
    }
    else
    {
      mapType = (flags & VBLOCK_WRITEONLY) ? D3D11_MAP_WRITE : (flags & VBLOCK_READONLY) ? D3D11_MAP_READ : D3D11_MAP_READ_WRITE;
    }
    lockMsr.pData = NULL;
  }
  else
  {
    if (buffer && !(bufFlags & SBCF_DYNAMIC))
    {
      buffer->Release();
      buffer = NULL;
      bufFlags |= SBCF_DYNAMIC;
      if (!systemCopy)
        systemCopy = new char[bufSize];
      logwarn("Locked an immutable buffer: '%s' ", getResName());
    }

    if (buffer == NULL && ((bufFlags & SBCF_DYNAMIC) == 0)) // not allocated?
    {
      if (sysMemBuf == NULL)
      {
        if (!systemCopy)
        {
          sysMemBuf = new char[bufSize];
        }
        else
        {
          sysMemBuf = systemCopy;
        }
      }

      return (void *)((uint8_t *)sysMemBuf + ofs_bytes);
    }

    G_ASSERT((flags & (VBLOCK_DISCARD | VBLOCK_NOOVERWRITE)) != (VBLOCK_DISCARD | VBLOCK_NOOVERWRITE));
    if ((flags & VBLOCK_READONLY) || systemCopy)
    {
      lockMsr.pData = NULL;
      if (!(flags & VBLOCK_READONLY))
      {
        sysMemBuf = systemCopy;
      }

      if (systemCopy)
      {
        return systemCopy + ofs_bytes;
      }

      return NULL;
    }

    if (flags & VBLOCK_WRITEONLY && bufFlags & SBCF_DYNAMIC)
    {
      mapType = D3D11_MAP_WRITE_NO_OVERWRITE;
    }

    if (flags & VBLOCK_DISCARD)
    {
      mapType = D3D11_MAP_WRITE_DISCARD;
    }
    else if (flags & VBLOCK_NOOVERWRITE)
    {
      mapType = D3D11_MAP_WRITE_NO_OVERWRITE;
    }
  }

  if (bufferToLock == stagingBuffer)
  {
    // clang-format off
    if ( (!(flags & VBLOCK_WRITEONLY) && ((internalState != BUFFER_COPIED) || (flags & VBLOCK_DELAYED)))
      || (internalState == UPDATED_BY_COPYTO) )
    {
      ContextAutoLock contextLock;
      disable_conditional_render_unsafe();
      VALIDATE_GENERIC_RENDER_PASS_CONDITION(!g_render_state.isGenericRenderPassActive,
    "DX11: GenericBuffer::lock uses CopyResource inside a generic render pass");
      dx_context->CopyResource(stagingBuffer, buffer);
      internalState = BUFFER_COPIED;
    }
    if (flags & VBLOCK_DELAYED)
    {
      return NULL;
    }
    // clang-format on
  }


  ContextAutoLock contextLock;
  HRESULT hr = dx_context->Map(bufferToLock, NULL, mapType, 0, &lockMsr);

  internalState = (bufferToLock == stagingBuffer)
                    ? (!(flags & VBLOCK_READONLY) ? STAGING_BUFFER_LOCKED_SHOULD_BE_COPIED : STAGING_BUFFER_LOCKED)
                    : BUFFER_LOCKED;
#if DAGOR_DBGLEVEL > 0
  DXFATAL(hr, "Map failed, <%s> mapType %d", getResName(), mapType);
#endif
  if (FAILED(hr))
    return nullptr;
  Stat3D::updateLockVIBuf();
  return lockMsr.pData ? (void *)((uint8_t *)lockMsr.pData + ofs_bytes) : NULL;
}

int GenericBuffer::unlock()
{
  LLLOG("vb-unlk (%d)", handle);

  if (sysMemBuf)
  {
    if (!buffer)
    {
      if (!createBuf())
        D3D_ERROR("can't create delayed buffer: %s", dx11_error(last_hres));
    }
    else
    {
      G_ASSERTF(bufFlags & (SBCF_DYNAMIC | SBCF_CPU_ACCESS_WRITE), "Cannot lock buffer '%s', because it has no cpu write access!",
        getResName());

      ContextAutoLock contextLock;
      HRESULT hr = dx_context->Map(buffer, NULL, D3D11_MAP_WRITE_DISCARD, 0, &lockMsr);
      DXFATAL(hr, "Sys Map failed");
      memcpy(lockMsr.pData, sysMemBuf, bufSize);
      dx_context->Unmap(buffer, NULL);
      internalState = BUFFER_INVALID;
      lockMsr.pData = NULL;
    }
    if (!systemCopy && sysMemBuf)
      delete[] ((char *)sysMemBuf);
    sysMemBuf = NULL;
    setRld(rld); // force memory release for reloadable buffers
  }
  else if (uav || (structSize && !(bufFlags & SBCF_DYNAMIC)))
  {
    G_ASSERT(stagingBuffer || (bufFlags & SBCF_CPU_ACCESS_MASK));
    ContextAutoLock contextLock;
    if (internalState == BUFFER_LOCKED)
    {
      G_ASSERT(lockMsr.pData != NULL);
      dx_context->Unmap(buffer, NULL);
      internalState = BUFFER_INVALID;
    }
    else if (internalState == STAGING_BUFFER_LOCKED || internalState == STAGING_BUFFER_LOCKED_SHOULD_BE_COPIED)
    {
      G_ASSERT(lockMsr.pData != NULL);
      dx_context->Unmap(stagingBuffer, NULL);
      if (internalState == STAGING_BUFFER_LOCKED_SHOULD_BE_COPIED)
      {
        disable_conditional_render_unsafe();
        VALIDATE_GENERIC_RENDER_PASS_CONDITION(!g_render_state.isGenericRenderPassActive,
          "DX11: GenericBuffer::unlock uses CopyResource inside a generic render pass");
        dx_context->CopyResource(buffer, stagingBuffer); // only if locked for writing
      }
      internalState = BUFFER_INVALID;
    }
  }
  else if (bufFlags & SBCF_DYNAMIC && lockMsr.pData)
  {
    G_ASSERT(buffer);
    ContextAutoLock contextLock;
    G_ASSERT(internalState == BUFFER_LOCKED);
    dx_context->Unmap(buffer, NULL);
    internalState = BUFFER_INVALID;
  }

  lockMsr.pData = NULL;

  return 1;
}

void GenericBuffer::setRld(Sbuffer::IReloadData *_rld)
{
  if (rld && rld != _rld)
  {
    rld->destroySelf();
  }

  rld = _rld;

  if (systemCopy && rld)
  {
    delete[] systemCopy;
    systemCopy = NULL;
  }
}

void GenericBuffer::removeFromStates()
{
  for (int i = 0; i < MAX_VERTEX_STREAMS; i++)
  {
    if (g_render_state.nextVertexInput.vertexStream[i].source == this)
    {
      D3D_ERROR("Deleting buffer that is still in render '%s'", getResName());
      d3d::setvsrc_ex(i, nullptr, 0, 0);
    }
  }

  if (g_render_state.nextVertexInput.indexBuffer == this)
  {
    D3D_ERROR("Deleting buffer that is still in render '%s'", getResName());
    d3d::setind(nullptr);
  }
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
      TextureFetchState::Samplers &res = g_render_state.texFetchState.resources[stage];
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
  return create_buffer(bd, getResName(), nullptr, out_buf);
}

} // namespace drv3d_dx11
