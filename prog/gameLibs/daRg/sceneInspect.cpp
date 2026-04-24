// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "guiScene.h"
#include "inputStack.h"
#include "scriptUtil.h"

#include <daRg/dag_element.h>
#include <daRg/dag_properties.h>
#include <daRg/dag_renderObject.h>
#include <daRg/dag_behavior.h>
#include <util/dag_string.h>
#include <osApiWrappers/dag_localConv.h>
#include <sqmodules/sqmodules.h>


namespace darg
{


static bool text_contains(const eastl::string &haystack, const char *needle, bool case_sensitive)
{
  if (case_sensitive)
    return haystack.find(needle) != eastl::string::npos;

  int nlen = (int)strlen(needle);
  if (nlen == 0)
    return true;
  if ((int)haystack.length() < nlen)
    return false;
  for (int i = 0; i <= (int)haystack.length() - nlen; ++i)
    if (dd_strnicmp(haystack.c_str() + i, needle, nlen) == 0)
      return true;
  return false;
}


typedef eastl::vector_map<const Behavior *, const char *> BhvNameMap;

static void build_bhv_name_map(SqModules *modules, BhvNameMap &out)
{
  out.clear();
  if (!modules)
    return;

  Sqrat::Object *bhvModulePtr = modules->findNativeModule("daRg.behaviors");
  if (!bhvModulePtr)
    return;

  HSQUIRRELVM vm = modules->getVM();
  HSQOBJECT hObj = bhvModulePtr->GetObject();
  sq_pushobject(vm, hObj);
  sq_pushnull(vm);
  while (SQ_SUCCEEDED(sq_next(vm, -2)))
  {
    SQUserPointer ptr = nullptr;
    const char *name = nullptr;
    if (sq_gettype(vm, -1) == OT_USERPOINTER && SQ_SUCCEEDED(sq_getuserpointer(vm, -1, &ptr)) &&
        SQ_SUCCEEDED(sq_getstring(vm, -2, &name)))
    {
      out[(const Behavior *)ptr] = name;
    }
    sq_pop(vm, 2);
  }
  sq_pop(vm, 2);
}


static void format_element_line(eastl::string &out, const Element *elem, int indent, const BhvNameMap &bhv_names)
{
  // Indentation
  for (int i = 0; i < indent; ++i)
    out += "  ";

  // Screen box [x,y wxh]
  const ScreenCoord &sc = elem->screenCoord;
  char buf[256];
  snprintf(buf, sizeof(buf), "[%d,%d %dx%d]", (int)sc.screenPos.x, (int)sc.screenPos.y, (int)sc.size.x, (int)sc.size.y);
  out += buf;

  // Render object type
  const char *robj_name = get_rendobj_factory_name(elem->rendObjType);
  if (robj_name)
  {
    out += ' ';
    out += robj_name;
  }

  // Text content
  if (!elem->props.text.empty())
  {
    out += " \"";
    if (elem->props.text.length() <= 50)
    {
      out += elem->props.text.c_str();
    }
    else
    {
      out.append(elem->props.text.c_str(), 47);
      out += "...";
    }
    out += '"';
  }

  // Behaviors
  if (!elem->behaviors.empty())
  {
    out += " {";
    bool first = true;
    for (const Behavior *bhv : elem->behaviors)
    {
      auto it = bhv_names.find(bhv);
      if (!first)
        out += ',';
      out += it != bhv_names.end() ? it->second : "?";
      first = false;
    }
    out += '}';
  }

  // Hidden/clipped markers
  if (elem->isHidden())
    out += " [hidden]";
  else if (elem->bboxIsClippedOut())
    out += " [clipped]";

  // Source location from script builder closure
  if (!elem->props.scriptBuilder.IsNull() && elem->props.scriptBuilder.GetType() == OT_CLOSURE)
  {
    String src_name;
    get_closure_full_name(elem->props.scriptBuilder, src_name);
    if (!src_name.empty() && src_name[0] != '<')
    {
      out += " -- ";
      out += src_name.c_str();
    }
  }

  out += '\n';
}


static bool is_non_visual(const Element *elem)
{
  // Behavior do not make an element visual, but when there are behaviors
  // in an element's hierarchy, this information mey be useful for inspection
  return elem->rendObjType < 0 && elem->props.text.empty() && elem->behaviors.empty();
}


struct WalkParams
{
  int maxDepth;
  bool includeHidden;
  bool skipNonVisual;
  int maxElements;
  int elemCount;
  const BhvNameMap *bhvNames;
};


// Unfiltered tree walk
static void walk_tree(const Element *elem, int depth, WalkParams &p, eastl::string &out)
{
  if (p.elemCount >= p.maxElements)
    return;
  if (depth > p.maxDepth)
    return;
  if (!p.includeHidden && (elem->isHidden() || elem->isDetached()))
    return;
  if (p.skipNonVisual && is_non_visual(elem) && elem->children.empty())
    return;

  bool output = !p.skipNonVisual || !is_non_visual(elem);
  if (output)
  {
    format_element_line(out, elem, depth, *p.bhvNames);
    ++p.elemCount;
  }

  int childDepth = output ? depth + 1 : depth;
  for (const Element *child : elem->children)
    walk_tree(child, childDepth, p, out);
}


// Filtered tree walk: returns true if this element or any descendant matches
static bool walk_tree_filtered(const Element *elem, int depth, WalkParams &p, const char *filter_text, eastl::string &out)
{
  if (p.elemCount >= p.maxElements)
    return false;
  if (depth > p.maxDepth)
    return false;
  if (!p.includeHidden && (elem->isHidden() || elem->isDetached()))
    return false;

  bool self_matches = !elem->props.text.empty() && text_contains(elem->props.text, filter_text, false);

  // Recurse into children, collecting their output into a temp buffer
  eastl::string children_out;
  bool any_child_matches = false;
  for (const Element *child : elem->children)
  {
    if (walk_tree_filtered(child, depth + 1, p, filter_text, children_out))
      any_child_matches = true;
  }

  if (self_matches || any_child_matches)
  {
    format_element_line(out, elem, depth, *p.bhvNames);
    ++p.elemCount;
    out += children_out;
    return true;
  }

  return false;
}


void GuiScene::inspectSceneTree(eastl::string &out, int max_depth, const char *filter_text, bool include_hidden, bool skip_non_visual,
  int max_elements)
{
  out.clear();

  BhvNameMap bhvNames;
  build_bhv_name_map(getModuleMgr(), bhvNames);

  WalkParams p;
  p.maxDepth = max_depth;
  p.includeHidden = include_hidden;
  p.skipNonVisual = skip_non_visual;
  p.maxElements = max_elements;
  p.elemCount = 0;
  p.bhvNames = &bhvNames;

  for (auto &[screen_id, screen] : screens)
  {
    if (!screen || !screen->etree.root)
      continue;

    if (screens.size() > 1)
    {
      char hdr[64];
      snprintf(hdr, sizeof(hdr), "--- screen %d%s ---\n", screen_id, screen.get() == focusedScreen ? " (focused)" : "");
      out += hdr;
    }

    if (filter_text && filter_text[0])
      walk_tree_filtered(screen->etree.root, 0, p, filter_text, out);
    else
      walk_tree(screen->etree.root, 0, p, out);
  }

  if (p.elemCount >= max_elements)
    out += "\n... truncated (max_elements reached)\n";
}


void GuiScene::inspectElementsAtPos(eastl::string &out, int x, int y)
{
  out.clear();

  if (!focusedScreen || !focusedScreen->etree.root)
  {
    out = "No focused screen";
    return;
  }

  Point2 pos((float)x, (float)y);
  InputStack stack;
  int hier_order = 0;
  focusedScreen->etree.root->traceHit(pos, &stack, 0, hier_order);

  if (stack.stack.empty())
  {
    out = "No elements at this position";
    return;
  }

  BhvNameMap bhvNames;
  build_bhv_name_map(getModuleMgr(), bhvNames);

  int idx = 0;
  char prefix[32];
  for (auto it = stack.stack.begin(); it != stack.stack.end(); ++it, ++idx)
  {
    snprintf(prefix, sizeof(prefix), "#%d ", idx);
    out += prefix;
    format_element_line(out, it->elem, 0, bhvNames);
  }
}


static void collect_text_matches(const Element *elem, const char *text, bool case_sensitive, int max_results,
  dag::Vector<const Element *> &results)
{
  if ((int)results.size() >= max_results)
    return;
  if (elem->isHidden() || elem->isDetached())
    return;

  if (!elem->props.text.empty() && text_contains(elem->props.text, text, case_sensitive))
    results.push_back(elem);

  for (const Element *child : elem->children)
    collect_text_matches(child, text, case_sensitive, max_results, results);
}


static void format_ancestor_path(eastl::string &out, const Element *elem)
{
  dag::Vector<const Element *> ancestors;
  for (const Element *e = elem->getParent(); e; e = e->getParent())
    ancestors.push_back(e);

  out += "   path: ";
  for (int i = (int)ancestors.size() - 1; i >= 0; --i)
  {
    const Element *a = ancestors[i];
    char buf[64];
    snprintf(buf, sizeof(buf), "[%d,%d %dx%d]", (int)a->screenCoord.screenPos.x, (int)a->screenCoord.screenPos.y,
      (int)a->screenCoord.size.x, (int)a->screenCoord.size.y);
    out += buf;

    const char *rn = get_rendobj_factory_name(a->rendObjType);
    if (rn)
    {
      out += ' ';
      out += rn;
    }
    out += " > ";
  }
  out += "this\n";
}


void GuiScene::findElementsByText(eastl::string &out, const char *text, bool case_sensitive, int max_results)
{
  out.clear();

  if (!text || !text[0])
  {
    out = "No search text provided";
    return;
  }

  dag::Vector<const Element *> results;

  for (auto &[screen_id, screen] : screens)
  {
    if (!screen || !screen->etree.root)
      continue;
    collect_text_matches(screen->etree.root, text, case_sensitive, max_results, results);
  }

  if (results.empty())
  {
    out = "No elements found";
    return;
  }

  BhvNameMap bhvNames;
  build_bhv_name_map(getModuleMgr(), bhvNames);

  char prefix[32];
  for (int i = 0; i < (int)results.size(); ++i)
  {
    snprintf(prefix, sizeof(prefix), "#%d ", i);
    out += prefix;
    format_element_line(out, results[i], 0, bhvNames);
    format_ancestor_path(out, results[i]);
  }
}


} // namespace darg
