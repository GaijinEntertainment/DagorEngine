// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/io/blk.h>
#include <generic/dag_tab.h>
#include <daECS/core/componentTypes.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/component.h>
#include <daECS/core/template.h>
#include <daECS/core/updateStage.h>
#include <daECS/core/sharedComponent.h>
#include <daECS/core/utility/enumDescription.h>
#include <regExp/regExp.h>
#include <EASTL/hash_map.h>
#include <EASTL/fixed_string.h>
#include <EASTL/string_view.h>
#include <EASTL/algorithm.h>
#include <EASTL/functional.h>
#include <ioSys/dag_dataBlock.h>
#include <math/dag_e3dColor.h>
#include <memory/dag_framemem.h>
#include <util/dag_string.h>
#include "../../core/tokenize_const_string.h" // FIXME
#include <osApiWrappers/dag_direct.h>
#include <ioSys/dag_findFiles.h>
#include <ioSys/dag_dataBlockUtils.h>
#if DAGOR_DBGLEVEL > 0
#include <daECS/core/entitySystem.h>
#endif

namespace ecs
{

#define VALUE_NID(b) (b.getNameId("value"))
#define GROUP_NID(b) (b.getNameId("_group"))
#define MAX_NAME_LEN 128
static inline bool is_reserved_name(const char *name) { return *name == '_'; }
#define COMP_DELIM "__"

static const char TAGS_NAME[] = "_tags";
static const char ES_TAGS_NAME[] = "_tags"; // can be different

ecs::ChildComponent load_comp_from_blk(const DataBlock &blk, int i)
{
  switch (blk.getParamType(i))
  {
    case DataBlock::TYPE_STRING: return ecs::string(blk.getStr(i));
#define IMPL_CASE(type, meth) \
  case DataBlock::TYPE_##type: return ecs::ChildComponent(blk.get##meth(i))
      IMPL_CASE(INT, Int);
      IMPL_CASE(REAL, Real);
      IMPL_CASE(POINT2, Point2);
      IMPL_CASE(POINT3, Point3);
      IMPL_CASE(POINT4, Point4);
      IMPL_CASE(IPOINT2, IPoint2);
      IMPL_CASE(IPOINT3, IPoint3);
      IMPL_CASE(BOOL, Bool);
      IMPL_CASE(MATRIX, Tm);
      IMPL_CASE(E3DCOLOR, E3dcolor);
      IMPL_CASE(INT64, Int64);
#undef IMPL_CASE
  }
  G_ASSERT(0); // Can't happen - ecs type system is strong superset of DataBlock's
  return ecs::ChildComponent();
}

typedef eastl::pair<eastl::string_view, eastl::string_view> SvPair;
typedef eastl::function<void(HashedConstString)> on_empty_comp_type_cb_t;
struct BlkLoadContext
{
  ComponentsList *clist = NULL; // not null
  const char *templName = NULL;
  const char *blkFilePath = NULL;
  const char *blockName = NULL;
  TemplateComponentsDB *db = NULL;
  TemplateDBInfo *info = NULL;
  ecs::EntityManager *mgr = NULL;
  int valueNid = -1, groupNid = -1;
  int replicateNid = -1, trackNid = -1, ignoreNid = -1, hideNid = -1, infoNid = -1;
  struct
  {
    ecs::Template::component_set *replicated = NULL, *tracked = NULL, *ignored = NULL, *hidden = NULL, *tagsSkipped = NULL;
  } sets;

  void load(const DataBlock &blk, const char *tags, on_empty_comp_type_cb_t on_empty_comp_type_cb = on_empty_comp_type_cb_t{});
  static const char *parseNameType(const char *name, char *sname, eastl::string_view &compName, eastl::string_view &compType);
};

static void load_object(BlkLoadContext &ctx, const char *name, const DataBlock &);
static void load_array(BlkLoadContext &ctx, const char *name, const DataBlock &);
static void load_shared_object(BlkLoadContext &ctx, const char *name, const DataBlock &);
static void load_shared_array(BlkLoadContext &ctx, const char *name, const DataBlock &);

template <typename T>
static void load_list(BlkLoadContext &ctx, const char *name, const DataBlock &);

template <typename T>
static void load_shared_list(BlkLoadContext &ctx, const char *name, const DataBlock &);

static inline ecs::string load_blk_str_impl(BlkLoadContext &ctx, const DataBlock &blk) { return blk.getStrByNameId(ctx.valueNid, ""); }

static inline void load_blk_str(BlkLoadContext &ctx, const char *name, const DataBlock &blk)
{
  ecs::string str = load_blk_str_impl(ctx, blk);
  ctx.clist->emplace_back(name, eastl::move(str));
}
static inline void load_blk_shared_str(BlkLoadContext &ctx, const char *name, const DataBlock &blk)
{
  ecs::string str = load_blk_str_impl(ctx, blk);
  ctx.clist->emplace_back(name, ecs::SharedComponent<ecs::string>(eastl::move(str)));
}
template <typename T, T (DataBlock::*getter)(int) const, typename U = T>
static inline void load_blk_type(BlkLoadContext &ctx, const char *name, const DataBlock &blk)
{
  ctx.clist->emplace_back(name, U(eastl::invoke(getter, blk, blk.findParam(ctx.valueNid))));
}

typedef void (*block_load_t)(BlkLoadContext &ctx, const char *name, const DataBlock &);
struct BlockLoaderDesc
{
  const char *typeName = NULL;
  size_t typeNameLen = 0;
  block_load_t load = NULL;
  component_type_t compType = 0;
};

#define LIST_TYPE(name, tn)                                                               \
  {TYPENAMEANDLEN("list<" #name ">"), &load_list<tn>, ComponentTypeInfo<List<tn>>::type}, \
    {TYPENAMEANDLEN("shared:list<" #name ">"), &load_shared_list<tn>, ComponentTypeInfo<SharedComponent<List<tn>>>::type},
#define LIST_TYPES              \
  LIST_TYPE(eid, ecs::EntityId) \
  LIST_TYPE(i, int)             \
  LIST_TYPE(i8, int8_t)         \
  LIST_TYPE(i16, int16_t)       \
  LIST_TYPE(i32, int)           \
  LIST_TYPE(i64, int64_t)       \
  LIST_TYPE(u8, uint8_t)        \
  LIST_TYPE(u16, uint16_t)      \
  LIST_TYPE(u32, uint32_t)      \
  LIST_TYPE(u64, uint64_t)      \
  LIST_TYPE(t, ecs::string)     \
  LIST_TYPE(r, float)           \
  LIST_TYPE(p2, Point2)         \
  LIST_TYPE(p3, Point3)         \
  LIST_TYPE(p4, Point4)         \
  LIST_TYPE(ip2, IPoint2)       \
  LIST_TYPE(ip3, IPoint3)       \
  LIST_TYPE(ip4, IPoint4)       \
  LIST_TYPE(b, bool)            \
  LIST_TYPE(m, TMatrix)         \
  LIST_TYPE(c, E3DCOLOR)

#define TYPENAMEANDLEN(x) x, sizeof(x) - 1
#define SHARED_PREFIX     "shared:"
#define SHARED_PREFIX_LEN uint32_t(7) // uint32_t(strlen(SHARED_PREFIX))

static const BlockLoaderDesc block_loaders[] = {LIST_TYPES{TYPENAMEANDLEN("object"), &load_object},
  {TYPENAMEANDLEN("array"), &load_array}, {TYPENAMEANDLEN("shared:object"), &load_shared_object},
  {TYPENAMEANDLEN("shared:array"), &load_shared_array},
  {TYPENAMEANDLEN("tag"),
    [](BlkLoadContext &ctx, const char *name, const DataBlock &) { ctx.clist->emplace_back(name, ChildComponent(ecs::Tag())); },
    ComponentTypeInfo<Tag>::type},
  {TYPENAMEANDLEN("eid"),
    [](BlkLoadContext &ctx, const char *name, const DataBlock &blk) {
      ecs::EntityId eid;
      const int valueParamId = blk.findParam(ctx.valueNid);
      if (valueParamId >= 0)
        eid = ecs::EntityId((ecs::entity_id_t)blk.getInt(valueParamId));
      ctx.clist->emplace_back(name, ChildComponent(eid));
    },
    ComponentTypeInfo<EntityId>::type},
  {TYPENAMEANDLEN("t"), &load_blk_str, ComponentTypeInfo<ecs::string>::type},
  {TYPENAMEANDLEN("shared:t"), &load_blk_shared_str, ComponentTypeInfo<SharedComponent<ecs::string>>::type},
  {TYPENAMEANDLEN("i"), &load_blk_type<int, &DataBlock::getInt>, ComponentTypeInfo<int>::type},
  {TYPENAMEANDLEN("r"), &load_blk_type<float, &DataBlock::getReal>, ComponentTypeInfo<float>::type},
  {TYPENAMEANDLEN("p2"), &load_blk_type<Point2, &DataBlock::getPoint2>, ComponentTypeInfo<Point2>::type},
  {TYPENAMEANDLEN("p3"), &load_blk_type<Point3, &DataBlock::getPoint3>, ComponentTypeInfo<Point3>::type},
  {TYPENAMEANDLEN("dp3"), &load_blk_type<Point3, &DataBlock::getPoint3, DPoint3>, ComponentTypeInfo<DPoint3>::type},
  {TYPENAMEANDLEN("p4"), &load_blk_type<Point4, &DataBlock::getPoint4>, ComponentTypeInfo<Point4>::type},
  {TYPENAMEANDLEN("ip2"), &load_blk_type<IPoint2, &DataBlock::getIPoint2>, ComponentTypeInfo<IPoint2>::type},
  {TYPENAMEANDLEN("ip3"), &load_blk_type<IPoint3, &DataBlock::getIPoint3>, ComponentTypeInfo<IPoint3>::type},
  {TYPENAMEANDLEN("b"), &load_blk_type<bool, &DataBlock::getBool>, ComponentTypeInfo<bool>::type},
  {TYPENAMEANDLEN("m"), &load_blk_type<TMatrix, &DataBlock::getTm>, ComponentTypeInfo<TMatrix>::type},
  {TYPENAMEANDLEN("c"), &load_blk_type<E3DCOLOR, &DataBlock::getE3dcolor>, ComponentTypeInfo<E3DCOLOR>::type},
  {TYPENAMEANDLEN("i8"), &load_blk_type<int, &DataBlock::getInt, int8_t>, ComponentTypeInfo<int8_t>::type},
  {TYPENAMEANDLEN("i16"), &load_blk_type<int, &DataBlock::getInt, int16_t>, ComponentTypeInfo<int16_t>::type},
  {TYPENAMEANDLEN("i32"), &load_blk_type<int, &DataBlock::getInt>, ComponentTypeInfo<int>::type},
  {TYPENAMEANDLEN("i64"), &load_blk_type<int64_t, &DataBlock::getInt64>, ComponentTypeInfo<int64_t>::type},
  {TYPENAMEANDLEN("u8"), &load_blk_type<int, &DataBlock::getInt, uint8_t>, ComponentTypeInfo<uint8_t>::type},
  {TYPENAMEANDLEN("u16"), &load_blk_type<int, &DataBlock::getInt, uint16_t>, ComponentTypeInfo<uint16_t>::type},
  {TYPENAMEANDLEN("u32"), &load_blk_type<int, &DataBlock::getInt, uint32_t>, ComponentTypeInfo<uint32_t>::type},
  {TYPENAMEANDLEN("u64"), &load_blk_type<int64_t, &DataBlock::getInt64, uint64_t>, ComponentTypeInfo<uint64_t>::type},
  {TYPENAMEANDLEN("BBox3"),
    [](BlkLoadContext &ctx, const char *name, const DataBlock &) { ctx.clist->emplace_back(name, ChildComponent(BBox3())); },
    ComponentTypeInfo<BBox3>::type}};
#undef TYPEANDLEN
#undef LIST_TYPES
#undef LIST_TYPE

static inline const BlockLoaderDesc *find_type_block_loader(eastl::string_view tp_name)
{
  auto pred = [tp_name](const BlockLoaderDesc &d) { return eastl::string_view{d.typeName, d.typeNameLen} == tp_name; };
  auto it = eastl::find_if(eastl::begin(block_loaders), eastl::end(block_loaders), pred);
  return it == eastl::end(block_loaders) ? nullptr : it;
}


const char *BlkLoadContext::parseNameType(const char *name, char *sname, eastl::string_view &compName, eastl::string_view &compType)
{
  G_FAST_ASSERT(name);
  int nmlen = (int)strlen(name);
  if (DAGOR_UNLIKELY(nmlen >= MAX_NAME_LEN))
    return "Too long name";
  const char *typeDelim = strchr(name, ':');
  if (!typeDelim)
  {
    compName = name;
    return !compName.empty() ? nullptr : "Empty name";
  }
  memcpy(sname, name, nmlen + 1);
  ptrdiff_t offs = typeDelim - name;
  sname[offs++] = '\0';
  while (sname[offs] == ' ') // skip spaces after ':'
    ++offs;
  compName = eastl::string_view{sname, (size_t)(typeDelim - name)};
  compType = eastl::string_view{&sname[offs]};
  return nullptr;
}

enum class RegisterRet
{
  OK,
  INVALID_TYPE,
  ERROR
};

static RegisterRet register_component(ecs::EntityManager &mgr, const char *compName, eastl::string_view compType, const DataBlock &blk)
{
  eastl::string sharedStr;
  if (compType.length() > SHARED_PREFIX_LEN &&
      strncmp(compType.data(), SHARED_PREFIX, min<uint32_t>(compType.length(), SHARED_PREFIX_LEN)) == 0)
  {
    sharedStr.sprintf("ecs::SharedComponent<%.*s>", (uint32_t)compType.length() - SHARED_PREFIX_LEN,
      compType.data() + SHARED_PREFIX_LEN);
    compType = sharedStr;
  }
  type_index_t cmpType = mgr.getComponentTypes().findType(ECS_HASHLEN_SLOW_LEN(compType.data(), (uint32_t)compType.length()).hash);
  if (cmpType == INVALID_COMPONENT_TYPE_INDEX)
    return RegisterRet::INVALID_TYPE;
  SmallTab<component_t, framemem_allocator> deps;
  int paramId = -1;
  int depNid = blk.getNameId("_deps");
  while ((paramId = blk.findParam(depNid, paramId)) >= 0)
  {
    if (const char *n = blk.getStr(paramId))
      deps.push_back(ECS_HASH_SLOW(n).hash);
  }
  const component_flags_t flag = blk.getBool("_hidden", false) ? DataComponent::DONT_REPLICATE : 0;

  dag::Span<component_t> depsSlice(deps.begin(), deps.size());
  const bool ok = mgr.createComponent(ECS_HASH_SLOW(compName), cmpType, depsSlice, NULL, flag) != INVALID_COMPONENT_INDEX;
  return ok ? RegisterRet::OK : RegisterRet::ERROR;
}

void BlkLoadContext::load(const DataBlock &blk, const char *tags, on_empty_comp_type_cb_t on_empty_comp_type_cb)
{
  char tmp[MAX_NAME_LEN];
  const char *mainFtags = info ? blk.getStr(TAGS_NAME, tags) : nullptr;
  const bool doMainLoad = !mainFtags || !info || ecs::filter_needed(mainFtags, info->filterTags); //-V595
  enum
  {
    TRCK = 1 << 0,
    REPL = 1 << 1,
    IGNR = 1 << 2,
    HIDE = 1 << 3
  };
  auto readSetFlags = [&](const DataBlock &b) {
    return (sets.tracked && b.getBoolByNameId(trackNid, false) ? TRCK : 0) |
           (sets.replicated && b.getBoolByNameId(replicateNid, false) ? REPL : 0) |
           (sets.ignored && b.getBoolByNameId(ignoreNid, false) ? IGNR : 0) |
           (sets.hidden && b.getBoolByNameId(hideNid, false) ? HIDE : 0);
  };
  const uint32_t setFlags = readSetFlags(blk);
  auto addSets = [&](const ecs::component_t hash, uint32_t fl) {
    if (fl & TRCK)
      sets.tracked->insert(hash);
    if (fl & REPL)
      sets.replicated->insert(hash);
    if (fl & IGNR)
      sets.ignored->insert(hash);
    if (fl & HIDE)
      sets.hidden->insert(hash);
  };
  for (int i = 0, e = blk.paramCount(); i < e; ++i)
  {
    const char *pName = blk.getParamName(i);
    if (is_reserved_name(pName))
    {
      if (blk.getParamNameId(i) == infoNid)
      {
        const bool isStr = blk.getParamType(i) == DataBlock::TYPE_STRING;
        const char *_infoStr = isStr ? blk.getStr(i) : nullptr;
        if (!_infoStr)
          logerr("String param expected in '%s' template metalink at %s", templName, blkFilePath);
        else if (info && !info->addMetaLink(TemplateDBInfo::MetaInfoType::TEMPLATE, templName, _infoStr))
          logerr("Bad '%s' template metalink '%s' at %s", templName, _infoStr, blkFilePath);
      }
      continue;
    }
    ecs::ChildComponent comp = load_comp_from_blk(blk, i);
    G_ASSERT_CONTINUE(!comp.isNull());
    HashedConstString hName = ECS_HASH_SLOW(pName);
    if (info)
      info->updateComponentTags(hName, mainFtags, templName, blkFilePath);
    if (doMainLoad && (!db || db->addComponent(hName.hash, comp.getUserType(), pName)))
    {
      addSets(hName.hash, setFlags);
      clist->emplace_back(pName, eastl::move(comp)); // todo: we should not calculate hash again
    }
    else if (doMainLoad && sets.tagsSkipped)
      sets.tagsSkipped->insert(hName.hash);
  }

  for (int j = 0, e = blk.blockCount(); j < e; ++j)
  {
    const DataBlock *subBlock = blk.getBlock(j);
    blockName = subBlock->getBlockName();
    if (subBlock->getBlockNameId() == groupNid)
    {
      load(*subBlock, mainFtags, on_empty_comp_type_cb);
      continue;
    }
    if (is_reserved_name(blockName))
    {
      if (subBlock->getBlockNameId() == infoNid)
        if (info && !info->addMetaData(TemplateDBInfo::MetaInfoType::TEMPLATE, templName, *subBlock))
          logerr("Dup '%s' template metadata at %s", templName, blkFilePath);
      continue;
    }
    eastl::string_view compName, compType;
    if (const char *errStr = parseNameType(blockName, tmp, compName, compType))
    {
      logerr("%s while loading %s:%s:%s", errStr, blkFilePath, templName, blockName);
      continue;
    }
    const char *pName = compName.data();
    const HashedConstString hName = ECS_HASH_SLOW(pName);
    const char *ftags = info ? subBlock->getStr(TAGS_NAME, mainFtags) : nullptr;
    const bool doLoad = !ftags || ecs::filter_needed(ftags, info->filterTags); //-V595, -V1004
    component_type_t userType = compType.empty() ? 0 : ECS_HASH_SLOW(compType.data()).hash;
    bool addedComponent = false;
    auto addDbComponent = [&](component_type_t type) {
      G_ASSERTF(!addedComponent, "component should be added once!");
      userType = type;
      addedComponent = (doLoad && (!db || db->addComponent(hName.hash, userType, hName.str))); //-V595
      return addedComponent;
    };
    if (compType.empty())
    {
      if (on_empty_comp_type_cb)
      {
        if (addDbComponent(userType))
          on_empty_comp_type_cb(hName);
      }
      else
        logwarn("Empty component type while parsing block name '%s' in template '%s:%s', ignored.", blockName, blkFilePath, templName);
    }
    else if (auto typeLoader = find_type_block_loader(compType))
    {
      if (addDbComponent(typeLoader->compType))
        typeLoader->load(*this, pName, *subBlock);
    }
    else if (auto enumDescr = find_enum_description(compType))
    {
      if (addDbComponent(enumDescr->compType))
      {
        clist->emplace_back(pName, enumDescr->parse(pName, compType.data(), subBlock->getStrByNameId(valueNid, "")));
      }
    }
    else
    {
      if (doLoad)
      {
        RegisterRet ret = register_component(*mgr, pName, compType, *subBlock);
        if (ret == RegisterRet::OK)
        {
          if (addDbComponent(userType))
            clist->emplace_back(pName, ChildComponent());
        }
        else if (ret == RegisterRet::INVALID_TYPE)
          logerr("Type '%.*s' in block name '%s' at '%s:%s' is unknown and can't be registered", (int)compType.length(),
            compType.data(), blockName, blkFilePath, templName);
        else
          logerr("Can't create component of type '%.*s' in block name '%s' of template '%s:%s'", (int)compType.length(),
            compType.data(), blockName, blkFilePath, templName);
      }
    }
    if (info)
    {
      info->updateComponentTags(hName, ftags, templName, blkFilePath);

      const int _infoParamIdx = subBlock->findParam(infoNid);
      if (_infoParamIdx >= 0)
      {
        const bool isStr = subBlock->getParamType(_infoParamIdx) == DataBlock::TYPE_STRING;
        const char *_infoStr = isStr ? subBlock->getStr(_infoParamIdx) : nullptr;
        if (!_infoStr)
          logerr("String param expected in '%s' component metalink at %s:%s", pName, blkFilePath, templName);
        else if (!info->addMetaLink(TemplateDBInfo::MetaInfoType::COMPONENT, pName, _infoStr))
          logerr("Bad '%s' component metalink '%s' at %s:%s", pName, _infoStr, blkFilePath, templName);
        else
          info->addMetaComp(templName, pName);
      }
      else if (const DataBlock *_infoBlk = subBlock->getBlockByName(infoNid))
      {
        if (!info->addMetaData(TemplateDBInfo::MetaInfoType::COMPONENT, pName, *_infoBlk))
          logerr("Dup '%s' component metadata at %s:%s", pName, blkFilePath, templName);
        else
          info->addMetaComp(templName, pName);
      }
    }
    if (addedComponent) //-V547
      addSets(hName.hash, setFlags | readSetFlags(*subBlock));
    else if (doLoad && sets.tagsSkipped)
      sets.tagsSkipped->insert(hName.hash);
  }
}

template <typename Cb>
static inline void load_component_list_impl(BlkLoadContext &ctx, const DataBlock &blk, Cb cb)
{
  ComponentsList *prevList = ctx.clist;
  TemplateDBInfo *prevInfo = ctx.info;
  TemplateComponentsDB *prevDb = ctx.db;
  ComponentsList olist;
  ctx.clist = &olist;
  ctx.info = nullptr;
  ctx.db = nullptr;
  auto sets = ctx.sets;
  ctx.sets = decltype(sets)();
  ctx.load(blk, nullptr);
  ctx.clist = prevList;
  ctx.info = prevInfo;
  ctx.db = prevDb;
  ctx.sets = sets;
  cb(eastl::move(olist));
}

static inline ecs::Object load_object_impl(BlkLoadContext &ctx, const DataBlock &blk)
{
  ecs::Object out_object;
  uint32_t cap = blk.paramCount() + blk.blockCount();
  out_object.reserve(cap);
  load_component_list_impl(ctx, blk, [&out_object](ComponentsList &&clist) {
    for (auto &&objElem : clist)
      out_object.addMember(objElem.first.c_str(), eastl::move(objElem.second));
  });
  if (out_object.size() <= ((cap * 3u) / 4u)) // if we are wasting at least 25% or more (TODO: tune this formula according to real
                                              // world usage)
    out_object.shrink_to_fit();
  return out_object;
}

static inline ecs::Array load_array_impl(BlkLoadContext &ctx, const DataBlock &blk)
{
  ecs::Array out_array;
  out_array.reserve(blk.paramCount() + blk.blockCount());
  load_component_list_impl(ctx, blk, [&out_array](ComponentsList &&clist) {
    for (auto &&arrElem : clist)
      out_array.push_back(eastl::move(arrElem.second));
  });
  if (out_array.size() <= ((out_array.capacity() * 3u) / 4u)) // if we are wasting at least 25% or more (TODO: tune this formula
                                                              // according to real world usage)
    out_array.shrink_to_fit();
  return out_array;
}

template <typename T>
static inline T load_array_impl(BlkLoadContext &ctx, const DataBlock &blk)
{
  T out_array;
  out_array.reserve(blk.paramCount() + blk.blockCount());
  load_component_list_impl(ctx, blk, [&ctx, &out_array](ComponentsList &&clist) {
    for (auto &&arrElem : clist)
    {
      if (arrElem.second.is<typename T::base_type::value_type>())
        out_array.push_back(eastl::move(arrElem.second));
      else
        logerr("Wrong type with key '%s' passed to '%s' in template '%s:%s'", arrElem.first, ctx.blockName, ctx.blkFilePath,
          ctx.templName);
    }
  });
  if (out_array.size() <= ((out_array.capacity() * 3u) / 4u)) // if we are wasting at least 25% or more (TODO: tune this formula
                                                              // according to real world usage)
    out_array.shrink_to_fit();
  return out_array;
}

static void load_object(BlkLoadContext &ctx, const char *objname, const DataBlock &blk)
{
  ecs::Object object = load_object_impl(ctx, blk);
  ctx.clist->emplace_back(objname, eastl::move(object));
}

static void load_array(BlkLoadContext &ctx, const char *arrname, const DataBlock &blk)
{
  ecs::Array array = load_array_impl(ctx, blk);
  ctx.clist->emplace_back(arrname, eastl::move(array));
}

static void load_shared_object(BlkLoadContext &ctx, const char *objname, const DataBlock &blk)
{
  ecs::Object object = load_object_impl(ctx, blk);
  ctx.clist->emplace_back(objname, ecs::SharedComponent<ecs::Object>(eastl::move(object)));
}

static void load_shared_array(BlkLoadContext &ctx, const char *arrname, const DataBlock &blk)
{
  ecs::Array array = load_array_impl(ctx, blk);
  ctx.clist->emplace_back(arrname, ecs::SharedComponent<ecs::Array>(eastl::move(array)));
}

template <typename T>
static void load_shared_list(BlkLoadContext &ctx, const char *listname, const DataBlock &blk)
{
  ecs::List<T> list = load_array_impl<ecs::List<T>>(ctx, blk);
  ctx.clist->emplace_back(listname, ecs::SharedComponent<ecs::List<T>>(eastl::move(list)));
}

template <typename T>
static void load_list(BlkLoadContext &ctx, const char *listname, const DataBlock &blk)
{
  ecs::List<T> list = load_array_impl<ecs::List<T>>(ctx, blk);
  ctx.clist->emplace_back(listname, eastl::move(list));
}

static void resolve_templates_imports(ecs::EntityManager &mgr, const char *path, const DataBlock &blk, TemplateRefs &templates,
  TemplateRefs &overrides, service_datablock_cb &cb, TemplateDBInfo *info);

static void resolve_one_import(ecs::EntityManager &mgr, TemplateRefs &templates, TemplateRefs &overrides, service_datablock_cb &cb,
  TemplateDBInfo *info, const char *imp_fn, const String &src_folder, bool is_optional)
{
  const dblk::ReadFlags loadBlkFlags = is_optional ? dblk::ReadFlag::ROBUST | dblk::ReadFlag::RESTORE_FLAGS : dblk::ReadFlags();

  bool abs_path = (imp_fn[0] == '#');
  bool mnt_path = (imp_fn[0] == '%');
  if (abs_path)
    imp_fn++;

  if (strchr(imp_fn, '*'))
  {
    Tab<SimpleString> files;
    const char *fn_with_ext = dd_get_fname(imp_fn);
    G_ASSERTF(strchr(imp_fn, '*') >= fn_with_ext, "imp_fn=%s%s", abs_path ? "#" : "", imp_fn);

    String fn_match_re;
    fn_match_re.reserve(i_strlen(fn_with_ext) * 2 + 1);
    for (const char *p = fn_with_ext; *p; p++)
      if (*p == '*')
        fn_match_re += ".*";
      else if (strchr("\\()[].+?^$", *p))
      {
        fn_match_re += '\\';
        fn_match_re += *p;
      }
      else
        fn_match_re += *p;
    fn_match_re += '$';

    RegExp re;
    if (!re.compile(fn_match_re))
    {
      logerr("failed to compile regexp /%s/ (made from %s, got from %s)", fn_match_re, fn_with_ext, imp_fn);
      return;
    }

    find_files_in_folder(files, String(0, "%s%.*s", abs_path ? "" : src_folder.str(), fn_with_ext - imp_fn, imp_fn));
    bool anyFileLoaded = false;
    for (const SimpleString &s : files)
      if (re.test(dd_get_fname(s)))
      {
        DataBlock imp_blk;
        if (dblk::load(imp_blk, s, loadBlkFlags))
        {
          resolve_templates_imports(mgr, NULL, imp_blk, templates, overrides, cb, info);
          anyFileLoaded = true;
        }
      }

    if (!anyFileLoaded && !is_optional)
      logerr("No such files on directory <%s>. Please make import_optional or add files to directory", imp_fn);
  }
  else
  {
    DataBlock imp_blk;
    if (dblk::load(imp_blk, (abs_path || mnt_path) ? imp_fn : (src_folder + imp_fn).str(), loadBlkFlags))
      resolve_templates_imports(mgr, NULL, imp_blk, templates, overrides, cb, info);
    else if (!is_optional)
      logerr("No such file on directory <%s>. Please make import_optional or add file.", imp_fn);
  }
}

static void load_templates_blk_file(ecs::EntityManager &mgr, const char *path, const DataBlock &blk, TemplateRefs &templates,
  TemplateRefs &overrides, service_datablock_cb &cb, TemplateDBInfo *info);

static void resolve_templates_imports(ecs::EntityManager &mgr, const char *path, const DataBlock &blk, TemplateRefs &templates,
  TemplateRefs &overrides, service_datablock_cb &cb, TemplateDBInfo *info)
{
  const char *src_fn = blk.resolveFilename();
  if (src_fn)
  {
    String src_folder;
    src_folder.allocBuffer(i_strlen(src_fn) + 2);
    dd_get_fname_location(src_folder.data(), src_fn);
    dd_append_slash_c(src_folder.data());
    src_folder.updateSz();

    const int importNid = blk.getNameId("import");
    const int importOptionalNid = blk.getNameId("import_optional");
    for (int i = 0, pe = blk.paramCount(); i < pe; ++i)
    {
      const int paramNid = blk.getParamNameId(i);
      if (paramNid == importNid || paramNid == importOptionalNid)
      {
        const bool isOptional = paramNid == importOptionalNid;
        resolve_one_import(mgr, templates, overrides, cb, info, blk.getStr(i), src_folder, isOptional);
      }
    }
  }
  load_templates_blk_file(mgr, path ? path : src_fn, blk, templates, overrides, cb, info);
}

static void load_templates_blk_file(ecs::EntityManager &mgr, const char *path, const DataBlock &blk, TemplateRefs &templates,
  TemplateRefs &overrides, service_datablock_cb &cb, TemplateDBInfo *info)
{
  const int parentNid = blk.getNameId("_use"), trackedNid = blk.getNameId("_tracked"), replicatedNid = blk.getNameId("_replicated"),
            hiddenNid = blk.getNameId("_hidden"), overrideNid = blk.getNameId("_override"),
            skipInitialNid = blk.getNameId("_skipInitialReplication"), singletonNid = blk.getNameId("_singleton"),
            replNid = blk.getNameId("_replicate"), trackNid = blk.getNameId("_track"),
            ignoreNid = blk.getNameId("_ignoreInitialReplication"), hideNid = blk.getNameId("_hide"), infoNid = blk.getNameId("_info");

  // second pass, create templates
  Template::component_set ignored, replicatedSet, trackedSet, hiddenSet, tagsSkippedSet;
  BlkLoadContext lctx{nullptr, nullptr, path, /*blockName*/ nullptr, &templates, info, &mgr, VALUE_NID(blk), GROUP_NID(blk), replNid,
    trackNid, ignoreNid, hideNid, infoNid};
  templates.reserve(blk.blockCount());
  for (int i = 0, be = blk.blockCount(); i < be; ++i)
  {
    const DataBlock *tblk_ = blk.getBlock(i);
    if (!tblk_)
      continue;
    ignored.clear();
    replicatedSet.clear();
    trackedSet.clear();
    tagsSkippedSet.clear();
    const DataBlock &tblk = *tblk_;
    const char *templName = tblk.getBlockName();
    const char *templatePath = path;
    G_UNUSED(templatePath);
    if (is_reserved_name(templName))
    {
      if (strcmp(templName, "_component") == 0)
      {
        const char *compName = tblk.getStr("name", NULL);
        const char *compType = tblk.getStr("type", NULL);
        if (compName && compType)
        {
          const char *ftags = tblk.getStr(TAGS_NAME, NULL);
          const bool doLoad = !ftags || (info && ecs::filter_needed(ftags, info->filterTags)); //-V595
          if (doLoad)
          {
            RegisterRet ret = register_component(mgr, compName, eastl::string_view(compType), tblk);
            if (ret == RegisterRet::INVALID_TYPE)
              logerr("_component block at %s is invalid. Type %s for %sis unknown", path, compType, compName);
            else if (ret != RegisterRet::OK)
              logerr("_component block at %s is invalid. Can't register %s:%s.", path, compName, compType);
            else if (info)
              info->updateComponentTags(ECS_HASH_SLOW(compName), ftags, templName, path);
          }
        }
        else
        {
          logerr("_component block at %s is invalid. it should have 'type:t=' and 'name:t='!", path);
        }
      }
      else if (strncmp(templName, "_infoCommon", 11) == 0)
      {
        if (strcmp(templName, "_infoCommonComponents") == 0)
        {
          for (int j = 0, jn = tblk.blockCount(); j < jn; ++j)
          {
            const DataBlock *_infoBlk = tblk.getBlock(j);
            if (info && !info->addMetaData(TemplateDBInfo::MetaInfoType::COMMON_COMPONENT, _infoBlk->getBlockName(), *_infoBlk))
              logerr("Dup '%s' common metadata in %s block at %s", _infoBlk->getBlockName(), templName, path);
          }
        }
        else if (strcmp(templName, "_infoCommonTemplates") == 0)
        {
          for (int j = 0, jn = tblk.blockCount(); j < jn; ++j)
          {
            const DataBlock *_infoBlk = tblk.getBlock(j);
            if (info && !info->addMetaData(TemplateDBInfo::MetaInfoType::COMMON_TEMPLATE, _infoBlk->getBlockName(), *_infoBlk))
              logerr("Dup '%s' common metadata in %s block at %s", _infoBlk->getBlockName(), templName, path);
          }
        }
        else
        {
          logerr("%s block at %s is unsupported metadata block", templName, path);
        }
      }
      else if (cb)
        cb(tblk);
      continue;
    }
    if (!TemplateDB::is_valid_template_name(templName))
    {
      logerr("invalid template name <%s> while parsing <%s>", templName, path);
      continue;
    }
    const bool isOverride = tblk.getBoolByNameId(overrideNid, false);
#if !DAECS_EXTENSIVE_CHECKS
    if (!isOverride)
    {
      auto tIt = templates.find(templName);
      if (tIt != templates.end() && tIt->isValid())
      {
        // logwarn("ignore already existing template '%s' while parsing '%s'", templName, path);
        continue;
      }
    }
#endif
#if DAGOR_DBGLEVEL > 0
    const char *pathString = templates.pathNames.getName(templates.pathNames.addNameId(templatePath));
#else
    const char *pathString = "";
#endif
    TemplateRefs &tlist = isOverride ? overrides : templates;
    Template::ParentsList parentNameIds;
    for (int j = 0, je = tblk.paramCount(); j < je; ++j)
      if (tblk.getParamNameId(j) == parentNid)
        parentNameIds.push_back(tlist.ensureTemplate(tblk.getStr(j)));
    parentNameIds.shrink_to_fit();

    ComponentsMap cmap;
    ComponentsList clist;
    lctx.clist = &clist; // -V506
    lctx.templName = templName;
    lctx.sets.ignored = &ignored;
    lctx.sets.tracked = &trackedSet;
    lctx.sets.replicated = &replicatedSet;
    lctx.sets.hidden = info ? &hiddenSet : NULL;
    lctx.sets.tagsSkipped = info ? &tagsSkippedSet : NULL;
    lctx.mgr = &mgr;

    lctx.load(tblk, nullptr, [&cmap] /*on_empty_comp_type_cb*/ (HashedConstString hName) { cmap[hName] = ChildComponent(); });
    for (auto &&c : clist)
    {
      HashedConstString cname = ECS_HASH_SLOW(c.first.c_str());
      if (info && hiddenSet.count(cname.hash))
        info->componentFlags[cname.hash] |= DataComponent::DONT_REPLICATE;
      if (templates.addComponent(cname.hash, c.second.getUserType(), cname.str)) // todo: add tags
        cmap[cname] = eastl::move(c.second);
      else
      {
        logerr("component <%s|0x%X> has type of <%s> in template <%s:%s>, while it is already typed as <%s> in other template.",
          c.first.c_str(), cname.hash, mgr.getComponentTypes().findTypeName(templates.getComponentType(cname.hash)), path, templName,
          mgr.getComponentTypes().findTypeName(c.second.getUserType()));
      }
    }
    for (int j = 0, je = tblk.paramCount(); j < je; ++j)
      if (tblk.getParamNameId(j) == skipInitialNid)
      {
        tokenize_const_string(tblk.getStr(j), ",", [&](eastl::string_view comp_name) {
          ignored.emplace(ecs_hash(comp_name));
          return true;
        });
      }

    auto buildList = [&](const DataBlock &blk, ecs::Template::component_set &list, const ecs::Template::component_set &skipped,
                       const char *name, int name_id, bool check) {
      G_UNUSED(name);
      G_UNUSED(check);
      int paramId = -1;
      while ((paramId = blk.findParam(name_id, paramId)) >= 0)
        if (blk.getParamType(paramId) == DataBlock::TYPE_STRING)
        {
          const char *cName = blk.getStr(paramId);
          const ecs::component_t c = ECS_HASH_SLOW(cName).hash;
          if (skipped.count(c)) // it was skipped in tags
            continue;
#if DAECS_EXTENSIVE_CHECKS
          if (strpbrk(cName, "|^.$"))
            logerr("template %s:%s in list of %s has regexp <%s> instead of list of components. Regexps are not supported", templName,
              path, name, cName);
          if (eastl::find_if(cmap.begin(), cmap.end(), [&](auto cl) { return cl.first == c; }) == cmap.end())
          {
            if (check)
              logerr("template %s:%s in list of %s has component %s, which is not within it's components", templName, path, name,
                cName);
            continue;
          }
#endif
          list.insert(c);
        }
    };
    const bool check = false; // todo: replace me with true, as soon as we fix bugs
    buildList(tblk, hiddenSet, tagsSkippedSet, "hidden", hiddenNid, check);
    buildList(tblk, trackedSet, tagsSkippedSet, "tracked", trackedNid, check);
    buildList(tblk, replicatedSet, tagsSkippedSet, "replicated", replicatedNid, check);
    // buildList(tblk, ignoredSet, ignoredList, "skipInitialReplication", skipInitialNid, true);// we cant enable it now, as it will
    // fail checks

    G_ASSERT(lctx.sets.tracked == &trackedSet);
    Template templ(templName, eastl::move(cmap), eastl::move(trackedSet), eastl::move(replicatedSet), eastl::move(ignored),
      tblk.getBoolByNameId(singletonNid, false), pathString);
    eastl::reverse(parentNameIds.begin(), parentNameIds.end()); // apply from back to front
#if DAECS_EXTENSIVE_CHECKS
    if (!isOverride)
    {
      auto eit = templates.find(templName);
      if (eit != templates.end() && eit->isValid())
      {
        if (eit->isEqual(templ, mgr.getComponentTypes()) && eit->getParents().size() == parentNameIds.size() &&
            eastl::equal(parentNameIds.begin(), parentNameIds.end(), eit->getParents().begin()))
        {
          // logwarn("ignore already existing template '%s' while parsing '%s'", templName, path);
        }
        else
        {
          debug("%d %d==%d %d", eit->isEqual(templ, mgr.getComponentTypes()), eit->getParents().size(), parentNameIds.size(),
            eit->getParents().size() == parentNameIds.size() &&
              eastl::equal(parentNameIds.begin(), parentNameIds.end(), eit->getParents().begin()));
          logerr("ignore already existing DIFFERENT template '%s' while parsing '%s' ", templName, path);
        }
        continue;
      }
    }
#endif
    if (!isOverride)
      templates.emplace(eastl::move(templ), eastl::move(parentNameIds));
    else
      overrides.amend(eastl::move(templ), eastl::move(parentNameIds));
  }
}

void load_templates_blk_file(ecs::EntityManager &mgr, const char *debug_path_name, const DataBlock &blk, TemplateRefs &templates,
  TemplateDBInfo *info, service_datablock_cb cb)
{
  TemplateRefs overrides(mgr);
  resolve_templates_imports(mgr, debug_path_name, blk, templates, overrides, cb, info);
  templates.amend(eastl::move(overrides));
}

bool load_templates_blk_file(ecs::EntityManager &mgr, const char *path, TemplateRefs &templates, TemplateDBInfo *info)
{
  // 'templates' intentionally not cleared because this function might be intentionally called several times
  DataBlock blk;
  if (!dblk::load(blk, path, dblk::ReadFlag::ROBUST_IN_REL))
    return false;
  load_templates_blk_file(mgr, path, blk, templates, info);
  return true;
}

void load_templates_blk(ecs::EntityManager &mgr, dag::ConstSpan<SimpleString> fnames, TemplateRefs &out_templates,
  TemplateDBInfo *info)
{
  for (auto &fname : fnames)
    G_VERIFYF(load_templates_blk_file(mgr, fname.str(), out_templates, info), "failed to load template '%s'", fname.str());
}

void create_entities_blk(ecs::EntityManager &mgr, const DataBlock &blk, const char *blk_path,
  const on_entity_creation_scheduled_t &on_creation_scheduled_cb, const create_entity_async_cb_t &on_entity_created_cb,
  const on_import_beginend_cb_t &on_import_beginend_cb)
{
  int enid = blk.getNameId("entity"), tnid = blk.getNameId("_template"), impnid = blk.getNameId("import"),
      scnid = blk.getNameId("scene");
  const TemplateDBInfo &info = mgr.getTemplateDB().info();
  BlkLoadContext lctx;
  lctx.mgr = &mgr;
  lctx.blkFilePath = blk_path;
  lctx.valueNid = VALUE_NID(blk);
  lctx.groupNid = GROUP_NID(blk);
  for (int i = 0; i < blk.blockCount(); ++i)
    if (blk.getBlock(i)->getBlockNameId() == enid)
    {
      ComponentsList clistTemp;
      const DataBlock &eb = *blk.getBlock(i);
      {
        const char *filterTags = eb.getStr(TAGS_NAME, NULL);
        if (filterTags && !ecs::filter_needed(filterTags, info.filterTags))
          continue;
      }

      const char *templName = eb.getStrByNameId(tnid, "");

      if (*templName && templName[0])
      {
        if (eb.paramCountById(tnid) != 1)
          logerr("Only single '_template:t' is accepted in entity. (template=%s at '%s')", templName, blk_path);
      }

      lctx.clist = &clistTemp; // -V506
      lctx.templName = templName;
      lctx.load(eb, nullptr);

      ComponentsMap cmap; // todo: replace with ComponentsInitializer
      if (on_creation_scheduled_cb)
      {
        for (auto &a : clistTemp)
          cmap[ECS_HASH_SLOW(a.first.c_str())] = ChildComponent(a.second);
      }
      else
      {
        for (auto &&a : clistTemp)
          cmap[ECS_HASH_SLOW(a.first.c_str())] = eastl::move(a.second); // todo: actually we'd better write explicit move constructor,
                                                                        // to move strings as well
      }

      EntityId eid;
      char buf[64];
      if (!*templName) // todo: remove this branch!
      {
        logerr("can not create entity without template name! add _template:t=template_name!");
        static int autogenT = 0;
        snprintf(buf, sizeof(buf), "__autogen%d", autogenT++);
        templName = buf;
        ComponentsMap cmapCopy(cmap);
        ecs::Template templD(buf, eastl::move(cmapCopy), Template::component_set(), Template::component_set(),
          Template::component_set(), false);
        mgr.addTemplate(eastl::move(templD));
      }
      if (*templName)
      {
        create_entity_async_cb_t ccb(on_entity_created_cb);
        eid = mgr.createEntityAsync(templName, ComponentsInitializer(eastl::move(cmap)), eastl::move(ccb));
      }
      if (eid)
      {
        if (on_creation_scheduled_cb)
          on_creation_scheduled_cb(eid, templName, eastl::move(clistTemp));
      }
      else
        logerr("Creation entity of template '%s' failed!", templName);
    }
    else if (blk.getBlock(i)->getBlockNameId() == impnid)
    {
      const DataBlock &iblk = *blk.getBlock(i);
      for (int pi = 0, pe = iblk.paramCount(); pi < pe; ++pi)
        if (iblk.getParamNameId(pi) == scnid)
        {
          const char *importScene = iblk.getStr(pi);
          if (on_import_beginend_cb)
            on_import_beginend_cb(importScene, /*begin*/ true);
          if (dd_file_exist(importScene))
          {
            DataBlock importBlk;
            if (dblk::load(importBlk, importScene, dblk::ReadFlag::ROBUST_IN_REL)) // not ROBUST in dev to see syntax errors
              create_entities_blk(mgr, importBlk, importScene, on_creation_scheduled_cb, on_entity_created_cb, on_import_beginend_cb);
          }
          else
            logerr("Missing import scene <%s> in <%s>", importScene, blk_path);
          if (on_import_beginend_cb)
            on_import_beginend_cb(importScene, /*begin*/ false);
        }
    }
    else
      logerr("unknown block in entities_blk, block name = %s", blk.getBlock(i)->getBlockName());
}

void load_es_order(ecs::EntityManager &mgr, const DataBlock &blk, Tab<SimpleString> &out_es_order, Tab<SimpleString> &out_es_skip,
  dag::ConstSpan<const char *> tags)
{
  ska::flat_hash_set<eastl::string> validTags;
  for (auto t : tags)
    validTags.insert(t);
  mgr.setEsTags(tags);
  for (int i = 0; i < blk.blockCount(); ++i)
  {
    const DataBlock *esBlk = blk.getBlock(i);
    const char *name = esBlk->getBlockName();
    if (name[0] == '-')
      out_es_skip.push_back() = name + 1;
    else
    {
      const char *tag = esBlk->getStr(ES_TAGS_NAME, nullptr);
#if DAGOR_DBGLEVEL > 0
      if (tag)
      {
        ecs::EntitySystemDesc *system = find_if_systems([name](EntitySystemDesc *sys) { return strcmp(sys->name, name) == 0; });
        if (system && system->getTags())
        {
          if (strcmp(system->getTags(), tag) == 0)
            logerr("ES system <%s> has code-defined tags of <%s>, which are same as in es_order. Just remove it from blk!", name,
              system->getTags(), tag);
          else
            logerr("ES system <%s> has code-defined tags of <%s>, while es_order also defines <%s>. Inspect it and fix!", name,
              system->getTags(), tag);
        }
        else
        {
          logerr("ES system <%s> defines tags <%s> in es_order.blk! replace with code define.\n"
                 "in c++: <ECS_TAG(tag_name)>; in das: tag=tag_name; in squirrel <tags=\"tag_name\">",
            name, tag);
        }
      }
#endif
      if (ecs::filter_needed(tag, validTags))
        out_es_order.push_back() = name;
      else
        out_es_skip.push_back() = name;
    }
  }
}

void load_comp_list_from_blk(ecs::EntityManager &mgr, const DataBlock &blk, ComponentsList &clist)
{
  BlkLoadContext lctx;
  lctx.clist = &clist;
  lctx.valueNid = VALUE_NID(blk);
  lctx.groupNid = GROUP_NID(blk);
  lctx.mgr = &mgr;
  lctx.load(blk, nullptr);
}

} // namespace ecs
