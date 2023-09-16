#include <dasModules/dasModulesCommon.h>
#include <dasModules/dasDataBlock.h>
#include <dasModules/aotDagorMath.h>
#include "dagorDataBlockCast.h"

#include <memory/dag_framemem.h>
#include <EASTL/memory.h>

DAS_BASE_BIND_ENUM_98(::DataBlock::ParamType, DataBlockParamType, TYPE_NONE, TYPE_STRING, TYPE_INT, TYPE_REAL, TYPE_POINT2,
  TYPE_POINT3, TYPE_POINT4, TYPE_IPOINT2, TYPE_IPOINT3, TYPE_BOOL, TYPE_E3DCOLOR, TYPE_MATRIX, TYPE_INT64)

DAS_BASE_BIND_ENUM(dblk::ReadFlag, DataBlockReadFlag, ROBUST, BINARY_ONLY, RESTORE_FLAGS, ALLOW_SS, ROBUST_IN_REL)

static bool dblk_load(DataBlock &b, const char *fn, dblk::ReadFlag flg) { return dblk::load(b, fn, flg); }

struct DataBlockAnnotation : das::ManagedStructureAnnotation<DataBlock, false>
{
  DataBlockAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("DataBlock", ml)
  {
    addProperty<DAS_BIND_MANAGED_PROP(getBlockName)>("getBlockName");
    addProperty<DAS_BIND_MANAGED_PROP(getBlockName)>("blockName", "getBlockName");
    addProperty<DAS_BIND_MANAGED_PROP(getBlockNameId)>("blockNameId", "getBlockNameId");
    addProperty<DAS_BIND_MANAGED_PROP(blockCount)>("blockCount");
    addProperty<DAS_BIND_MANAGED_PROP(paramCount)>("paramCount");
  }

#if DAGOR_DBGLEVEL > 0
  virtual void walk(das::DataWalker &walker, void *obj) override
  {
    DataBlock *blk = (DataBlock *)obj;

    DynamicMemGeneralSaveCB cwr(framemem_ptr(), 0, 2 << 10);
    blk->saveToTextStreamCompact(cwr);
    cwr.writeInt(0);
    unsigned char *data = cwr.data();
    walker.String(*(char **)&data);
  }
#endif
};

struct SharedDataBlockAnnotation : das::ManagedStructureAnnotation<eastl::shared_ptr<DataBlock>, false>
{
  SharedDataBlockAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("SharedDataBlock", ml)
  {
    cppName = " ::eastl::shared_ptr<DataBlock>";
    addProperty<DAS_BIND_MANAGED_PROP(get)>("blk", "get");
  }
};


namespace bind_dascript
{
static char dagorDataBlock_das[] =
#include "dagorDataBlock.das.inl"
  ;

void datablock_debug_print_datablock(const char *name, const DataBlock &blk) { ::debug_print_datablock(name, &blk); }

const char *datablock_to_string(const DataBlock &blk, das::Context *context)
{
  DynamicMemGeneralSaveCB cwr(framemem_ptr(), 0, 2 << 10);
  blk.saveToTextStream(cwr);
  return context->stringHeap->allocateString((const char *)cwr.data(), cwr.size());
}

const DataBlock &datablock_get_empty() { return DataBlock::emptyBlock; }

void datablock_set_from(DataBlock &blk, const DataBlock &from, const char *src_fname) { blk.setFrom(&from, src_fname); }


class DagorDataBlock final : public das::Module
{
public:
  DagorDataBlock() : das::Module("DagorDataBlock")
  {
    das::ModuleLibrary lib(this);
    addBuiltinDependency(lib, require("math"));
    addBuiltinDependency(lib, require("DagorMath"));

    addEnumeration(das::make_smart<EnumerationDataBlockParamType>());
    addEnumeration(das::make_smart<EnumerationDataBlockReadFlag>());

    addAnnotation(das::make_smart<DataBlockAnnotation>(lib));
    addAnnotation(das::make_smart<SharedDataBlockAnnotation>(lib));
    das::addUsing<DataBlock>(*this, lib, "DataBlock");

#define BIND_BLK(FUNC_NAME, SIDE_EFFECT) \
  das::addExtern<DAS_BIND_FUN(FUNC_NAME)>(*this, lib, #FUNC_NAME, SIDE_EFFECT, "bind_dascript::" #FUNC_NAME)

// macro for bind class member function
#define CLASS_MEMBER(FUNC_NAME, SYNONIM, SIDE_EFFECT)                                                                           \
  {                                                                                                                             \
    using memberMethod = DAS_CALL_MEMBER(DataBlock::FUNC_NAME);                                                                 \
    das::addExtern<DAS_CALL_METHOD(memberMethod)>(*this, lib, SYNONIM, SIDE_EFFECT, DAS_CALL_MEMBER_CPP(DataBlock::FUNC_NAME)); \
  }

// macro for bind class member function with special signature to manage with void f(int) and void f(float)
#define BLK_MEMBER(FUNC_NAME, SYNONIM, SIDE_EFFECT, SIGNATURE)                      \
  {                                                                                 \
    using memberMethod = das::das_call_member<SIGNATURE, &DataBlock::FUNC_NAME>;    \
    das::addExtern<DAS_CALL_METHOD(memberMethod)>(*this, lib, SYNONIM, SIDE_EFFECT, \
      "das_call_member<" #SIGNATURE ", &DataBlock::" #FUNC_NAME ">::invoke");       \
  }

// macro for bind class member function with const/not const versions to manage with T& getT() and const T& getT() const
#define BLK_MEMBER_CONST(FUNC_NAME, SYNONIM, SIDE_EFFECT, SIGNATURE)                                                      \
  {                                                                                                                       \
    using memberMethod = das::das_call_member<SIGNATURE, &DataBlock::FUNC_NAME>;                                          \
    das::addExtern<DAS_CALL_METHOD(memberMethod), das::SimNode_ExtFuncCall, das::explicitConstArgFn>(*this, lib, SYNONIM, \
      SIDE_EFFECT, "das_call_member<" #SIGNATURE ", &DataBlock::" #FUNC_NAME ">::invoke");                                \
  }

    struct removeRefFn : das::defaultTempFn
    {
      removeRefFn(int idx_) : idx{idx_} {}
      ___noinline bool operator()(das::Function *fn)
      {
        defaultTempFn::operator()(fn);
        fn->arguments[idx]->type->ref = false;
        return true;
      }
      const int idx = 2;
    };

// macro for bind class member function and remove ref from 1 argument.
//  void f(const Point3 &) -> void f(Point3). It's need to pass rvalue in this functions
#define BLK_MEMBER_REMOVE_REF(FUNC_NAME, SYNONIM, SIDE_EFFECT, SIGNATURE, REF_IDX)                                         \
  {                                                                                                                        \
    using memberMethod = das::das_call_member<SIGNATURE, &DataBlock::FUNC_NAME>;                                           \
    das::addExtern<DAS_CALL_METHOD(memberMethod), das::SimNode_ExtFuncCall, removeRefFn>(*this, lib, SYNONIM, SIDE_EFFECT, \
      "das_call_member<" #SIGNATURE ", &DataBlock::" #FUNC_NAME ">::invoke", removeRefFn(REF_IDX));                        \
  }

    BLK_MEMBER(clearData, "datablock_clear_data", das::SideEffects::modifyArgument, void(DataBlock::*)())
    BLK_MEMBER(reset, "datablock_reset", das::SideEffects::modifyArgument, void(DataBlock::*)())
    BLK_MEMBER(load, "datablock_load", das::SideEffects::modifyArgument, bool(DataBlock::*)(const char *))

    das::addExtern<DAS_BIND_FUN(dblk_load)>(*this, lib, "datablock_load", das::SideEffects::modifyArgumentAndExternal, "dblk::load");

    BLK_MEMBER_CONST(getBlock, "datablock_get_block", das::SideEffects::none, const DataBlock *(DataBlock::*)(uint32_t) const)
    BLK_MEMBER_CONST(getBlock, "datablock_get_block", das::SideEffects::none, DataBlock * (DataBlock::*)(uint32_t))
    BLK_MEMBER_CONST(getBlockByName, "datablock_get_block_by_name", das::SideEffects::none, DataBlock * (DataBlock::*)(const char *))
    BLK_MEMBER_CONST(getBlockByName, "datablock_get_block_by_name", das::SideEffects::none,
      const DataBlock *(DataBlock::*)(const char *) const)
    BLK_MEMBER_CONST(getBlockByNameId, "datablock_get_block_by_name_id", das::SideEffects::none, DataBlock * (DataBlock::*)(int))
    BLK_MEMBER_CONST(getBlockByNameId, "datablock_get_block_by_name_id", das::SideEffects::none,
      const DataBlock *(DataBlock::*)(int) const)
    BLK_MEMBER_CONST(getBlockByNameEx, "datablock_get_block_by_name_ex", das::SideEffects::none,
      const DataBlock *(DataBlock::*)(const char *) const);

    BLK_MEMBER(getBlockName, "datablock_getBlockName", das::SideEffects::none, const char *(DataBlock::*)() const)
    BLK_MEMBER(getNameId, "datablock_getNameId", das::SideEffects::none, int(DataBlock::*)(const char *) const)
    BLK_MEMBER(getBlockNameId, "datablock_getBlockNameId", das::SideEffects::none, int(DataBlock::*)() const)
    BLK_MEMBER(getParamNameId, "datablock_getParamNameId", das::SideEffects::none, int(DataBlock::*)(uint32_t) const)

    BLK_MEMBER(findParam, "datablock_find_param", das::SideEffects::none, int(DataBlock::*)(int) const)
    BLK_MEMBER(findParam, "datablock_find_param", das::SideEffects::none, int(DataBlock::*)(const char *, int) const)
    BLK_MEMBER(paramExists, "datablock_param_exists", das::SideEffects::none, bool(DataBlock::*)(int, int) const)
    BLK_MEMBER(paramExists, "datablock_param_exists", das::SideEffects::none, bool(DataBlock::*)(const char *, int) const)
    BLK_MEMBER(blockExists, "datablock_block_exists", das::SideEffects::none, bool(DataBlock::*)(const char *) const)
    BLK_MEMBER(addBlock, "datablock_add_block", das::SideEffects::modifyArgument, DataBlock * (DataBlock::*)(const char *))
    BLK_MEMBER(addNewBlock, "datablock_add_new_block", das::SideEffects::modifyArgument, DataBlock * (DataBlock::*)(const char *))
    BLK_MEMBER(getParamName, "datablock_get_param_name", das::SideEffects::none, const char *(DataBlock::*)(uint32_t) const)


    // registration of get/set/add DataBlock functions

#define BLK_GET_SET_ADD(FUNCTION, TYPE)                                                                                       \
  BLK_MEMBER(get##FUNCTION, "datablock_get" #FUNCTION, das::SideEffects::none, TYPE (DataBlock::*)(const char *, TYPE) const) \
  BLK_MEMBER(get##FUNCTION, "datablock_get" #FUNCTION, das::SideEffects::none, TYPE (DataBlock::*)(int) const)                \
  BLK_MEMBER(set##FUNCTION, "set", das::SideEffects::modifyArgument, int (DataBlock::*)(const char *, TYPE))                  \
  BLK_MEMBER(set##FUNCTION, "set", das::SideEffects::modifyArgument, bool (DataBlock::*)(int, TYPE))                          \
  BLK_MEMBER(add##FUNCTION, "add", das::SideEffects::modifyArgument, int (DataBlock::*)(const char *, TYPE))

    // it require remove reference hack to pass rvalue float2-3-4 as default value

#define BLK_GET_SET_ADD_REF(FUNCTION, TYPE)                                                                                        \
  BLK_MEMBER_REMOVE_REF(get##FUNCTION, "datablock_get" #FUNCTION, das::SideEffects::none,                                          \
    TYPE (DataBlock::*)(const char *, const TYPE &) const, 2)                                                                      \
  BLK_MEMBER(get##FUNCTION, "datablock_get" #FUNCTION, das::SideEffects::none, TYPE (DataBlock::*)(int) const)                     \
  BLK_MEMBER_REMOVE_REF(set##FUNCTION, "set", das::SideEffects::modifyArgument, int (DataBlock::*)(const char *, const TYPE &), 2) \
  BLK_MEMBER_REMOVE_REF(set##FUNCTION, "set", das::SideEffects::modifyArgument, bool (DataBlock::*)(int, const TYPE &), 2)         \
  BLK_MEMBER_REMOVE_REF(add##FUNCTION, "add", das::SideEffects::modifyArgument, int (DataBlock::*)(const char *, const TYPE &), 2)


    BLK_GET_SET_ADD(Str, const char *)
    BLK_GET_SET_ADD(Bool, bool)
    BLK_GET_SET_ADD(Int, int)
    BLK_GET_SET_ADD(Int64, int64_t)
    BLK_GET_SET_ADD(Real, float)
    BLK_GET_SET_ADD_REF(Point2, Point2)
    BLK_GET_SET_ADD_REF(Point3, Point3)
    BLK_GET_SET_ADD_REF(Point4, Point4)
    BLK_GET_SET_ADD_REF(IPoint2, IPoint2)
    BLK_GET_SET_ADD_REF(IPoint3, IPoint3)
    BLK_GET_SET_ADD(E3dcolor, E3DCOLOR)

    // TMatrix require special bindings with SimNode_ExtFuncCallAndCopyOrMove

    using getTmMethod = das::das_call_member<TMatrix (DataBlock::*)(const char *, const TMatrix &) const, &DataBlock::getTm>;
    das::addExtern<DAS_CALL_METHOD(getTmMethod), das::SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "datablock_getTm",
      das::SideEffects::none, "das_call_member<TMatrix(DataBlock::*)(const char*, const TMatrix&) const, &DataBlock::getTm>::invoke");

    using method_getTmIdx = das::das_call_member<TMatrix (DataBlock::*)(int) const, &DataBlock::getTm>;
    das::addExtern<DAS_CALL_METHOD(method_getTmIdx), das::SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "datablock_getTm",
      das::SideEffects::none, "das::das_call_member<TMatrix(DataBlock::*)(int) const, &DataBlock::getTm>::invoke");

    BLK_MEMBER(addTm, "add", das::SideEffects::modifyArgument, int(DataBlock::*)(const char *, const TMatrix &))
    BLK_MEMBER(setTm, "set", das::SideEffects::modifyArgument, int(DataBlock::*)(const char *, const TMatrix &))


    BLK_MEMBER(removeParam, "datablock_remove_param", das::SideEffects::modifyArgument, bool(DataBlock::*)(const char *))
    BLK_MEMBER(removeBlock, "datablock_remove_block", das::SideEffects::modifyArgument, bool(DataBlock::*)(const char *))
    BLK_MEMBER(setParamsFrom, "datablock_set_params_from", das::SideEffects::modifyArgument, void(DataBlock::*)(const DataBlock *))
    BLK_MEMBER(changeBlockName, "datablock_change_block_name", das::SideEffects::modifyArgument, void(DataBlock::*)(const char *))
    BLK_MEMBER(loadText, "datablock_load_text", das::SideEffects::modifyArgument, bool(DataBlock::*)(const char *, int, const char *))

    das::addExtern<DAS_BIND_FUN(dblk::save_to_binary_file)>(*this, lib, "datablock_save_to_binary_file",
      das::SideEffects::modifyExternal, "dblk::save_to_binary_file");

    das::addExtern<DAS_BIND_FUN(datablock_debug_print_datablock)>(*this, lib, "debug_print_datablock",
      das::SideEffects::modifyExternal, "bind_dascript::datablock_debug_print_datablock");

    das::addExtern<DAS_BIND_FUN(datablock_to_string)>(*this, lib, "string", das::SideEffects::none,
      "bind_dascript::datablock_to_string");

    das::addExternTempRef<DAS_BIND_FUN(datablock_get_empty)>(*this, lib, "datablock_get_empty", das::SideEffects::accessExternal,
      "bind_dascript::datablock_get_empty");

    das::addExtern<DAS_BIND_FUN(datablock_set_from)>(*this, lib, "datablock_set_from", das::SideEffects::modifyArgument,
      "bind_dascript::datablock_set_from");

    CLASS_MEMBER(resolveFilename, "datablock_resolveFilename", das::SideEffects::none)
    CLASS_MEMBER(getParamName, "datablock_getParamName", das::SideEffects::none)
    CLASS_MEMBER(getParamType, "datablock_getParamType", das::SideEffects::none)
    CLASS_MEMBER(saveToTextFile, "datablock_save_to_text_file", das::SideEffects::modifyExternal)


#undef BIND_BLK
#undef BLK_MEMBER
#undef BLK_MEMBER_CONST
#undef CLASS_MEMBER
#undef BLK_GET_SET_ADD
#undef BLK_GET_SET_ADD_REF

    das::addExtern<DAS_BIND_FUN(bind_dascript::read_interpolate_tab_as_params)>(*this, lib, "read_interpolate_tab_as_params",
      das::SideEffects::modifyArgument, "bind_dascript::read_interpolate_tab_as_params");
    das::addExtern<DAS_BIND_FUN(bind_dascript::read_interpolate_tab_float_p2)>(*this, lib, "read_interpolate_tab_float_p2",
      das::SideEffects::modifyArgument, "bind_dascript::read_interpolate_tab_float_p2");

    compileBuiltinModule("dagorDataBlock.das", (unsigned char *)dagorDataBlock_das, sizeof(dagorDataBlock_das));

    verifyAotReady();
  }

  virtual das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include <dasModules/dasDataBlock.h>\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace bind_dascript

REGISTER_MODULE_IN_NAMESPACE(DagorDataBlock, bind_dascript)
