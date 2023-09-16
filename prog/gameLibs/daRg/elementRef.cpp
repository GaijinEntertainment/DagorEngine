#include "elementRef.h"
#include <daRg/dag_element.h>
#include <quirrel/sqStackChecker.h>


namespace darg
{

ElementRef::ElementRef() {}

ElementRef::~ElementRef()
{
  if (elem)
    elem->ref = nullptr;
}


ElementRef *ElementRef::get_from_stack(HSQUIRRELVM vm, int idx)
{
  // Non-Sqrat instance operations

  using namespace Sqrat;

  if (!ClassType<ElementRef>::hasClassData(vm))
  {
    G_ASSERT(!"ElementRef class not registered");
    return nullptr;
  }

  AbstractStaticClassData *classType = ClassType<ElementRef>::getStaticClassData().lock().get();

  ElementRef *instance = nullptr;
  if (SQ_FAILED(sq_getinstanceup(vm, idx, (SQUserPointer *)&instance, classType)))
  {
    G_ASSERT(!"Instance of ElementRef required");
    return nullptr;
  }

  G_ASSERT_RETURN(instance, nullptr);

  AbstractStaticClassData *actualType = nullptr;
  sq_gettypetag(vm, idx, (SQUserPointer *)&actualType);
  G_ASSERT_RETURN(classType == actualType, nullptr);
  return instance;
}

// static SQInteger elem_get_comp_desc(HSQUIRRELVM vm)
// {
//   Sqrat::Var<Element *> elem(vm, 1);
//   Sqrat::Var<Sqrat::Object>::push(vm, elem.value->props.scriptDesc);
//   return 1;
// }

#define GET_EREF                                          \
  ElementRef *ref = ElementRef::get_from_stack(vm, 1);    \
  if (!ref)                                               \
    return sq_throwerror(vm, "Object is not ElementRef"); \
  if (!ref->elem)                                         \
    return sq_throwerror(vm, "Element is deleted");


static SQInteger elem_get_width(HSQUIRRELVM vm)
{
  GET_EREF;
  sq_pushfloat(vm, ref->elem->screenCoord.size.x);
  return 1;
}

static SQInteger elem_get_height(HSQUIRRELVM vm)
{
  GET_EREF;
  sq_pushfloat(vm, ref->elem->screenCoord.size.y);
  return 1;
}

static SQInteger elem_get_screen_pos_x(HSQUIRRELVM vm)
{
  GET_EREF;
  sq_pushfloat(vm, ref->elem->screenCoord.screenPos.x);
  return 1;
}

static SQInteger elem_get_screen_pos_y(HSQUIRRELVM vm)
{
  GET_EREF;
  sq_pushfloat(vm, ref->elem->screenCoord.screenPos.y);
  return 1;
}

static SQInteger elem_get_content_width(HSQUIRRELVM vm)
{
  GET_EREF;
  sq_pushfloat(vm, ref->elem->screenCoord.contentSize.x);
  return 1;
}

static SQInteger elem_get_content_height(HSQUIRRELVM vm)
{
  GET_EREF;
  sq_pushfloat(vm, ref->elem->screenCoord.contentSize.y);
  return 1;
}

static SQInteger elem_get_scroll_offs_x(HSQUIRRELVM vm)
{
  GET_EREF;
  sq_pushfloat(vm, ref->elem->screenCoord.scrollOffs.x);
  return 1;
}

static SQInteger elem_get_scroll_offs_y(HSQUIRRELVM vm)
{
  GET_EREF;
  sq_pushfloat(vm, ref->elem->screenCoord.scrollOffs.y);
  return 1;
}

static SQInteger on_delete_elemref_instance(SQUserPointer ptr, SQInteger size)
{
  G_UNUSED(size);
  ElementRef *ref = reinterpret_cast<ElementRef *>(ptr);
  --(ref->scriptInstancesCount);
  G_ASSERT(ref->scriptInstancesCount >= 0);

  if (!ref->scriptInstancesCount)
    delete ref;

  return 0;
}

Sqrat::Object ElementRef::createScriptInstance(HSQUIRRELVM vm)
{
  SqStackChecker check(vm);

  using namespace Sqrat;
  ClassData<ElementRef> *cd = ClassType<ElementRef>::getClassData(vm);
  sq_pushobject(vm, cd->classObj);
  G_VERIFY(SQ_SUCCEEDED(sq_createinstance(vm, -1)));

  G_VERIFY(SQ_SUCCEEDED(sq_setinstanceup(vm, -1, this)));
  sq_setreleasehook(vm, -1, &on_delete_elemref_instance);

  Sqrat::Var<Sqrat::Object> res(vm, -1);
  ++scriptInstancesCount;

  check.restore();

  return res.value;
}

void ElementRef::bind_script(HSQUIRRELVM vm)
{
  ///@class daRg/ElementRef
  Sqrat::Class<ElementRef, Sqrat::NoConstructor<ElementRef>> sqElementRef(vm, "ElementRef");

  (void)sqElementRef
    //.SquirrelFunc("getCompDesc", elem_get_comp_desc)
    .SquirrelFunc("getScreenPosX", elem_get_screen_pos_x, 1, "x")
    .SquirrelFunc("getScreenPosY", elem_get_screen_pos_y, 1, "x")
    .SquirrelFunc("getWidth", elem_get_width, 1, "x")
    .SquirrelFunc("getHeight", elem_get_height, 1, "x")
    .SquirrelFunc("getContentWidth", elem_get_content_width, 1, "x")
    .SquirrelFunc("getContentHeight", elem_get_content_height, 1, "x")
    .SquirrelFunc("getScrollOffsX", elem_get_scroll_offs_x, 1, "x")
    .SquirrelFunc("getScrollOffsY", elem_get_scroll_offs_y, 1, "x");
}

} // namespace darg
