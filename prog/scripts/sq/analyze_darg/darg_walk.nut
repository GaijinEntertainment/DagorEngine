const DEF_MAX_DEPTH = 256
const DEF_MAX_ELEMENTS = 2_000_000
const DEF_MAX_ERRORS_KEPT = 16
const MAX_NESTED_RESULTS = 32
const PATH_TAIL = 12
const MOCK_ELEM_SIZE = 100
const SIZE_BUCKETS = 6

const SETUP_KEYS = [
  "key", "rendObj", "behavior", "bindProps", "children",
  "key", "text", "opacity",
  "pos", "size", "flow", "halign", "valign", "hplace", "vplace", "gap", "margin", "padding",
  "minWidth", "minHeight", "maxWidth", "maxHeight",
  "scrollOffsX", "scrollOffsY", "scrollHandler", "group", "xmbNode", "animations", "watch",
  "transform", "zOrder", "transitions", "subPixel", "onClick", "onDoubleClick", "waitForChildrenFadeOut",
  "cursor", "moveResizeCursors", "clipChildren", "eventHandlers", "disableInput", "stopMouse", "stopHover",
  "stopHotkeys", "joystickScroll", "skipDirPadNav", "inputPassive", "ignoreEarlyClip", "isHidden", "hotkeys"
]
const NO_ROBJ_KEYS = ["children", "size"]
const COLOR_KEYS = ["color", "tint", "brightness"]
const FONT_FX_KEYS = ["fontFx", "fontFxColor", "tint", "fontFxFactor", "fontFxOffsX", "fontFxOffsY"]
const FONT_TEX_KEYS = ["fontTex", "fontTexSu", "fontTexSv", "fontTexBov", "font", "fontSize"]
const IMAGE_KEYS = ["image", "fallbackImage", "keepAspect", "imageHalign", "imageValign", "flipX", "flipY",
  "picSaturate"]
// rendObj values of the shared daRg stub are the names
let ROBJ_KEYS = freeze({
  ROBJ_TEXT = [].extend(COLOR_KEYS, ["password", "spacing", "monoWidth"], FONT_FX_KEYS, FONT_TEX_KEYS,
    ["textOverflow"])
  ROBJ_TEXTAREA = [].extend(COLOR_KEYS, ["textOverflow"], FONT_FX_KEYS, ["font", "fontSize"])
  ROBJ_INSCRIPTION = [].extend(COLOR_KEYS, FONT_FX_KEYS, FONT_TEX_KEYS)
  ROBJ_IMAGE = [].extend(COLOR_KEYS, IMAGE_KEYS, ["imageAffectsLayout"])
  ROBJ_VECTOR_CANVAS = [].extend(COLOR_KEYS, ["fillColor", "tint", "lineWidth", "commands", "draw"])
  ROBJ_BOX = ["borderColor", "tint", "fillColor", "tint", "brightness"].extend(IMAGE_KEYS,
    ["borderWidth", "borderRadius"])
  ROBJ_FRAME = [].extend(COLOR_KEYS, ["fillColor", "tint", "borderWidth"])
  ROBJ_MOVIE = [].extend(COLOR_KEYS, ["picSaturate", "keepAspect", "imageHalign", "imageValign"])
})

// no layout: nonzero sizes avoid zero division
class ElemRefMock {
  function getScreenPosX() { return 0 }
  function getScreenPosY() { return 0 }
  function getWidth() { return MOCK_ELEM_SIZE }
  function getHeight() { return MOCK_ELEM_SIZE }
  function getContentWidth() { return MOCK_ELEM_SIZE }
  function getContentHeight() { return MOCK_ELEM_SIZE }
  function getScrollOffsX() { return 0 }
  function getScrollOffsY() { return 0 }
  function getOverScrollOffsY() { return 0 }
}

function funcLabel(f) {
  let fi = f.getfuncinfos()
  if (fi.native)
    return $"{fi.name}(native)"
  return $"{fi.name}@{fi.src}:{fi.line}"
}

function segLabel(seg) {
  let t = type(seg)
  if (t == "integer")
    return $"[{seg}]"
  if (t == "function")
    return funcLabel(seg)
  if (t == "string")
    return seg
  return t
}

function formatPath(stack) {
  let start = stack.len() > PATH_TAIL ? stack.len() - PATH_TAIL : 0
  let parts = stack.slice(start).map(segLabel)
  if (start > 0)
    parts.insert(0, "...")
  return "/".join(parts)
}

function readSetupKeys(desc, counts) {
  local hits = 0
  local reads = SETUP_KEYS.len()
  foreach (k in SETUP_KEYS)
    if (desc?[k] != null)
      hits++
  if (desc?.onClick != null || desc?.onDoubleClick != null) {
    reads++
    if (desc?.stickCursor != null)
      hits++
  }
  let robj = desc?.rendObj
  let robjKeys = robj == null ? NO_ROBJ_KEYS : (ROBJ_KEYS?[robj] ?? COLOR_KEYS)
  foreach (k in robjKeys)
    if (desc?[k] != null)
      hits++
  counts[0] += reads + robjKeys.len()
  counts[1] += hits
}

function walkTree(root, opts = null) {
  let maxDepth = opts?.maxDepth ?? DEF_MAX_DEPTH
  let maxElements = opts?.maxElements ?? DEF_MAX_ELEMENTS
  let maxErrorsKept = opts?.maxErrorsKept ?? DEF_MAX_ERRORS_KEPT
  let callOnAttach = opts?.callOnAttach ?? true
  let catchErrors = opts?.catchErrors ?? true
  let setupReads = opts?.setupReads ?? true
  let elemRef = opts?.elemRef ?? ElemRefMock()

  let builders = []
  let tables = []
  let childArrays = []
  let errorList = []
  let onPath = {}
  let pathStack = []
  let callableClasses = {}
  let attachQueue = []
  local elements = 0
  local builderCalls = 0
  local attachCalls = 0
  local errors = 0
  local depthReached = 0
  local aborted = false
  let propCounts = [0, 0]
  let sizeHist = array(SIZE_BUCKETS, 0)

  function addError(msg, stack = null) {
    errors++
    if (errorList.len() < maxErrorsKept)
      errorList.append({ msg, path = formatPath(stack ?? pathStack) })
  }

  function readSetup(desc) {
    if (setupReads)
      readSetupKeys(desc, propCounts)
    if (type(desc) != "table")
      return
    let n = desc.len()
    local b = 0
    while (b < SIZE_BUCKETS - 1 && n > (4 << b))
      b++
    sizeHist[b]++
  }

  function resolve(comp, depth, nested) {
    if (aborted)
      return
    if (depth > maxDepth) {
      addError($"depth limit {maxDepth} reached")
      aborted = true
      return
    }
    let t = type(comp)
    if (t == "table" || t == "class") {
      if (comp in onPath) {
        addError("cycle: a description contains itself")
        return
      }
      if (elements >= maxElements) {
        addError($"element limit {maxElements} reached")
        aborted = true
        return
      }
      elements++
      tables.append(comp)
      readSetup(comp)
      if (depth > depthReached)
        depthReached = depth
      if (callOnAttach) {
        let onAttach = comp?.onAttach
        if (type(onAttach) == "function")
          attachQueue.append(onAttach, clone pathStack)
      }
      let children = comp?.children
      if (children == null)
        return
      onPath[comp] <- true
      if (type(children) == "array") {
        childArrays.append(children)
        foreach (i, child in children) {
          pathStack.append(i)
          resolve(child, depth + 1, 0)
          pathStack.pop()
        }
      }
      else
        resolve(children, depth + 1, 0)
      onPath.$rawdelete(comp)
      return
    }
    if (t == "null")
      return
    if (nested > MAX_NESTED_RESULTS) {
      addError("builder results nest too deep")
      return
    }
    let isInstance = t == "instance"
    if (t == "function" || isInstance) {
      if (isInstance) {
        let cls = comp.getclass()
        if (cls not in callableClasses)
          callableClasses[cls] <- comp.$getmetamethod("_call") != null
        if (!callableClasses[cls])
          return
      }
      if (comp in onPath) {
        addError("cycle: a builder is its own descendant")
        return
      }
      pathStack.append(comp)
      local res = null
      local ok = true
      if (!catchErrors)
        res = comp()
      else {
        try {
          res = comp()
        }
        catch (e) {
          ok = false
          addError($"builder: {e}")
        }
      }
      builders.append(comp)
      builderCalls++
      if (ok) {
        onPath[comp] <- true
        resolve(res, depth, nested + 1)
        onPath.$rawdelete(comp)
      }
      pathStack.pop()
      return
    }
    if (t == "array") {
      if (comp in onPath) {
        addError("cycle: an array contains itself")
        return
      }
      childArrays.append(comp)
      onPath[comp] <- true
      foreach (i, child in comp) {
        pathStack.append(i)
        resolve(child, depth, nested)
        pathStack.pop()
      }
      onPath.$rawdelete(comp)
      return
    }
    addError($"invalid component type {t}")
  }

  function callAttach(onAttach) {
    if (onAttach.getarity().num_params > 0)
      onAttach(elemRef)
    else
      onAttach()
  }

  pathStack.append("root")
  resolve(root, 1, 0)
  pathStack.pop()

  for (local i = 0; i < attachQueue.len(); i += 2) {
    attachCalls++
    if (!catchErrors) {
      callAttach(attachQueue[i])
      continue
    }
    try {
      callAttach(attachQueue[i])
    }
    catch (e) {
      addError($"onAttach: {e}", attachQueue[i + 1])
    }
  }

  return {
    elements
    builderCalls
    attachCalls
    maxDepth = depthReached
    errors
    errorList
    propReads = propCounts[0]
    propHits = propCounts[1]
    sizeHist
    kept = { builders, tables, childArrays }
  }
}

return {
  walkTree
  readSetupKeys
  ElemRefMock
}
