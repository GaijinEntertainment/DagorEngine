#include <debug/dag_debug.h>
#include <memory/dag_framemem.h>
#include <daECS/core/dataComponent.h>

namespace ecs
{
CompileComponentRegister *CompileComponentRegister::tail = 0;
LTComponentList *LTComponentList::tail = 0;
void DataComponents::updateComponentToLTInternal()
{
  for (LTComponentList *lt = LTComponentList::tail, *prev = NULL, *next = NULL; lt; prev = lt, lt = next)
  {
    next = lt->next;
    if (auto ltMap = componentToLT.findOr(lt->name, NULL))
    {
      auto oldNext = ltMap->next;
      ltMap->next = lt;
      lt->next = oldNext;
    }
    else
    {
      componentToLT.emplace_new(lt->name, lt);
      lt->next = NULL;
    }
  }
  LTComponentList::tail = NULL;
}

component_index_t DataComponents::createComponent(const HashedConstString name_, type_index_t component_type,
  dag::Span<component_t> deps, ComponentSerializer *io, component_flags_t flags, const ComponentTypes &types)
{
  if (isFilteredOutComponent(name_.hash))
  {
    ECS_VERBOSE_LOG("data component <%s|0x%X> is filtered out", name_.str, name_.hash);
    return INVALID_COMPONENT_INDEX;
  }
  const bool isCopy = (flags & DataComponent::IS_COPY) ? true : false;
  component_t nameHash = name_.hash;
  const char *nameStr = name_.str;
  if (isCopy)
    nameHash = ecs_mem_hash<sizeof(ecs::component_t) * 8>("$", 1, nameHash); // same as string+$
  component_index_t existingId = findComponentId(nameHash);

  type_index_t existingType = INVALID_COMPONENT_TYPE_INDEX;

  if (DAGOR_LIKELY(existingId != INVALID_COMPONENT_INDEX))
  {
    existingType = getComponentById(existingId).componentType;
    const uint32_t oldNameAddr = components.get<NAME>()[existingId];
    if (nameStr != nullptr && oldNameAddr != 0 && strcmp(nameStr, names.getDataRawUnsafe(oldNameAddr)) != 0) //-V522
    {
      logerr("component <%s> with same hash =0x%X as <%s> is already registered, hash collision.", getComponentNameById(existingId),
        nameHash, nameStr);
      return INVALID_COMPONENT_INDEX;
    }
    if (component_type != existingType)
    {
      if (component_type != INVALID_COMPONENT_TYPE_INDEX)
      {
        logerr("component <%s>(0x%X) with type <%s>(%d) is already registered with different type <%s>(%d)!", nameStr, nameHash,
          types.getTypeNameById(component_type), component_type, types.getTypeNameById(getComponentById(existingId).componentType),
          getComponentById(existingId).componentType);
        return INVALID_COMPONENT_INDEX;
      }
      else
        component_type = existingType;
    }
    if (oldNameAddr == 0 && nameStr != nullptr)
      components.get<NAME>()[existingId] = names.addDataRaw(nameStr, strlen(nameStr) + 1); // replace nullptr name with
    return existingId;
  }

  updateComponentToLT();

  const char *usedName = nameStr;

  if (isCopy)
  {
    flags |= DataComponent::DONT_REPLICATE; // we don't replicate copies
    const component_t baseNameHash = name_.hash;
    const component_index_t base = findComponentId(baseNameHash);
    if (base == INVALID_COMPONENT_INDEX)
    {
      logerr("can't create copy for unknown component <%s|0x%X>", usedName, baseNameHash);
      return INVALID_COMPONENT_INDEX;
    }
    const type_index_t baseComponentType = getComponentById(base).componentType;
    if (baseComponentType != component_type && component_type != INVALID_COMPONENT_TYPE_INDEX)
    {
      logerr("can't create copy for component <%s|0x%X> because of type base=%s != copy=%s", usedName, baseNameHash,
        types.getTypeNameById(baseComponentType), types.getTypeNameById(component_type));
      return INVALID_COMPONENT_INDEX;
    }
    component_type = baseComponentType;

    G_ASSERT(components.get<component_index_t>()[base] == INVALID_COMPONENT_INDEX);
    components.get<component_index_t>()[base] = components.size();
    usedName = getComponentNameById(base);
    ECS_VERBOSE_LOG(" %d ecs component <%s> hash<0x%X> of component_type %d<%s> is a copy of %d (%s|0x%X)", components.size(),
      usedName, nameHash, component_type, types.getTypeNameById(component_type), base, usedName, baseNameHash);

    for (LTComponentList *lt = componentToLT.findOr(baseNameHash, NULL); lt; lt = lt->next)
      lt->info.canTrack = true;

#if DAGOR_DBGLEVEL > 0
    if (types.getTypeInfo(component_type).size == 0)
      logerr(" %d ecs component <%s> hash<0x%X> of component_type %d<%s> is a copy of 0 size", components.size(), nameStr, nameHash,
        component_type, types.getTypeNameById(component_type));
#endif
  }
  if (component_type == INVALID_COMPONENT_TYPE_INDEX)
  {
#if DAGOR_DBGLEVEL > 0
    logerr("can't create component %s|0x%X with undefined type", nameStr, name_.hash);
#endif
    return INVALID_COMPONENT_INDEX;
  }
  if (types.getTypeInfo(component_type).flags & COMPONENT_TYPE_NON_TRIVIAL_CREATE)
    flags |= DataComponent::TYPE_HAS_CONSTRUCTOR;
  component_type_t component_type_name = types.getTypeById(component_type);
  if (component_type_name == ComponentTypeInfo<Tag>::type)
    flags |= DataComponent::DONT_REPLICATE;

  if (!isCopy)
  {
    for (LTComponentList *lt = componentToLT.findOr(nameHash, NULL); lt; lt = lt->next)
    {
      G_ASSERT(lt->name == nameHash);
#if DAGOR_DBGLEVEL > 0
      if (lt->nameStr && nameStr && strcmp(lt->nameStr, nameStr) != 0)
      {
        logerr("hash collision for component <%s> nameStr = %s defined at file <%s>, line %d", lt->nameStr, nameStr, lt->fileStr,
          lt->line);
        lt->info.valid = false;
        continue;
      }
      if (lt->type != component_type_name)
        logerr("expected type of component <%s> in get at file <%s>, line %d is <%s|0x%X> registered is <%s|0x%X>", nameStr,
          lt->fileStr, lt->line, types.getTypeNameById(lt->type), lt->type, types.getTypeNameById(component_type),
          component_type_name);
#endif
      if (lt->type != component_type_name)
      {
        lt->info.valid = false; // incorrect type, it can never become correct!
        continue;
      }
      lt->info.cidx = components.size();
      lt->info.canTrack = false; // copies are only created after base components, so currently unknown
    }
  }

  if (io)
  {
    const ComponentTypeFlags typeFlags = types.getTypeInfo(component_type).flags;
    if (!is_pod(typeFlags) && !has_io(typeFlags))
      logerr("component <%s> hash<0x%X> of component_type %d<%s|0x%X> is registered with io, while it's type is not serializable."
             "\nThis can be easily fixed, as it is issue only with recreation, (de)serialize_comp_nameless.",
        nameStr, nameHash, component_type, types.getTypeNameById(component_type), component_type_name);
    else
      flags |= DataComponent::HAS_SERIALIZER;
  }
  G_ASSERT_RETURN(components.size() < INVALID_COMPONENT_INDEX - 1, INVALID_COMPONENT_INDEX);
  componentIndex[nameHash] = components.size();
  const uint32_t nameAddr = usedName ? names.addDataRaw(usedName, strlen(usedName) + 1) : 0;
  components.push_back(DataComponent{component_type, flags, component_type_name}, io, nameHash, INVALID_COMPONENT_INDEX,
    {(uint32_t)dependencies.size()}, nameAddr);

  dependencies.insert(dependencies.end(), deps.begin(), deps.end());


  ECS_VERBOSE_LOG("create %d ecs component <%s> hash<0x%X> of component_type %d<%s|0x%X> flags=%d", components.size() - 1, usedName,
    nameHash, component_type, types.getTypeNameById(component_type), component_type_name, flags);
  G_UNUSED(types);
  return components.size() - 1;
}

void DataComponents::clear()
{
  componentIndex.clear();
  components.clear();
  dependencies.clear();
  names.clear();
  G_VERIFY(names.addDataRaw("", 1) == 0);
}
const char *DataComponents::getHashName(component_index_t id) const
{
  static char buf[16];
  snprintf(buf, sizeof(buf), "#%X", components.get<COMPONENT>()[id]);
  return buf;
}

void DataComponents::initialize(const ComponentTypes &types)
{
  clear();
  // componentIndex.set_empty_key(0);
  // it is essential to have eid as a first component index
  createComponent(ECS_HASH("eid"), types.findType(ECS_HASH("ecs::EntityId").hash), dag::Span<component_t>(), nullptr, 0, types);
  // roots
  eastl::vector<CompileComponentRegister *, framemem_allocator> childs;
  uint32_t max_size = 0;
  for (CompileComponentRegister *start = CompileComponentRegister::tail; start; start = start->next)
  {
    G_ASSERTF(ECS_HASH_SLOW(start->type_name).hash == start->type,
      "data component <%s> has type of <%s> but it's typeid is different <0x%X != 0x%X>", start->name.str, start->type_name,
      ECS_HASH_SLOW(start->type_name).hash, start->type);
    const char *typeName = types.findTypeName(start->type);
    if (!typeName)
    {
      logerr("data component <%s> is registered with type of <%s>, which can not be found in registered types."
             " Potential reasons: you forgot to REGISTER the type (type is only declared),"
             " or the module with registration was excluded by linker, due to know cohesion."
             " In that case try to pull some variable from module",
        start->name.str, start->type_name);
      continue;
    }
    else
    {
      if (strcmp(start->type_name, typeName) != 0)
      {
        logerr("data component <%s> has type of <%s> but it's registered typeName <%s> is different. Hash collision?", start->name.str,
          start->type_name, typeName);
        continue;
      }
    }
    if (filterTags.find(start->name.hash) != filterTags.end())
    {
      ECS_VERBOSE_LOG("data component <%s> is filtered with tag", start->name.str);
      continue;
    }
    if (!start->deps.size())
      createComponent(start->name, types.findType(start->type), dag::Span<component_t>(), start->io, start->flags, types);
    else
    {
      childs.push_back(start);
      max_size = max(max_size, (uint32_t)start->deps.size());
    }
  }

  eastl::vector<component_t, framemem_allocator> deps_components;
  deps_components.reserve(max_size);
  auto tryAddComponent = [&deps_components, this, &types](auto &childs, bool can_skip_optional) -> bool {
    bool somethingAdded = false;
    for (auto i = childs.begin(); i != childs.end(); ++i)
    {
      CompileComponentRegister *start = *i;
      int dep;
      deps_components.resize(0);
      bool allDependendsExist = true;
      for (dep = 0; dep < start->deps.size(); ++dep)
      {
        const char *depName = start->deps[dep];
        bool optional = false;
        if (*depName == '?')
        {
          optional = true;
          depName++;
        }
        HashedConstString depConstString = ECS_HASH_SLOW(depName);
        if (filterTags.find(depConstString.hash) == filterTags.end())
        {
          if (!this->hasComponent(depConstString.hash) && (!optional || (optional && !can_skip_optional)))
          {
            allDependendsExist = false;
            break;
          }
          if (!optional)
            deps_components.push_back(depConstString.hash);
        }
      }
      if (allDependendsExist)
      {
        this->createComponent(start->name, types.findType(start->type), make_span(deps_components), start->io, start->flags, types);
        childs.erase_unsorted(i);
        --i;
        somethingAdded = true;
      }
    }
    return somethingAdded;
  };

  for (; childs.size() && tryAddComponent(childs, false);) // iteratively add dependent components
    ;
  for (; childs.size() && tryAddComponent(childs, true);) // iteratively add dependent components, but skip non existent optional
                                                          // components
    ;
  if (childs.size())
  {
    logerr("there are %d components which depends on undefined components", childs.size());
    for (auto start : childs)
    {
      eastl::string err;
      deps_components.resize(0);
      for (auto dep : start->deps)
      {
        HashedConstString depConstString = ECS_HASH_SLOW(dep);
        if (*dep != '?')
          deps_components.push_back(depConstString.hash);
        if (!hasComponent(depConstString.hash))
          err += eastl::string(eastl::string::CtorSprintf(), "%s%s", err.length() ? ", " : "", dep);
      }
      logerr(" <%s> depends on undefined components: <%s>", start->name.str, err.c_str());
      createComponent(start->name, types.findType(start->type), make_span(deps_components), start->io, start->flags, types);
    }
  }
}
}; // namespace ecs
