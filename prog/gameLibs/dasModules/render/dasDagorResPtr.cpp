#include <dasModules/dasDagorResPtr.h>


// These types are supposed to be trivially copyable/destructible for
// das bindings to work. And in general, we want them to be lightweight
// (ptr, id) pairs, not something convoluted.

// DAS also wants these types to be trivially constructible, but we
// cannot ensure that, as they are zero-initialized by default for
// convenience inside C++. We therefore have to force-tell das that it's
// okay to zero-initialize these types, as all default initializers are
// zero anyways.


static_assert(eastl::is_trivially_copyable_v<ManagedTexView>);
static_assert(eastl::is_trivially_destructible_v<ManagedTexView>);
static_assert(sizeof(ManagedTexView) <= 16);

static_assert(eastl::is_trivially_copyable_v<ManagedBufView>);
static_assert(eastl::is_trivially_destructible_v<ManagedBufView>);
static_assert(sizeof(ManagedBufView) <= 16);


struct ManagedTexViewAnnotation final : das::ManagedValueAnnotation<ManagedTexView>
{
  ManagedTexViewAnnotation(das::ModuleLibrary &ml) : ManagedValueAnnotation(ml, "ManagedTexView", " ::ManagedTexView") {}
  bool hasNonTrivialCtor() const override { return false; }
};
struct ManagedBufViewAnnotation final : das::ManagedValueAnnotation<ManagedBufView>
{
  ManagedBufViewAnnotation(das::ModuleLibrary &ml) : ManagedValueAnnotation(ml, "ManagedBufView", " ::ManagedBufView") {}
  bool hasNonTrivialCtor() const override { return false; }
};
struct ManagedTexAnnotation final : das::ManagedStructureAnnotation<ManagedTex>
{
  ManagedTexAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("ManagedTex", ml, "ManagedTex")
  {
    addPropertyForManagedType<DAS_BIND_MANAGED_PROP(getTexId)>("getTexId");
    addPropertyForManagedType<DAS_BIND_MANAGED_PROP(getTex2D)>("getTex2D");
    addPropertyForManagedType<DAS_BIND_MANAGED_PROP(getArrayTex)>("getArrayTex");
    addPropertyForManagedType<DAS_BIND_MANAGED_PROP(getCubeTex)>("getCubeTex");
    addPropertyForManagedType<DAS_BIND_MANAGED_PROP(getVolTex)>("getVolTex");
  }
};
template <typename T>
struct ManagedResAnnotation : das::ManagedStructureAnnotation<T, false>
{
  ManagedResAnnotation(const char *name, das::ModuleLibrary &ml) : das::ManagedStructureAnnotation<T, false>(name, ml, name) {}
  bool rtti_isHandledTypeAnnotation() const override { return true; }
  bool isRefType() const override { return true; }
  bool isLocal() const override { return false; }
  bool isPod() const override { return false; }
  bool canCopy() const override { return false; }
  bool canMove() const override { return false; }
  bool canClone() const override { return false; }
};
struct SharedTexAnnotation final : ManagedResAnnotation<SharedTex>
{
  SharedTexAnnotation(das::ModuleLibrary &ml) : ManagedResAnnotation("SharedTex", ml)
  {
    addPropertyForManagedType<DAS_BIND_MANAGED_PROP(getTexId)>("getTexId");
    addPropertyForManagedType<DAS_BIND_MANAGED_PROP(getTex2D)>("getTex2D");
    addPropertyForManagedType<DAS_BIND_MANAGED_PROP(getArrayTex)>("getArrayTex");
    addPropertyForManagedType<DAS_BIND_MANAGED_PROP(getCubeTex)>("getCubeTex");
    addPropertyForManagedType<DAS_BIND_MANAGED_PROP(getVolTex)>("getVolTex");
  }
};
struct SharedTexHolderAnnotation final : ManagedResAnnotation<SharedTexHolder>
{
  SharedTexHolderAnnotation(das::ModuleLibrary &ml) : ManagedResAnnotation("SharedTexHolder", ml)
  {
    addPropertyForManagedType<DAS_BIND_MANAGED_PROP(getTexId)>("getTexId");
    addPropertyForManagedType<DAS_BIND_MANAGED_PROP(getTex2D)>("getTex2D");
    addPropertyForManagedType<DAS_BIND_MANAGED_PROP(getArrayTex)>("getArrayTex");
    addPropertyForManagedType<DAS_BIND_MANAGED_PROP(getCubeTex)>("getCubeTex");
    addPropertyForManagedType<DAS_BIND_MANAGED_PROP(getVolTex)>("getVolTex");
  }
};
struct UniqueTexAnnotation final : ManagedResAnnotation<UniqueTex>
{
  UniqueTexAnnotation(das::ModuleLibrary &ml) : ManagedResAnnotation("UniqueTex", ml)
  {
    addPropertyForManagedType<DAS_BIND_MANAGED_PROP(getTexId)>("getTexId");
    addPropertyForManagedType<DAS_BIND_MANAGED_PROP(getTex2D)>("getTex2D");
    addPropertyForManagedType<DAS_BIND_MANAGED_PROP(getArrayTex)>("getArrayTex");
    addPropertyForManagedType<DAS_BIND_MANAGED_PROP(getCubeTex)>("getCubeTex");
    addPropertyForManagedType<DAS_BIND_MANAGED_PROP(getVolTex)>("getVolTex");
  }
};
struct UniqueTexHolderAnnotation final : ManagedResAnnotation<UniqueTexHolder>
{
  UniqueTexHolderAnnotation(das::ModuleLibrary &ml) : ManagedResAnnotation("UniqueTexHolder", ml)
  {
    addPropertyForManagedType<DAS_BIND_MANAGED_PROP(getTexId)>("getTexId");
    addPropertyForManagedType<DAS_BIND_MANAGED_PROP(getTex2D)>("getTex2D");
    addPropertyForManagedType<DAS_BIND_MANAGED_PROP(getArrayTex)>("getArrayTex");
    addPropertyForManagedType<DAS_BIND_MANAGED_PROP(getCubeTex)>("getCubeTex");
    addPropertyForManagedType<DAS_BIND_MANAGED_PROP(getVolTex)>("getVolTex");
  }
};
struct SharedBufAnnotation final : ManagedResAnnotation<SharedBuf>
{
  SharedBufAnnotation(das::ModuleLibrary &ml) : ManagedResAnnotation("SharedBuf", ml)
  {
    addPropertyForManagedType<DAS_BIND_MANAGED_PROP(getBufId)>("getBufId");
  }
};
struct SharedBufHolderAnnotation final : ManagedResAnnotation<SharedBufHolder>
{
  SharedBufHolderAnnotation(das::ModuleLibrary &ml) : ManagedResAnnotation("SharedBufHolder", ml)
  {
    addPropertyForManagedType<DAS_BIND_MANAGED_PROP(getBufId)>("getBufId");
  }
};
struct UniqueBufAnnotation final : ManagedResAnnotation<UniqueBuf>
{
  UniqueBufAnnotation(das::ModuleLibrary &ml) : ManagedResAnnotation("UniqueBuf", ml)
  {
    addPropertyForManagedType<DAS_BIND_MANAGED_PROP(getBufId)>("getBufId");
  }
};
struct UniqueBufHolderAnnotation final : ManagedResAnnotation<UniqueBufHolder>
{
  UniqueBufHolderAnnotation(das::ModuleLibrary &ml) : ManagedResAnnotation("UniqueBufHolder", ml)
  {
    addPropertyForManagedType<DAS_BIND_MANAGED_PROP(getBufId)>("getBufId");
  }
};

namespace bind_dascript
{

class DagorResPtr final : public das::Module
{
public:
  DagorResPtr() : das::Module("DagorResPtr")
  {
    das::ModuleLibrary lib(this);
    addBuiltinDependency(lib, das::Module::require("DagorTexture3D"), true);

    addAnnotation(das::make_smart<ManagedTexAnnotation>(lib));
    addAnnotation(das::make_smart<ManagedTexViewAnnotation>(lib));
    addAnnotation(das::make_smart<ManagedBufViewAnnotation>(lib));

    addAnnotation(das::make_smart<SharedBufAnnotation>(lib));
    addAnnotation(das::make_smart<SharedBufHolderAnnotation>(lib));
    addAnnotation(das::make_smart<UniqueBufAnnotation>(lib));
    addAnnotation(das::make_smart<UniqueBufHolderAnnotation>(lib));
    addAnnotation(das::make_smart<SharedTexAnnotation>(lib));
    addAnnotation(das::make_smart<SharedTexHolderAnnotation>(lib));
    addAnnotation(das::make_smart<UniqueTexAnnotation>(lib));
    addAnnotation(das::make_smart<UniqueTexHolderAnnotation>(lib));

    das::addExtern<DAS_BIND_FUN(bind_dascript::get_tex2d)>(*this, lib, "get_tex2d", das::SideEffects::none,
      " ::bind_dascript::get_tex2d");
    das::addExtern<DAS_BIND_FUN(bind_dascript::get_buf)>(*this, lib, "get_buf", das::SideEffects::none, " ::bind_dascript::get_buf");

    das::addExtern<D3DRESID (*)(ManagedTexView), &bind_dascript::get_res_id>(*this, lib, "get_res_id", das::SideEffects::none,
      " ::bind_dascript::get_res_id");
    das::addExtern<D3DRESID (*)(ManagedBufView), &bind_dascript::get_res_id>(*this, lib, "get_res_id", das::SideEffects::none,
      " ::bind_dascript::get_res_id");

    das::addExtern<DAS_BIND_FUN(bind_dascript::SharedBuf_get_buf)>(*this, lib, "getBuf", das::SideEffects::modifyExternal,
      " ::bind_dascript::SharedBuf_get_buf");
    das::addExtern<DAS_BIND_FUN(bind_dascript::SharedBufHolder_get_buf)>(*this, lib, "getBuf", das::SideEffects::modifyExternal,
      " ::bind_dascript::SharedBufHolder_get_buf");
    das::addExtern<DAS_BIND_FUN(bind_dascript::UniqueBuf_get_buf)>(*this, lib, "getBuf", das::SideEffects::modifyExternal,
      " ::bind_dascript::UniqueBuf_get_buf");
    das::addExtern<DAS_BIND_FUN(bind_dascript::UniqueBufHolder_get_buf)>(*this, lib, "getBuf", das::SideEffects::modifyExternal,
      " ::bind_dascript::UniqueBufHolder_get_buf");

#define BIND_FUNCTION(function, sinonim, side_effect) \
  das::addExtern<DAS_BIND_FUN(function)>(*this, lib, sinonim, side_effect, "bind_dascript::" #function);

    BIND_FUNCTION(get_tex_gameres1, "get_tex_gameres", das::SideEffects::modifyArgumentAndExternal)
    BIND_FUNCTION(get_tex_gameres2, "get_tex_gameres", das::SideEffects::modifyArgumentAndExternal)
    BIND_FUNCTION(add_managed_array_texture1, "add_managed_array_texture", das::SideEffects::modifyArgumentAndExternal)
    BIND_FUNCTION(add_managed_array_texture2, "add_managed_array_texture", das::SideEffects::modifyArgumentAndExternal)


#define BIND_RES_FUNCTION(function, name) BIND_FUNCTION(function, name, das::SideEffects::modifyArgumentAndExternal)

#define MANAGED_TEX(TYPE, tex)                                 \
  BIND_RES_FUNCTION(create_tex##tex, "create_tex")             \
  BIND_RES_FUNCTION(create_cubetex##tex, "create_cubetex")     \
  BIND_RES_FUNCTION(create_voltex##tex, "create_voltex")       \
  BIND_RES_FUNCTION(create_array_tex##tex, "create_array_tex") \
  BIND_RES_FUNCTION(create_cube_array_tex##tex, "create_cube_array_tex")

    MANAGED_TEX_TYPES
#undef MANAGED_TEX

#define MANAGED_BUF(TYPE, suf)                   \
  BIND_RES_FUNCTION(create_vb##suf, "create_vb") \
  BIND_RES_FUNCTION(create_ib##suf, "create_ib") \
  BIND_RES_FUNCTION(create_sbuffer##suf, "create_sbuffer")

    MANAGED_BUF_TYPES
#undef MANAGED_BUF

#undef BIND_RES_FUNCTION
#undef BIND_FUNCTION

    verifyAotReady();
  }
  virtual das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include <dasModules/dasDagorResPtr.h>\n";
    return das::ModuleAotType::cpp;
  }
};

} // namespace bind_dascript
REGISTER_MODULE_IN_NAMESPACE(DagorResPtr, bind_dascript);
