#include <daRg/dag_stringKeys.h>
#include <daRg/dag_element.h>

#include "scriptUtil.h"
#include "dargDebugUtils.h"


namespace darg
{


Layout::Layout() :
  flowType(FLOW_PARENT_RELATIVE),
  hAlign(ALIGN_LEFT),
  vAlign(ALIGN_TOP),
  hPlacement(PLACE_DEFAULT),
  vPlacement(PLACE_DEFAULT),
  margin(0, 0, 0, 0),
  padding(0, 0, 0, 0),
  gap(0)
{}


bool Layout::size_spec_from_obj(const Sqrat::Object &obj, SizeSpec &res, const char **err_msg)
{
  SQObjectType tp = obj.GetType();
  if (tp == OT_USERPOINTER)
  {
    SizeSpec::Mode mode = script_cast_from_userpointer<SizeSpec::Mode>(obj);
    G_ASSERT(mode == SizeSpec::CONTENT);
    res.reset(mode);
  }
  else if (tp == OT_USERDATA)
  {
    SizeSpec *ss = NULL;
    HSQOBJECT hObj = obj.GetObject();
    if (SQ_SUCCEEDED(sq_direct_getuserdata(&hObj, (SQUserPointer *)&ss)))
    {
      G_ASSERT(ss);
      res = *ss;
    }
  }
  else if (tp & SQOBJECT_NUMERIC)
  {
    res.mode = SizeSpec::PIXELS;
    res.value = obj.Cast<float>();
  }
  else if (tp == OT_NULL)
  {
    res.mode = SizeSpec::PIXELS;
    res.value = 0;
  }
  else
  {
    res.mode = SizeSpec::PIXELS;
    res.value = 0;
    if (err_msg)
      *err_msg = "Unsupported size field type. Expected numeric, user pointer, user data";
    return false;
  }
  return true;
}


bool Layout::size_spec_from_array(const Sqrat::Array &arr, SizeSpec res[2], const char **err_msg)
{
  bool ok = true;

  int len = arr.Length();
  if (len != 2)
  {
    if (err_msg)
      *err_msg = "Unexpected array size of 'pos' or 'size', must be 2 elements";
    ok = false;
  }

  res[0].reset();
  res[1].reset();

  for (int axis = 0, n = ::min(2, len); axis < n; ++axis)
  {
    Sqrat::Object obj = arr.RawGetSlot(SQInteger(axis));
    if (!size_spec_from_obj(obj, res[axis], err_msg))
      ok = false;
  }

  return ok;
}


bool Layout::readPos(const Sqrat::Object &pos_obj, const char **err_msg)
{
  invalidatePos();

  SQObjectType tp = pos_obj.GetType();
  if (tp == OT_NULL)
    return true;
  else if (tp == OT_ARRAY)
    return size_spec_from_array(pos_obj, pos, err_msg);
  else
  {
    if (err_msg)
      *err_msg = "Unexpected 'pos' field type. Expected array";
    return false;
  }
}


bool Layout::readSize(const Sqrat::Object &size_obj, const char **err_msg)
{
  invalidateSize();

  SQObjectType tp = size_obj.GetType();
  if (tp == OT_NULL)
  {
    size[0].reset(SizeSpec::CONTENT);
    size[1].reset(SizeSpec::CONTENT);
    return true;
  }
  else if (tp == OT_USERPOINTER)
  {
    SizeSpec::Mode mode = script_cast_from_userpointer<SizeSpec::Mode>(size_obj);
    G_ASSERT(mode == SizeSpec::CONTENT);
    size[0].reset(mode);
    size[1].reset(mode);
    return true;
  }
  else if (tp == OT_USERDATA)
  {
    SizeSpec *ss = NULL;
    HSQOBJECT hObj = size_obj.GetObject();
    if (SQ_SUCCEEDED(sq_direct_getuserdata(&hObj, (SQUserPointer *)&ss)) && ss)
    {
      size[0] = *ss;
      size[1] = *ss;
      return true;
    }
    else
    {
      if (err_msg)
        *err_msg = "Failed to get size field userdata";
      return false;
    }
  }
  else if (tp == OT_ARRAY)
    return size_spec_from_array(size_obj, size, err_msg);
  else
  {
    if (err_msg)
      *err_msg = "Unexpected 'size' field type. Expected array, user pointer or userdata";
    return false;
  }
}


void Layout::read(const Element *elem, const Properties &props, const StringKeys *csk)
{
  const Sqrat::Table &scriptDesc = props.scriptDesc;
  SqStackChecker stackCheck(scriptDesc.GetVM());

  const char *errMsg = nullptr;

  Sqrat::Object posObj = scriptDesc.RawGetSlot(csk->pos);
  if (!readPos(posObj, &errMsg))
    darg_assert_trace_var(errMsg, scriptDesc, csk->pos);

  stackCheck.check();

  Sqrat::Object sizeObj = scriptDesc.RawGetSlot(csk->size);
  if (!readSize(sizeObj, &errMsg))
    darg_assert_trace_var(errMsg, scriptDesc, csk->size);

  stackCheck.check();

  flowType = props.getInt<LayoutFlow>(csk->flow, FLOW_PARENT_RELATIVE);
  hAlign = props.getInt<ElemAlign>(csk->halign, ALIGN_LEFT);
  vAlign = props.getInt<ElemAlign>(csk->valign, ALIGN_TOP);
  hPlacement = props.getInt<ElemAlign>(csk->hplace, PLACE_DEFAULT);
  vPlacement = props.getInt<ElemAlign>(csk->vplace, PLACE_DEFAULT);


  if (!script_parse_offsets(elem, scriptDesc.RawGetSlot(csk->margin), &margin.x, &errMsg))
    darg_assert_trace_var(errMsg, scriptDesc, csk->margin);

  if (!script_parse_offsets(elem, scriptDesc.RawGetSlot(csk->padding), &padding.x, &errMsg))
    darg_assert_trace_var(errMsg, scriptDesc, csk->padding);

  Sqrat::Object gapObj = scriptDesc.RawGetSlot(csk->gap);
  if (gapObj.GetType() & SQOBJECT_NUMERIC)
    gap = ::ceilf(gapObj.Cast<float>());
  else
    gap = 0;

  if (!size_spec_from_obj(scriptDesc.RawGetSlot(csk->minWidth), minSize[0], &errMsg))
    darg_assert_trace_var(errMsg, scriptDesc, csk->minWidth);

  if (!size_spec_from_obj(scriptDesc.RawGetSlot(csk->minHeight), minSize[1], &errMsg))
    darg_assert_trace_var(errMsg, scriptDesc, csk->minHeight);

  if (!size_spec_from_obj(scriptDesc.RawGetSlot(csk->maxWidth), maxSize[0], &errMsg))
    darg_assert_trace_var(errMsg, scriptDesc, csk->maxWidth);

  if (!size_spec_from_obj(scriptDesc.RawGetSlot(csk->maxHeight), maxSize[1], &errMsg))
    darg_assert_trace_var(errMsg, scriptDesc, csk->maxHeight);
}


void Layout::invalidateSize()
{
  size[0].reset();
  size[1].reset();
}


void Layout::invalidatePos()
{
  pos[0].reset();
  pos[1].reset();
}


} // namespace darg
