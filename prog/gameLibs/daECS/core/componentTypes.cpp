#include <daECS/core/componentTypes.h>
#include <daECS/core/sharedComponent.h>
#include <daECS/core/entityManager.h>

G_STATIC_ASSERT(ecs::ComponentTypeInfo<int>::can_be_tracked &&ecs::ComponentTypeInfo<ecs::Array>::can_be_tracked
    &&ecs::ComponentTypeInfo<ecs::Object>::can_be_tracked &&ecs::ComponentTypeInfo<vec4f>::can_be_tracked
      &&ecs::ComponentTypeInfo<Point3>::can_be_tracked &&ecs::ComponentTypeInfo<ecs::string>::can_be_tracked
        &&ecs::ComponentTypeInfo<E3DCOLOR>::can_be_tracked &&ecs::ComponentTypeInfo<ecs::EntityId>::can_be_tracked);

#define DECL_LIST_TYPE(lt, t) G_STATIC_ASSERT(ecs::ComponentTypeInfo<ecs::lt>::can_be_tracked);
ECS_DECL_LIST_TYPES
#undef DECL_LIST_TYPE

// we don't need default serializer for pod types (as null is as good as default).
ECS_REGISTER_TYPE(ecs::EntityId, nullptr);
ECS_REGISTER_TYPE(bool, nullptr);
ECS_REGISTER_TYPE(uint8_t, nullptr);
ECS_REGISTER_TYPE(uint16_t, nullptr);
ECS_REGISTER_TYPE(uint32_t, nullptr);
ECS_REGISTER_TYPE(uint64_t, nullptr);
ECS_REGISTER_TYPE(int8_t, nullptr);
ECS_REGISTER_TYPE(int16_t, nullptr);
ECS_REGISTER_TYPE(int, nullptr);
ECS_REGISTER_TYPE(int64_t, nullptr);
ECS_REGISTER_TYPE(float, nullptr);
ECS_REGISTER_TYPE(E3DCOLOR, nullptr);
ECS_REGISTER_TYPE(Point2, nullptr);
ECS_REGISTER_TYPE(Point3, nullptr);
ECS_REGISTER_TYPE(Point4, nullptr);
ECS_REGISTER_TYPE(IPoint2, nullptr);
ECS_REGISTER_TYPE(IPoint3, nullptr);
ECS_REGISTER_TYPE(IPoint4, nullptr);
ECS_REGISTER_TYPE(DPoint3, nullptr);
ECS_REGISTER_TYPE(TMatrix, nullptr);
ECS_REGISTER_TYPE(vec4f, nullptr);
ECS_REGISTER_TYPE(bbox3f, nullptr);
ECS_REGISTER_TYPE(mat44f, nullptr);
ECS_REGISTER_SHARED_TYPE(ecs::Object, nullptr);
ECS_REGISTER_SHARED_TYPE(ecs::Array, nullptr);
ECS_REGISTER_SHARED_TYPE(ecs::string, nullptr);

#if defined(_MSC_VER)
#pragma warning(push)           // Save warning settings.
#pragma warning(disable : 4592) // Disable symbol will be dynamically initialized (implementation limitation) warning (compiler bug)
#endif
ecs::ChildComponent ecs::Object::emptyAttrRO;
ecs::ChildComponent ecs::Array::emptyAttrRO;
#if defined(_MSC_VER)
#pragma warning(pop) // Restore warnings to previous state.
#endif

#include <daECS/core/baseIo.h>
#include <daECS/core/entityComponent.h>

namespace ecs
{
ComponentSerializer default_serializer;
size_t pull_components_type = 1;
extern const int MAX_STRING_LENGTH = 32768; // just for safety. Keep string size reasonable please!
};                                          // namespace ecs


class StringSerializer final : public ecs::ComponentSerializer
{
public:
  void serialize(ecs::SerializerCb &cb, const void *data, size_t sz, ecs::component_type_t hint) override
  {
    G_ASSERT(hint == ecs::ComponentTypeInfo<ecs::string>::type);
    G_UNUSED(hint);
    G_UNUSED(sz);
    ecs::write_string(cb, ((const ecs::string *)data)->c_str(), ecs::MAX_STRING_LENGTH);
  }
  bool deserialize(const ecs::DeserializerCb &cb, void *data, size_t sz, ecs::component_type_t hint) override
  {
    G_ASSERT(hint == ecs::ComponentTypeInfo<ecs::string>::type);
    G_UNUSED(hint);
    G_UNUSED(sz);
    char buf[ecs::MAX_STRING_LENGTH];
    int len = ecs::read_string(cb, buf, sizeof(buf));
    if (len < 0)
      return false;
    *((ecs::string *)data) = buf;
    return true;
  }
};
static StringSerializer string_serializer;
ECS_REGISTER_RELOCATABLE_TYPE(ecs::string, &string_serializer);

class ObjectSerializer final : public ecs::ComponentSerializer
{
public:
  typedef ecs::Object SerializedType;
  void serialize(ecs::SerializerCb &cb, const void *data, size_t sz, ecs::component_type_t hint) override
  {
    G_ASSERT(hint == ecs::ComponentTypeInfo<SerializedType>::type);
    cb.write(data, sz, hint);
  }
  bool deserialize(const ecs::DeserializerCb &cb, void *data, size_t sz, ecs::component_type_t hint) override
  {
    G_ASSERT(hint == ecs::ComponentTypeInfo<SerializedType>::type);
    return cb.read(data, sz, hint);
  }
};
static ObjectSerializer object_serializer;
ECS_REGISTER_RELOCATABLE_TYPE(ecs::Object, &object_serializer);

class ArraySerializer final : public ecs::ComponentSerializer
{
public:
  typedef ecs::Array SerializedType;
  void serialize(ecs::SerializerCb &cb, const void *data, size_t sz, ecs::component_type_t hint) override
  {
    G_ASSERT(hint == ecs::ComponentTypeInfo<SerializedType>::type);
    G_UNUSED(hint);
    G_UNUSED(sz);
    const SerializedType &arr = *((const SerializedType *)data);
    ecs::write_compressed(cb, arr.size());
    for (auto &it : arr)
      ecs::serialize_child_component(it, cb);
  }
  bool deserialize(const ecs::DeserializerCb &cb, void *data, size_t sz, ecs::component_type_t hint) override
  {
    G_ASSERT(hint == ecs::ComponentTypeInfo<SerializedType>::type);
    G_UNUSED(hint);
    G_UNUSED(sz);
    SerializedType &arr = *((SerializedType *)data);
    arr.clear();
    uint32_t cnt;
    if (!ecs::read_compressed(cb, cnt))
      return false;
    arr.reserve(cnt);
    for (uint32_t i = 0; i < cnt; ++i)
    {
      if (ecs::MaybeChildComponent mbcomp = ecs::deserialize_child_component(cb))
        arr.push_back(eastl::move(*mbcomp));
      else
        return false;
    }
    return true;
  }
};
static ArraySerializer array_serializer;
ECS_REGISTER_RELOCATABLE_TYPE(ecs::Array, &array_serializer);

template <typename SerializedType>
class ListSerializer final : public ecs::ComponentSerializer
{
public:
  using ItemType = typename SerializedType::value_type;

  static constexpr size_t itemSizeInBits = CHAR_BIT * ecs::ComponentTypeInfo<ItemType>::size;
  static constexpr ecs::component_type_t itemComponentType = ecs::ComponentTypeInfo<ItemType>::type;

  void serialize(ecs::SerializerCb &cb, const void *data, size_t sz, ecs::component_type_t hint) override
  {
    G_ASSERT(hint == ecs::ComponentTypeInfo<SerializedType>::type);
    G_UNUSED(hint);
    G_UNUSED(sz);
    const SerializedType &list = *((const SerializedType *)data);
    ecs::write_compressed(cb, list.size());
    for (const ItemType &item : list)
      cb.write(&item, itemSizeInBits, itemComponentType);
  }
  bool deserialize(const ecs::DeserializerCb &cb, void *data, size_t sz, ecs::component_type_t hint) override
  {
    G_ASSERT(hint == ecs::ComponentTypeInfo<SerializedType>::type);
    G_UNUSED(hint);
    G_UNUSED(sz);
    SerializedType &list = *((SerializedType *)data);
    list.clear();
    uint32_t cnt;
    if (!ecs::read_compressed(cb, cnt))
      return false;
    list.resize(cnt);
    for (ItemType &item : list)
      if (!cb.read(&item, itemSizeInBits, itemComponentType))
        return false;
    return true;
  }
};

#define DECL_LIST_TYPE(lt, t)                          \
  static ListSerializer<ecs::lt> lt##_list_serializer; \
  ECS_REGISTER_SHARED_TYPE(ecs::lt, nullptr);          \
  ECS_REGISTER_RELOCATABLE_TYPE(ecs::lt, &lt##_list_serializer);

ECS_DECL_LIST_TYPES
#undef DECL_LIST_TYPE

// ecs20 compatibility
// todo: remove or optimize
const char *ecs::ChildComponent::getStr() const
{
  G_ASSERTF(is<ecs::string>() && !isNull(), "0x%X != 0x%X", +ComponentTypeInfo<ecs::string>::type, componentType);
  return is<ecs::string>() ? get<ecs::string>().c_str() : "";
}
// ecs20 compatibility
// todo: remove or optimize
const char *ecs::ChildComponent::getOr(const char *v) const { return is<ecs::string>() ? get<ecs::string>().c_str() : v; }

ecs::ChildComponent &ecs::ChildComponent::operator=(const char *v)
{
  *this = ecs::string(v);
  return *this;
}

const char *ecs::Object::getMemberOr(const HashedConstString &name, const char *def) const
{
  auto it = find_as(name);
  if (it == end())
    return def;
  return it->second.getOr(def);
}

bool ecs::Object::slowIsEqual(const ecs::Object &a) const
{
  G_ASSERT(!noCollisions && a.size() == size() && !a.noCollisions);
  auto ahi = a.hashContainer.begin();
  auto bhi = hashContainer.begin();
  for (auto ai = a.begin(), bi = begin(), be = end(); bi != be; ++bi, ++ai, ++ahi, ++bhi)
    if (*ahi != *bhi || ai->first != bi->first || ai->second != bi->second)
      return false;
  return true;
}

void ecs::Object::validate() const
{
  G_ASSERT(hashContainer.size() == container.size());
  if (!size())
    return;
  auto hi = hashContainer.begin();
  for (auto i = begin(), e = end(); i != e; ++i, ++hi)
    G_ASSERT(*hi == ECS_HASH_SLOW(i->first.c_str()).hash);

  if (size() <= 1)
    return;
  if (noCollisions)
  {
    for (auto hi = hashContainer.begin(), he = hashContainer.end() - 1; hi != he; ++hi)
      G_ASSERT(*hi < *(hi + 1));
  }
  else
  {
    auto hi = hashContainer.begin();
    for (auto i = begin(), e = end() - 1; i != e; ++i, ++hi)
    {
      G_ASSERT(*hi <= *(hi + 1));
      if (*hi == *(hi + 1))
        G_ASSERT(strcmp(i->first.c_str(), (i + 1)->first.c_str()) < 0);
    }
  }
}

ecs::ChildComponent &ecs::Object::insertWithCollision(hash_container_t::const_iterator hashIt, const ecs::HashedConstString str)
{
  G_ASSERT(!noCollisions);
  if (hashIt != hashContainer.end() && *hashIt == str.hash)
  {
    auto it = container.begin() + (hashIt - hashContainer.begin());
    for (auto e = hashContainer.end(); hashIt != e; ++hashIt, ++it)
    {
      if (*hashIt != str.hash)
        break;
      int cmp = strcmp(it->first.c_str(), str.str);
      if (cmp == 0)
        return it->second;
      if (cmp > 0)
        break;
    }
  }
  if (hashIt == hashContainer.end())
  {
    hashContainer.emplace_back(str.hash);
    return container.emplace_back(value_type{str.str, ChildComponent{}}).second;
  }
  auto it = container.begin() + (hashIt - hashContainer.begin());
  hashContainer.emplace(hashIt, str.hash);
  return container.emplace(it, value_type{str.str, ChildComponent{}})->second;
}

ecs::Object::const_iterator ecs::Object::findAsWithCollision(hash_container_t::const_iterator hashIt,
  const ecs::HashedConstString str) const
{
  G_ASSERT(!noCollisions);
  G_ASSERTF_RETURN(str.str, end(), "find_as can't be called without string on Object with hash collisions");
  auto start = container.begin() + (hashIt - hashContainer.begin());
  for (auto it = start, e = container.end(); it != e; ++it, ++hashIt)
  {
    if (*hashIt != str.hash)
      break;
    if (it->first == str.str)
      return it;
  }
  return end();
}

ecs::type_index_t ecs::find_component_type_index(ecs::component_type_t component_type, ecs::EntityManager *emgr)
{
  return (emgr ? emgr : g_entity_mgr.get())->getComponentTypes().findType(component_type);
}
