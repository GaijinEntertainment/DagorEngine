// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daRg/dag_stringKeys.h>
#include <daRg/dag_element.h>

#include "scriptUtil.h"
#include "dargDebugUtils.h"


namespace darg
{


const Offsets Layout::zeroOffsets{};
const SizeSpec Layout::defaultSizeSpec;


Layout::Layout() :
  ext(nullptr),
  gap(0),
  flowType(FLOW_PARENT_RELATIVE),
  hAlign(ALIGN_LEFT),
  vAlign(ALIGN_TOP),
  hPlacement(PLACE_DEFAULT),
  vPlacement(PLACE_DEFAULT)
{}


Layout::~Layout() { delete ext; }


LayoutExtended *Layout::ensureExt()
{
  if (!ext)
    ext = new LayoutExtended();
  return ext;
}


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
    if (SQ_SUCCEEDED(sq_obj_getuserdata(&hObj, (SQUserPointer *)&ss, nullptr)))
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
  pos[0].reset();
  pos[1].reset();

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


bool Layout::readSize(const Sqrat::Object &size_obj, const char **err_msg) { return read_size(size_obj, size, err_msg); }


bool Layout::read_size(const Sqrat::Object &size_obj, SizeSpec res[2], const char **err_msg)
{
  res[0].reset();
  res[1].reset();

  SQObjectType tp = size_obj.GetType();
  if (tp == OT_NULL)
  {
    res[0].reset(SizeSpec::CONTENT);
    res[1].reset(SizeSpec::CONTENT);
    return true;
  }
  else if (tp == OT_USERPOINTER)
  {
    SizeSpec::Mode mode = script_cast_from_userpointer<SizeSpec::Mode>(size_obj);
    G_ASSERT(mode == SizeSpec::CONTENT);
    res[0].reset(mode);
    res[1].reset(mode);
    return true;
  }
  else if (tp == OT_USERDATA)
  {
    SizeSpec *ss = NULL;
    HSQOBJECT hObj = size_obj.GetObject();
    if (SQ_SUCCEEDED(sq_obj_getuserdata(&hObj, (SQUserPointer *)&ss, nullptr)) && ss)
    {
      res[0] = *ss;
      res[1] = *ss;
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
    return size_spec_from_array(size_obj, res, err_msg);
  else if (tp & SQOBJECT_NUMERIC)
  {
    SizeSpec ss;
    if (!size_spec_from_obj(size_obj, ss, err_msg))
      return false;
    res[0] = ss;
    res[1] = ss;
    return true;
  }
  else
  {
    if (err_msg)
      *err_msg = "Unexpected 'size' field type. Expected array, user pointer or userdata";
    return false;
  }
}


template <typename T>
static T read_enum_prop(const Properties &props, const Sqrat::Object &key, T def, int min, int after_max, const char *err_msg)
{
  int val = props.getInt(key, int(def));
  if (val >= min && val < after_max)
    return (T)val;

  darg_assert_trace_var(err_msg, props.scriptDesc, key);
  return def;
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

  flowType = read_enum_prop<LayoutFlow>(props, csk->flow, FLOW_PARENT_RELATIVE, FLOW_PARENT_RELATIVE, _NUM_FLOW_TYPES, "Invalid flow");
  hAlign = read_enum_prop<ElemAlign>(props, csk->halign, ALIGN_LEFT, ALIGN_LEFT, _NUM_ALIGNMENTS, "Invalid halign");
  vAlign = read_enum_prop<ElemAlign>(props, csk->valign, ALIGN_TOP, ALIGN_TOP, _NUM_ALIGNMENTS, "Invalid valign");
  hPlacement = read_enum_prop<ElemAlign>(props, csk->hplace, PLACE_DEFAULT, PLACE_DEFAULT, _NUM_ALIGNMENTS, "Invalid hplace");
  vPlacement = read_enum_prop<ElemAlign>(props, csk->vplace, PLACE_DEFAULT, PLACE_DEFAULT, _NUM_ALIGNMENTS, "Invalid vplace");

  Sqrat::Object gapObj = scriptDesc.RawGetSlot(csk->gap);
  if (gapObj.GetType() & SQOBJECT_NUMERIC)
    gap = ::ceilf(gapObj.Cast<float>());
  else
    gap = 0;

  Sqrat::Object marginObj = scriptDesc.RawGetSlot(csk->margin);
  Sqrat::Object paddingObj = scriptDesc.RawGetSlot(csk->padding);
  Sqrat::Object minWidthObj = scriptDesc.RawGetSlot(csk->minWidth);
  Sqrat::Object minHeightObj = scriptDesc.RawGetSlot(csk->minHeight);
  Sqrat::Object maxWidthObj = scriptDesc.RawGetSlot(csk->maxWidth);
  Sqrat::Object maxHeightObj = scriptDesc.RawGetSlot(csk->maxHeight);

  bool needExt = (marginObj.GetType() != OT_NULL || paddingObj.GetType() != OT_NULL || minWidthObj.GetType() != OT_NULL ||
                  minHeightObj.GetType() != OT_NULL || maxWidthObj.GetType() != OT_NULL || maxHeightObj.GetType() != OT_NULL);

  if (needExt)
  {
    LayoutExtended *e = ensureExt();
    *e = LayoutExtended();

    if (!script_parse_offsets(elem, marginObj, e->margin.ptr(), &errMsg))
      darg_assert_trace_var(errMsg, scriptDesc, csk->margin);

    if (!script_parse_offsets(elem, paddingObj, e->padding.ptr(), &errMsg))
      darg_assert_trace_var(errMsg, scriptDesc, csk->padding);

    if (!size_spec_from_obj(minWidthObj, e->minSize[0], &errMsg))
      darg_assert_trace_var(errMsg, scriptDesc, csk->minWidth);

    if (!size_spec_from_obj(minHeightObj, e->minSize[1], &errMsg))
      darg_assert_trace_var(errMsg, scriptDesc, csk->minHeight);

    if (!size_spec_from_obj(maxWidthObj, e->maxSize[0], &errMsg))
      darg_assert_trace_var(errMsg, scriptDesc, csk->maxWidth);

    if (!size_spec_from_obj(maxHeightObj, e->maxSize[1], &errMsg))
      darg_assert_trace_var(errMsg, scriptDesc, csk->maxHeight);
  }
  else
  {
    delete ext;
    ext = nullptr;
  }
}


} // namespace darg
