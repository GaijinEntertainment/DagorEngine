// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <propsRegistry/propsRegistry.h>
#include <ioSys/dag_dataBlock.h>
#include <osApiWrappers/dag_direct.h>
#include <ADT/fastHashNameMap.h>
#include <EASTL/string_view.h>
#include <EASTL/vector_set.h>
#include <memory/dag_framemem.h>
#include <debug/dag_debug.h>

struct ClassRegistry
{
  FastHashNameMap propNames;
  eastl::vector_set<propsreg::PropsRegistry *> propsRegistries; // PropsRegistry is statically allocated

  int getPropId(const char *name) const { return propNames.getNameId(name); }

  void clear()
  {
    propNames.clear();
    for (auto reg : propsRegistries)
      reg->clearProps();
    propsRegistries.clear();
    propsRegistries.shrink_to_fit();
  }

  void cleanup()
  {
    propNames.clear();
    for (auto reg : propsRegistries)
      reg->clearProps();
  }

  const char *getNameSlow(int props_id) { return propNames.getNameSlow(props_id); }

  int registerProps(const char *name, const DataBlock *blk)
  {
    G_ASSERT(name != NULL);
    G_ASSERT(blk);
    auto ins = propNames.insert(name);
    if (ins.second) // if inserted
      for (auto reg : propsRegistries)
        reg->registerProps(ins.first, blk);
    return ins.first;
  }

  void addRegistry(propsreg::PropsRegistry &new_registry) { propsRegistries.insert(&new_registry); }
};

static eastl::vector<ClassRegistry> class_registry;
static FastHashNameMapT<eastl::string_view> type_classes; // key (strings) here are statically allocated

int propsreg::register_props(const char *name, const DataBlock *blk, int prop_class_id)
{
  G_ASSERT(prop_class_id >= 0);
  return class_registry[prop_class_id].registerProps(name, blk);
}

int propsreg::register_props(const char *name, const DataBlock *blk, const char *class_name)
{
  int propsClassId = type_classes.getNameId(class_name);
  G_ASSERTF(propsClassId >= 0, "Attempt to load/register props of unknown class '%s'", class_name);
  return class_registry[propsClassId].registerProps(name, blk);
}

int propsreg::register_props(const char *filename, int prop_class_id)
{
  DataBlock blk(framemem_ptr());
#if DAGOR_DBGLEVEL > 0
  if (dd_file_exist(filename))
#endif
    if (dblk::load(blk, filename, dblk::ReadFlag::ROBUST_IN_REL))
      return register_props(dd_get_fname(filename), &blk, prop_class_id); // To consider: drop ".blk" suffix to increase fit in
                                                                          // string's SSO
  logerr("Cannot load '%s' for props class class <%s>(%d)", filename, type_classes.getNameSlow(prop_class_id), prop_class_id);
  return -1;
}

int propsreg::register_props(const char *filename, const char *class_name)
{
  int propsClassId = type_classes.getNameId(class_name);
  G_ASSERTF(propsClassId >= 0, "Attempt to load/register props of unknown class '%s'", class_name);
  return register_props(filename, propsClassId);
}

const char *propsreg::get_props_registered_name(int props_id, const char *class_name)
{
  int propsClassId = type_classes.getNameId(class_name);
  if (propsClassId < 0)
    return "Invalid class";
  return class_registry[propsClassId].getNameSlow(props_id);
}


bool propsreg::is_props_valid(int props_id, int prop_class_id)
{
  return (unsigned)props_id < class_registry[prop_class_id].propNames.size();
}

bool propsreg::is_props_valid(int props_id, const char *class_name)
{
  int classId = type_classes.getNameId(class_name);
  return classId >= 0 && is_props_valid(props_id, classId);
}

int propsreg::get_props_id(const char *name, const char *class_name)
{
  G_ASSERT(name != NULL);
  int classId = get_prop_class_dyn(class_name);
  G_ASSERTF(classId >= 0, "Cannot find class '%s' needed to gather props '%s'", class_name, name);
  int propId = class_registry[classId].getPropId(name);
  if (propId < 0)
    propId = class_registry[classId].getPropId(dd_get_fname(name));
  return propId;
}

void propsreg::clear_registry()
{
  for (auto &creg : class_registry)
    creg.clear();
}

void propsreg::cleanup_props()
{
  for (auto &creg : class_registry)
    creg.cleanup();
}

void propsreg::register_props_class_internal(propsreg::PropsRegistry &new_registry, const char *prop_class, size_t len)
{
  G_ASSERT(prop_class != NULL);
  G_ASSERT(strlen(prop_class) == len);
  int classId = type_classes.addNameId(prop_class, (int)len);
  if (classId >= class_registry.size())
    class_registry.resize(classId + 1);
  class_registry[classId].addRegistry(new_registry);
}

int propsreg::get_prop_class_dyn(const eastl::string_view &prop_class) { return type_classes.getNameId(prop_class); }

void propsreg::register_props_list(const DataBlock *blk, const char *class_name)
{
  int classId = type_classes.getNameId(class_name);
  G_ASSERT(classId >= 0);
  auto &reg = class_registry[classId];
  for (int i = 0; i < blk->blockCount(); ++i)
  {
    const DataBlock *propBlk = blk->getBlock(i);
    reg.registerProps(propBlk->getBlockName(), propBlk);
  }
}

void propsreg::clear_props(const char *class_name)
{
  int classId = type_classes.getNameId(class_name);
  G_ASSERT(classId >= 0);
  class_registry[classId].clear();
}

void propsreg::unreg_props(const char *class_name)
{
  int classId = type_classes.getNameId(class_name);
  G_ASSERT(classId >= 0);
  class_registry[classId].cleanup();
}
