// fields that define resource bindings
#pragma once
#include "util/tracked_state.h"
#include "driver.h"
#include "buffer_resource.h"

namespace drv3d_vulkan
{

struct BufferRef;
struct BaseTex;
class SBuffer;
#if D3D_HAS_RAY_TRACING
class RaytraceAccelerationStructure;
#endif

struct TRegister
{
  struct ImgBind
  {
    Image *ptr = nullptr;
    ImageViewState view = {};
  };
  union
  {
    ImgBind img;
    BufferRef buf;
#if D3D_HAS_RAY_TRACING
    RaytraceAccelerationStructure *rtas;
#endif
  };

  enum
  {
    TYPE_NULL = 0,
    TYPE_IMG = 1,
    TYPE_BUF = 2,
    TYPE_AS = 3
  };

  uint8_t type : 2;
  // keep it out of img struct to be more compact
  uint8_t isSwapchainColor : 1;
  uint8_t isSwapchainDepth : 1;

  void clear() { type = TYPE_NULL; }

  TRegister(Sbuffer *sb);
  TRegister(BaseTex *in_texture);
#if D3D_HAS_RAY_TRACING
  TRegister(RaytraceAccelerationStructure *in_as);
#endif

// MSVC decides that zero-init ctors in union is non-trivial, making itself unhappy about any ctors
// but code is initializing relevant union fields, so just silence this warning up
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4582)
#endif

  TRegister() : isSwapchainColor(0), isSwapchainDepth(0), type(TYPE_NULL){};

#ifdef _MSC_VER
// C4582 off
#pragma warning(pop)
#endif
};

inline bool operator!=(const TRegister &l, const TRegister &r)
{
  if (l.type != r.type)
    return true;

  switch (l.type)
  {
    case TRegister::TYPE_IMG:
      return (l.img.ptr != r.img.ptr) || (l.img.view != r.img.view) || (l.isSwapchainColor != r.isSwapchainColor) ||
             (l.isSwapchainDepth != r.isSwapchainDepth);
    case TRegister::TYPE_BUF: return (l.buf != r.buf);
#if D3D_HAS_RAY_TRACING
    case TRegister::TYPE_AS: return (l.rtas != r.rtas);
#endif
    default: return false;
  }
}

struct URegister
{
  Image *image = nullptr;
  ImageViewState imageView = {};
  BufferRef buffer;

  URegister() = default;
  URegister(BaseTexture *tex, uint32_t face, uint32_t mip_level, bool as_uint);
  URegister(Sbuffer *sb);
  void clear()
  {
    image = nullptr;
    imageView = {};
    buffer.clear();
  }
};

inline bool operator!=(const URegister &l, const URegister &r)
{
  return (l.image != r.image) || (l.imageView != r.imageView) || (l.buffer != r.buffer);
}

struct SRegister
{
  union
  {
    SamplerState state;
    const SamplerResource *resPtr;
  };

  enum
  {
    TYPE_NULL = 0,
    TYPE_RES = 1,
    TYPE_STATE = 2
  };
  uint8_t type : 2;

// MSVC decides that zero-init ctors in union is non-trivial, making itself unhappy about any ctors
// but code is initializing relevant union fields, so just silence this warning up
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4582)
#endif
  SRegister() : type(TYPE_NULL) {}
  SRegister(const SamplerResource *val) : type(TYPE_RES) { resPtr = val; }
  SRegister(SamplerState val) : type(TYPE_STATE) { state = val; }
#ifdef _MSC_VER
// C4582 off
#pragma warning(pop)
#endif
};

inline bool operator!=(const SRegister &l, const SRegister &r)
{
  if (l.type != r.type)
    return true;

  if (l.type == SRegister::TYPE_NULL)
    return false;
  else if (l.type == SRegister::TYPE_RES)
    return l.resPtr != r.resPtr;
  else if (l.type == SRegister::TYPE_STATE)
    return !(l.state == r.state);

  G_ASSERTF(0, "vulkan: uknown sreg type %u", l.type);
  return false;
}

struct StateFieldBRegister : TrackedStateFieldGenericPOD<BufferRef>
{
  using Indexed = TrackedStateIndexedDataRORef<BufferRef>;

  VULKAN_TRACKED_STATE_ARRAY_FIELD_CB_DEFENITIONS();
};

struct StateFieldTRegister : TrackedStateFieldGenericPOD<TRegister>
{
  using Indexed = TrackedStateIndexedDataRORef<TRegister>;

  // bool diff(const BufferRef& new_buf) const;
  // void set(const BufferRef& new_buf);

  VULKAN_TRACKED_STATE_ARRAY_FIELD_CB_DEFENITIONS();
};

struct StateFieldURegister : TrackedStateFieldGenericPOD<URegister>
{
  using Indexed = TrackedStateIndexedDataRORef<URegister>;

  VULKAN_TRACKED_STATE_ARRAY_FIELD_CB_DEFENITIONS();
};

struct StateFieldSRegister : TrackedStateFieldGenericPOD<SRegister>
{
  using Indexed = TrackedStateIndexedDataRORef<SRegister>;

  VULKAN_TRACKED_STATE_ARRAY_FIELD_CB_DEFENITIONS();
};

struct StateFieldBRegisterSet : TrackedStateFieldArray<StateFieldBRegister, spirv::B_REGISTER_INDEX_MAX, true, true>
{};

struct StateFieldTRegisterSet : TrackedStateFieldArray<StateFieldTRegister, spirv::T_REGISTER_INDEX_MAX, true, true>
{};

struct StateFieldURegisterSet : TrackedStateFieldArray<StateFieldURegister, spirv::U_REGISTER_INDEX_MAX, true, true>
{};

struct StateFieldSRegisterSet : TrackedStateFieldArray<StateFieldSRegister, spirv::S_REGISTER_INDEX_MAX, true, true>
{};

class ImmediateConstBuffer
{
  static constexpr int ring_size = FRAME_FRAME_BACKLOG_LENGTH;
  static constexpr int initial_blocks = 16384;
  static constexpr int element_size = MAX_IMMEDIATE_CONST_WORDS * sizeof(uint32_t);

  Buffer *ring[ring_size] = {};
  uint32_t ringIdx = 0;
  uint32_t offset = 0;
  uint32_t blockSize = 0;

  void flushWrites();

public:
  BufferRef push(const uint32_t *data);
  void onFlush();
  void shutdown(ExecutionContext &ctx);
};

// use fast no diff path for now
struct StateFieldImmediateConst : TrackedStateFieldBase<false, false>
{
  struct SrcData
  {
    uint8_t count;
    const uint32_t *data;
  };

  bool enabled;
  uint32_t data[4];

  template <typename StorageType>
  void reset(StorageType &)
  {
    enabled = false;
  }

  void set(SrcData value)
  {
    G_ASSERT(value.count <= MAX_IMMEDIATE_CONST_WORDS);
    G_ASSERT(value.data || !value.count);
    enabled = value.count > 0;
    if (enabled)
      memcpy(data, value.data, value.count * sizeof(uint32_t));
  }
  bool diff(SrcData) const { return true; }

  VULKAN_TRACKED_STATE_FIELD_CB_DEFENITIONS();
};

struct StateFieldGlobalConstBuffer : TrackedStateFieldBase<true, true>, TrackedStateFieldGenericPOD<BufferRef>
{
  VULKAN_TRACKED_STATE_FIELD_CB_DEFENITIONS();
};

struct StateFieldResourceBindsStorage
{
  StateFieldGlobalConstBuffer globalConstBuf;
  StateFieldImmediateConst immConst;
  StateFieldURegisterSet uRegs;
  StateFieldTRegisterSet tRegs;
  StateFieldBRegisterSet bRegs;
  StateFieldSRegisterSet sRegs;
  ShaderStage stage;

  void reset() {}
  void dumpLog() const { debug("StateFieldResourceBindsStorage end"); }

  VULKAN_TRACKED_STATE_STORAGE_CB_DEFENITIONS();
};

class StateFieldResourceBinds
  : public TrackedState<StateFieldResourceBindsStorage, StateFieldURegisterSet, StateFieldTRegisterSet, StateFieldBRegisterSet,
      StateFieldSRegisterSet, StateFieldImmediateConst, StateFieldGlobalConstBuffer>
{
public:
  template <typename T>
  bool handleObjectRemoval(T object);

  template <typename T>
  bool isReferenced(T object) const;

  template <typename T, typename B>
  bool replaceResource(T old_obj, const B &new_obj, uint32_t flags);

  VULKAN_TRACKED_STATE_DEFAULT_NESTED_FIELD_CB_NO_RESET();
};

template <ShaderStage Stage>
class StateFieldStageResourceBinds : public StateFieldResourceBinds
{
public:
  template <typename T>
  void reset(T &)
  {
    getData().stage = Stage;
    TrackedState::reset();
  }

  StateFieldResourceBinds &getValue() { return (StateFieldResourceBinds &)*this; }
};

} // namespace drv3d_vulkan