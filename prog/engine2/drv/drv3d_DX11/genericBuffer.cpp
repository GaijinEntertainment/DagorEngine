#include <3d/dag_drv3dCmd.h>
#include "validation.h"

#include "driver.h"
#include "buffers.h"
#include "platformDependent.h"

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

bool GenericBuffer::createBuf(bool delayed)
{
  D3D11_BUFFER_DESC bd;
  ZeroMemory(&bd, sizeof(bd));
  bd.ByteWidth = (UINT)bufSize;
  if ((bufFlags & SBCF_DYNAMIC) && bufSize >= (16 << 20))
    logwarn("Potential performance issue. Dynamic buffer <%s> of size bigger than 16mb", getResName());

  bd.Usage = (bufFlags & SBCF_DYNAMIC) ? D3D11_USAGE_DYNAMIC : (delayed ? D3D11_USAGE_IMMUTABLE : D3D11_USAGE_DEFAULT); // requires
                                                                                                                        // init
  bd.BindFlags = bindFlags;
  bd.MiscFlags = miscFlags;

  UINT access = 0;
  if (bufFlags & SBCF_DYNAMIC)
  {
    access |= D3D11_CPU_ACCESS_WRITE;
  }

  bd.CPUAccessFlags = (D3D11_CPU_ACCESS_FLAG)access;

  D3D11_SUBRESOURCE_DATA idata;
  eastl::vector<unsigned char> zeroes;
  if (bufFlags & SBCF_ZEROMEM && !sysMemBuf)
  {
    zeroes.resize(bufSize, 0);
    idata.pSysMem = (void *)zeroes.data();
  }
  else
  {
    idata.pSysMem = (void *)sysMemBuf;
  }
  idata.SysMemPitch = 0;
  idata.SysMemSlicePitch = 0;

  if ((bd.Usage == D3D11_USAGE_IMMUTABLE) && sysMemBuf == NULL) // could not alloc?
  {
    return false;
  }

  if (device_is_lost == S_OK)
  {
    last_hres = dx_device->CreateBuffer(&bd, sysMemBuf ? &idata : NULL, &buffer);
    if (FAILED(last_hres))
    {
      if (!device_should_reset(last_hres, "CreateBuffer"))
      {
        DXFATAL(last_hres, "CreateBuffer");
        return 0;
      }
    }
  }

#if DAGOR_DBGLEVEL > 0
  if (stagingBuffer)
    if (auto len = i_strlen(getResName()))
      stagingBuffer->SetPrivateData(WKPDID_D3DDebugObjectName, len, getResName());
  if (buffer)
    if (auto len = i_strlen(getResName()))
      buffer->SetPrivateData(WKPDID_D3DDebugObjectName, len, getResName());
#endif

  if (!RESET_SUPPORTED && systemCopy)
  {
    delete[] systemCopy;
    systemCopy = NULL;
    sysMemBuf = NULL;
  }

  return true;
}

bool GenericBuffer::createSBuf(int struct_size, int elements, bool staging, ID3D11Buffer *&outBuf)
{
  if ((bufFlags & SBCF_CPU_ACCESS_MASK) == (SBCF_CPU_ACCESS_READ | SBCF_CPU_ACCESS_WRITE) && !bindFlags && !miscFlags)
  {
    staging = true; // it is staging buffer!
  }
  D3D11_BUFFER_DESC bd;
  ZeroMemory(&bd, sizeof(bd));
  bd.ByteWidth = (UINT)bufSize;
  bd.Usage = staging ? D3D11_USAGE_STAGING : ((bufFlags & SBCF_DYNAMIC) ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT);
  if ((bufFlags & SBCF_DYNAMIC) && bufSize >= (16 << 20))
    logwarn("Potential performance issue. Dynamic buffer <%s> of size bigger than 16mb", getResName());
  bd.BindFlags = staging ? 0 : bindFlags;
  bd.CPUAccessFlags = staging ? (D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE) : ((bufFlags & SBCF_CPU_ACCESS_MASK) << 2);
  if ((bufFlags & SBCF_DYNAMIC))
    bd.CPUAccessFlags |= D3D11_CPU_ACCESS_WRITE;
  else if (!staging)
    bd.CPUAccessFlags &= (~D3D11_CPU_ACCESS_WRITE);
  bd.MiscFlags = staging ? 0 : miscFlags;
  bd.StructureByteStride = (bufFlags & SBCF_MISC_STRUCTURED) ? struct_size : 0;
  if (!(bufFlags & SBCF_MISC_STRUCTURED) && format)
  {
    G_ASSERT(dxgi_format_to_bpp(dxgi_format_from_flags(format)) == struct_size * 8);
  }
  G_ASSERTF(!(bufFlags & SBCF_BIND_CONSTANT) || bufSize <= 65536,
    "Windows 7 and earlier can not create cb of size more than 64k. %s is %d", getResName(), bufSize);

  G_ASSERTF(!(bd.CPUAccessFlags & D3D11_CPU_ACCESS_WRITE) || bd.Usage == D3D11_USAGE_DYNAMIC || bd.Usage == D3D11_USAGE_STAGING,
    "CPU write access is only supported for dynamic buffers and staging buffers. %s has usage 0x%x", getResName(), bd.Usage);

  G_ASSERTF(!(bd.CPUAccessFlags & D3D11_CPU_ACCESS_READ) || bd.Usage == D3D11_USAGE_STAGING,
    "CPU read access is only supported for staging buffers. %s has usage 0x%x", getResName(), bd.Usage);

  if (device_is_lost == S_OK)
  {
    HRESULT hres = d3d::DagorCreateBuffer(bd, outBuf, *this);
    if (FAILED(hres))
    {
      if (!device_should_reset(hres, "CreateSBuffer"))
      {
        DXFATAL(hres, "CreateSBuffer %s: size = %d usage = %d bind = %d acces=%d, misc=%d structSize = %d", getResName(), bd.ByteWidth,
          bd.Usage, bd.BindFlags, bd.CPUAccessFlags, bd.MiscFlags, bd.StructureByteStride);
        return 0;
      }
    }
  }
#if DAGOR_DBGLEVEL > 0
  if (buffer)
    if (auto len = i_strlen(getResName()))
      buffer->SetPrivateData(WKPDID_D3DDebugObjectName, len, getResName());
#endif

  return true;
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
        logerr("The structure size of %u of buffer %s has a hardware unfriendly size and "
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
    logerr("getSrv on unstructured buffer");
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
        logerr("The structure size of %u of buffer %s has a hardware unfriendly size and "
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
    return createBuf(false);
  }
  if ((buf_flags & SBCF_BIND_UNORDERED) && !(buf_flags & SBCF_MAYBELOST))
  {
#if DAGOR_DBGLEVEL > 0
    logerr("buffer <%s> with unordered access view is required to be created with system copy."
           " Doesn't make any sense, add SBCF_MAYBELOST",
      getResName());
#endif
    buf_flags |= SBCF_MAYBELOST;
  }

  if (RESET_SUPPORTED && !(buf_flags & SBCF_MAYBELOST))
    systemCopy = new char[bufsize];

  return true; // delayed create
}

bool GenericBuffer::createStructured(uint32_t struct_size, uint32_t elements, uint32_t buf_flags, uint32_t buf_format,
  const char *statName_)
{
  G_ASSERT(struct_size == 4 || !(buf_flags & SBCF_MISC_ALLOW_RAW));
  if (bool(buf_flags & SBCF_MISC_STRUCTURED) && bool(buf_flags & SBCF_MISC_DRAWINDIRECT))
  {
    logerr("indirect buffer can't be structured one, check <%s>", statName_);
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

  return createSBuf(struct_size, elements, false, buffer);
}

bool GenericBuffer::recreateBuf(Sbuffer *sb)
{
  G_ASSERT(RESET_SUPPORTED);

  bool result = true;
  TEXQL_ON_BUF_ALLOC(this);
  if (bufFlags & (SBCF_BIND_INDEX | SBCF_BIND_VERTEX))
  {
    if (bufFlags & (SBCF_DYNAMIC | SBCF_MAYBELOST))
      result |= createBuf(false);
  }
  else
  {
    result |= createSBuf(structSize, bufSize / structSize, false, buffer); // Create resource before UAV.
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
  G_ASSERTF(((dest->getFlags() & SBCF_BIND_MASK) != SBCF_BIND_VERTEX && (dest->getFlags() & SBCF_BIND_MASK) != SBCF_BIND_INDEX) ||
              (dest->getFlags() & SBCF_MAYBELOST), // it is valid to copy to non-immutable buffer
    "destination index/vertex buffer is immutable");
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
  disable_conditional_render_unsafe();
  dx_context->CopySubresourceRegion(destvb->buffer, 0, dst_ofs_bytes, 0, 0, buffer, 0, &box);
  return true;
}
bool GenericBuffer::copyTo(Sbuffer *dest)
{
  if (!dest)
    return false;
  GenericBuffer *destvb = (GenericBuffer *)dest;
  G_ASSERTF((dest->getFlags() & SBCF_BIND_MASK) != SBCF_BIND_VERTEX && (dest->getFlags() & SBCF_BIND_MASK) != SBCF_BIND_INDEX,
    "destination index/vertex buffer can be immutable");
  if (!destvb->buffer)
    return false;
  ContextAutoLock contextLock;
  disable_conditional_render_unsafe();
  dx_context->CopyResource(destvb->buffer, buffer);
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
      logerr("buffer lock in updateData during reset %s", getResName());
    return false;
  }
  if (!buffer && (bufFlags & SBCF_DYNAMIC) == 0 && (bufFlags & SBCF_MAYBELOST))
    createBuf(false);
  if (!buffer)
  {
    if ((bufFlags & SBCF_DYNAMIC) == 0)
      return updateDataWithLock(ofs_bytes, size_bytes, src, lockFlags); // case of delayed create. todo: we can also optimize rare case
                                                                        // of SBCF_MAYBELOST, and not create system copy
    logerr("buffer not created %s", getResName());
    return false;
  }
  if ((bufFlags &
        (SBCF_DYNAMIC | SBCF_CPU_ACCESS_WRITE)) || // dynamic and immutable are not updateable,
                                                   // https://msdn.microsoft.com/en-us/library/windows/desktop/ff476486(v=vs.85).aspx
      (lockFlags & (VBLOCK_NOOVERWRITE | VBLOCK_DISCARD)) ||
      !(bufFlags & SBCF_MAYBELOST)) // if it is not maybelost, we can't lost data on reset and have to update with sysmem copy
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
      logerr("buffer lock during reset %s", getResName());
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
        createSBuf(structSize, bufSize / structSize, true, stagingBuffer);
        if (!stagingBuffer)
        {
          logerr("Could not create staging buffer");
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
    if (!systemCopy && buffer && !(bufFlags & SBCF_DYNAMIC))
    {
      buffer->Release();
      buffer = NULL;
      bufFlags |= SBCF_DYNAMIC;
      systemCopy = new char[bufSize];
      logwarn("was immutable buffer");
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
    if (!(flags & VBLOCK_WRITEONLY) && ((internalState != BUFFER_COPIED) || (flags & VBLOCK_DELAYED)))
    {
      ContextAutoLock contextLock;
      disable_conditional_render_unsafe();
      dx_context->CopyResource(stagingBuffer, buffer);
      internalState = BUFFER_COPIED;
    }
    if (flags & VBLOCK_DELAYED)
    {
      return NULL;
    }
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
      if (!createBuf(true))
        logerr("can't create immutable buffer: %s", dx11_error(last_hres));
    }
    else
    {
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
      logerr("Deleting buffer that is still in render '%s'", getResName());
      d3d::setvsrc_ex(i, nullptr, 0, 0);
    }
  }

  if (g_render_state.nextVertexInput.indexBuffer == this)
  {
    logerr("Deleting buffer that is still in render '%s'", getResName());
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

} // namespace drv3d_dx11
