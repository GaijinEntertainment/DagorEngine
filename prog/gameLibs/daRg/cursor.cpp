// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "cursor.h"
#include "guiScene.h"

#include "component.h"
#include <daRg/dag_stringKeys.h>
#include <daRg/dag_transform.h>


namespace darg
{

Cursor::Cursor(GuiScene *scene_) : scene(scene_), etree(scene_, nullptr), renderList(), hotspot(0, 0) { etree.canHandleInput = false; }


Cursor::~Cursor()
{
  clear();
  scene->removeCursor(this);
}


void Cursor::clear()
{
  renderList.clear();
  etree.clear();
}


void Cursor::readHotspot()
{
  if (etree.root && etree.root->children.size() == 1)
  {
    Sqrat::Table &scriptDesc = etree.root->children[0]->props.scriptDesc;
    Sqrat::Array customHotspot = scriptDesc.RawGetSlot(scene->getStringKeys()->hotspot);
    if (!customHotspot.IsNull() && customHotspot.Length() == 2)
    {
      hotspot.x = customHotspot.GetValue<int>(0);
      hotspot.y = customHotspot.GetValue<int>(1);
    }
    else
      hotspot.zero();
  }
  else
    hotspot.zero();
}


void Cursor::init(const Sqrat::Object &markup)
{
  G_ASSERTF(!etree.root, "Repeated call to init()");

  HSQUIRRELVM sqvm = scene->getScriptVM();
  StringKeys *csk = scene->getStringKeys();

  Sqrat::Table markupWithRoot(sqvm);
  markupWithRoot.SetValue(csk->children, markup);

  {
    Component rootComp;
    Component::build_component(rootComp, markupWithRoot, csk, markup);

    int rebuildResult = 0;
    etree.root = etree.rebuild(sqvm, etree.root, rootComp, nullptr, rootComp.scriptBuilder, 0, rebuildResult);

    readHotspot();
  }

  G_ASSERT(etree.root);

  if (etree.root)
  {
    etree.root->transform = new Transform();
    etree.root->recalcLayout();
    etree.root->callScriptAttach(scene);
  }

  rebuildStacks();
  scene->addCursor(this);
}


void Cursor::rebuildStacks()
{
  renderList.clear();
  if (etree.root)
  {
    ElemStacks stacks;
    ElemStackCounters counters;
    stacks.rlist = &renderList;
    etree.root->putToSortedStacks(stacks, counters, 0, false);
  }
  renderList.afterRebuild();
}


void Cursor::update(float dt)
{
  int updRes = etree.update(dt);
  if (updRes & ElementTree::RESULT_ELEMS_ADDED_OR_REMOVED)
    rebuildStacks();
}


class SqratCursorClass : public Sqrat::Class<Cursor, Sqrat::NoCopy<Cursor>>
{
public:
  SqratCursorClass(HSQUIRRELVM v) : Sqrat::Class<Cursor, Sqrat::NoCopy<Cursor>>(v, "Cursor") { BindConstructor(CustomNew, 1); }

  static SQInteger CustomNew(HSQUIRRELVM vm)
  {
    if (sq_gettop(vm) < 2 || (sq_gettype(vm, 2) != OT_CLOSURE && sq_gettype(vm, 2) != OT_TABLE))
      return sq_throwerror(vm, "Cursor requires component as argument");

    GuiScene *scene = GuiScene::get_from_sqvm(vm);
    Sqrat::Var<Sqrat::Object> markup(vm, 2);
    Cursor *inst = new Cursor(scene);
    inst->init(markup.value);
    Sqrat::ClassType<Cursor>::SetManagedInstance(vm, 1, inst);
    return 0;
  }
};


void Cursor::bind_script(Sqrat::Table &exports)
{
  SqratCursorClass cursorClass(exports.GetVM());
  exports.Bind("Cursor", cursorClass);
}


} // namespace darg
