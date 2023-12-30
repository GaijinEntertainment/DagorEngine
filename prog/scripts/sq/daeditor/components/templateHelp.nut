from "%darg/ui_imports.nut" import *

let ecs = require("ecs")
let txt = require("%daeditor/components/text.nut").dtext

let entity_editor = require("entity_editor")


function modifyReduceExtends(arr) {
  let cnt = arr.len()
  if (cnt < 2)
    return
  let path1 = arr[cnt-2]
  let path2 = arr[cnt-1]
  let len1 = path1.len()
  let len2 = path2.len()
  if (len1 > len2)
    return
  for (local i = 0; i < len1-1; i++)
    if (path1[i] != path2[i])
      return
  local slen1 = 0
  for (local i = 0; i < len1; i++)
    slen1 += path1[i].len()
  if (slen1 >= 50)
    return
  local name = path1.top()
  path1.pop()
  path1.append($"{name}, {path2[len1-1]}")
  for (local i = len1; i < len2; i++)
    path1.append(path2[i])
  arr.pop()
}

function getTemplateExtendsReduced(templ, path) {
  local result = []
  let numParents = templ.getNumParentTemplates()
  if (numParents > 0 && (numParents <= (5-path.len()) || path.len() < 1)) {
    for (local i = numParents-1; i >= 0; i--) {
      let parent = templ.getParentTemplate(i)
      path.append(parent.getName())
      result.extend(getTemplateExtendsReduced(parent, path))
      path.pop()
      modifyReduceExtends(result)
    }
  }
  else {
    if (numParents > 0)
      path.append("...")
    if (path.len() > 0)
      result.append(clone path)
    if (numParents > 0)
      path.pop()
  }
  return result
}


function getTemplateTemplsDescs(templ, addedTempls, need_tpl_desc, extendBy, insertTo) {
  local result = []

  let templName = templ.getName()
  if (addedTempls?[templName]!=null)
    return result
  addedTempls[templName] <- true

  let db = ecs.g_entity_mgr.getTemplateDB()

  let metaInfo = db.getTemplateMetaInfo(templName)
  if (metaInfo != null) {
    if (need_tpl_desc) {
      result.append({ type = "template", name = templName, text = metaInfo?.desc ?? "" })
      need_tpl_desc = false
    }

    if (metaInfo?.extendBy != null) {
      let num_by = metaInfo.extendBy?.paramCount() ?? 0
      for (local i = 0; i < num_by; i++)
        extendBy[metaInfo.extendBy.getParamValue(i)] <- true
    }

    if (metaInfo?.insertTo != null) {
      let num_to = metaInfo.insertTo?.paramCount() ?? 0
      for (local i = 0; i < num_to; i++)
        insertTo[metaInfo.insertTo.getParamValue(i)] <- true
    }
  }

  let numParents = templ.getNumParentTemplates()
  for (local i = numParents-1; i >= 0; i--) {
    let parent = templ.getParentTemplate(i)
    let parentName = parent.getName()
    let tpls = parentName?.split("+") ?? []
    foreach (tpl_name in tpls) {
      let parentTempl = db.getTemplateByName(tpl_name)
      if (parentTempl==null)
        continue
      result.extend(getTemplateTemplsDescs(parentTempl, addedTempls, need_tpl_desc, extendBy, insertTo))
    }
  }

  return result
}


function mkCompMetaInfoText(metaInfo, format="oneLiner") {
  local text = metaInfo?.desc ?? ""

  if (metaInfo?.values != null) {
    if (format == "multiLine") {
      text = $"{text}:\n"
      let num_values = metaInfo.values?.paramCount() ?? 0
      for (local i = 0; i < num_values; i++) {
        let value = metaInfo.values.getParamValue(i)
        text = $"{text}\n{value}"
      }
    }
    else {
      text = $"{text} \{ "
      let num_values = metaInfo.values?.paramCount() ?? 0
      for (local i = 0; i < num_values; i++) {
        let value = metaInfo.values.getParamValue(i)
        if (i > 0)
          text = $"{text}, {value}"
        else
          text = $"{text}{value}"
      }
      text = $"{text} \}"
    }
  }
  if (metaInfo?.min != null || metaInfo?.max != null) {
    if (metaInfo?.min == null)
      text = $"{text} (..{metaInfo.max}]"
    else if (metaInfo?.max == null)
      text = $"{text} [{metaInfo.min}..)"
    else
      text = $"{text} [{metaInfo.min}..{metaInfo.max}]"
  }
  if (metaInfo?.seeAlso != null) {
    text = $"{text} [see: "
    let num_tpls = metaInfo.seeAlso?.paramCount() ?? 0
    for (local i = 0; i < num_tpls; i++) {
      let value = metaInfo.seeAlso.getParamValue(i)
      if (i > 0)
        text = $"{text}, {value}"
      else
        text = $"{text}{value}"
    }
    text = $"{text}]"
  }

  return text
}


function getTemplateCompsDescs(templ, addedTempls, addedComps, valueComps) {
  local result = []

  let templName = templ.getName()
  if (addedTempls?[templName]!=null)
    return result
  addedTempls[templName] <- true

  let db = ecs.g_entity_mgr.getTemplateDB()

  let compNames = templ.getComponentsNames()
  foreach(compName in compNames) {
    if (!db.hasComponentMetaInfo(templName, compName)) {
      if (db.getComponentMetaInfo(compName) != null)
        valueComps[compName] <- true
      continue
    }

    if (addedComps?[compName] == null) {
      addedComps[compName] <- true

      let metaInfo = db.getComponentMetaInfo(compName)
      if (metaInfo != null)
        result.append({ type = "component", name = compName, text = mkCompMetaInfoText(metaInfo, "oneLiner") })
    }
  }

  let numParents = templ.getNumParentTemplates()
  for (local i = numParents-1; i >= 0; i--) {
    let parent = templ.getParentTemplate(i)
    let parentName = parent.getName()
    let tpls = parentName?.split("+") ?? []
    foreach (tpl_name in tpls) {
      let parentTempl = db.getTemplateByName(tpl_name)
      if (parentTempl==null)
        continue
      result.extend(getTemplateCompsDescs(parentTempl, addedTempls, addedComps, valueComps))
    }
  }

  return result
}


let categories = {}
let getCategoryTemplates = function(category) {
  if (categories?[category] != null)
    return categories[category]

  if (categories?[category] == null)
    categories[category] <- []

  let db = ecs.g_entity_mgr.getTemplateDB()

  let tpls = entity_editor?.get_instance()?.getEcsTemplates("*")
  if (tpls == null)
    return []

  foreach(tpl_name in tpls) {
    let metaInfo = db.getTemplateMetaInfo(tpl_name)
    if (metaInfo?.category == category)
      categories[category].append(tpl_name)
  }

  return categories[category]
}


function getTemplateInfo(templName) {
  local extendsListReduced = []
  local extendBy = []
  local insertTo = []
  local valued = {}
  local descs = []
  local other = []
  local tags = []

  local addedTempls = {}
  local extendByTable = {}
  local insertToTable = {}
  let tpls = templName?.split("+") ?? []
  foreach (tpl_name in tpls) {
    let templ = ecs.g_entity_mgr.getTemplateDB().getTemplateByName(tpl_name)
    if (templ==null)
      continue
    extendsListReduced.extend(getTemplateExtendsReduced(templ, []))
    descs.extend(getTemplateTemplsDescs(templ, addedTempls, true, extendByTable, insertToTable))
    tags.extend(templ.getTags())
  }

  foreach(tpl, _val in addedTempls) {
    if (extendByTable?[tpl] != null)
      extendByTable.$rawdelete(tpl)
    if (insertToTable?[tpl] != null)
      insertToTable.$rawdelete(tpl)
  }
  foreach(tpl, _val in extendByTable)
    extendBy.append(tpl)
  foreach(tpl, _val in insertToTable)
    insertTo.append(tpl)
  extendBy.sort()
  insertTo.sort()

  addedTempls = {}
  local addedComps = {}
  local descsAdd = []
  foreach (tpl_name in tpls) {
    let templ = ecs.g_entity_mgr.getTemplateDB().getTemplateByName(tpl_name)
    if (templ==null)
      continue
    descsAdd.extend(getTemplateCompsDescs(templ, addedTempls, addedComps, valued))
  }
  descsAdd.sort(@(a,b) a.name <=> b.name)
  descs.extend(descsAdd)

  foreach(tpl_name in tpls) {
    let metaInfo = ecs.g_entity_mgr.getTemplateDB().getTemplateMetaInfo(tpl_name)
    if (metaInfo?.category != null) {
      let tpl_other = getCategoryTemplates(metaInfo.category)
      foreach(tpl_other_name in tpl_other)
        if (addedTempls?[tpl_other_name] == null)
          other.append(tpl_other_name)
    }
  }

  return {
    name = templName
    extendsListReduced
    extendBy
    insertTo
    valued
    descs
    other
    tags
  }
}


let mkTemplateInfoTag = @(text, fillColor = Color(100,100,200), size = SIZE_TO_CONTENT, textColor = Color(32,32,32)) {
  rendObj = ROBJ_BOX
  size
  borderWidth = 0
  borderRadius = hdpx(4)
  fillColor
  padding = [0,hdpx(1)]
  vplace = ALIGN_CENTER
  margin = [hdpx(2),0]
  children = {
    rendObj = ROBJ_TEXT
    size
    text
    fontSize = hdpx(12)
    color = textColor
  }
}

function requestSliceLongText(inText, rowLen, cb) {
  if (inText.len() <= rowLen) {
    cb(true, inText)
    return
  }
  local firstRow = true
  local text = clone inText
  while (text.len() >= rowLen) {
    local sliceLen = rowLen
    while (sliceLen < text.len() && (text[sliceLen] != ' ' || text[sliceLen+1] == '}'))
      ++sliceLen
    cb(firstRow, text.slice(0, sliceLen))
    text = text.slice(sliceLen)
    firstRow = false
  }
  if (text.len() > 0)
    cb(firstRow, text)
}

function mkTemplateTooltip(templName) {
  let templInfo = getTemplateInfo(templName)

  let nameStyle = { fontSize = hdpx(17) }
  let extendsStyle = { fontSize = hdpx(12), color=Color(120,200,120) }
  let infoStyle = { fontSize = hdpx(14) }
  let paramNameStyle = { fontSize = hdpx(12), color=Color(150,150,255) }
  let paramInfoStyle = { fontSize = hdpx(12), color=Color(150,150,150) }
  let paramValueStyle = { fontSize = hdpx(12), color=Color(220,220,255) }
  let extraInfoStyle = { fontSize = hdpx(12), color=Color(150,150,150) }

  let smallSkipStyle = { fontSize = hdpx(1) }
  let skipStyle = { fontSize = hdpx(4) }
  let longSkipStyle = { fontSize = hdpx(10) }

  let hasExtends = templInfo.extendsListReduced.len() > 0
  let allExtends = templInfo.extendsListReduced.map(function(ext) {
    local text = ""
    foreach (templ in ext) {
      if (text == "")
        text = $" use {templ}"
      else
        text = $"{text} <- {templ}"
    }
    return txt(text, extendsStyle)
  })

  let descNames = []
  let descTexts = []
  local gotDescs = false
  local wasComps = false
  templInfo.descs.map(function(desc) {
    if (desc.type == "template") {
      if (wasComps) {
        wasComps = false
        descNames.append(txt("", skipStyle))
        descTexts.append(txt("", skipStyle))
      }
      gotDescs = true;
      requestSliceLongText(desc.text, 77, function(first, text) {
        descNames.append(txt(first ? $" {desc.name}   " : "", infoStyle))
        descTexts.append(txt(text, infoStyle))
      })
      descNames.append(txt("", skipStyle))
      descTexts.append(txt("", skipStyle))
    }
    else if (desc.type == "component") {
      gotDescs = true
      wasComps = true
      let hasNoMetaValue = templInfo.valued?[desc.name] ?? false
      if (!hasNoMetaValue) {
        requestSliceLongText(desc.text, 90, function(first, text) {
          descNames.append(txt(first ? $"  {desc.name}   " : "", paramNameStyle))
          descTexts.append(txt(text, paramInfoStyle))
        })
      }
      else {
        local compValue = null
        let tpls = templName?.split("+") ?? []
        foreach (tpl_name in tpls) {
          let tpl = ecs.g_entity_mgr.getTemplateDB().getTemplateByName(tpl_name)
          if (tpl != null) {
            let tpl_val = tpl?.getCompValNullable(desc.name)
            if (tpl_val != null)
              compValue = tpl_val
          }
        }
        if (compValue == null)
          compValue = "???"
        let valType = type(compValue)
        if (valType != "bool" && valType != "string" && valType != "integer" && valType != "float")
          compValue = "( ... )"
        descNames.append(txt($"  {desc.name}  ", paramValueStyle))
        descTexts.append(txt($"{compValue}", paramValueStyle))
      }
    }
  })
  if (gotDescs) {
    descNames.append(txt("", longSkipStyle))
    descTexts.append(txt("", longSkipStyle))
  }

  let allDescs = {
    flow = FLOW_HORIZONTAL
    children = [
      { flow = FLOW_VERTICAL, children = descNames }
      { flow = FLOW_VERTICAL, children = descTexts }
    ]
  }

  local allExtendBy = null
  if (templInfo.extendBy.len() > 0) {
    local text = " extendable by "
    foreach(idx, tpl in templInfo.extendBy) {
      if (text.len() >= 120) {
        text = $"{text}, ..."
        break
      }
      if (idx == 0)
        text = $"{text}{tpl}"
      else
        text = $"{text}, {tpl}"
    }
    allExtendBy = {
      flow = FLOW_VERTICAL
      children = [
        txt("", smallSkipStyle)
        txt(text, extraInfoStyle)
      ]
    }
  }

  local allInsertTo = null
  if (templInfo.insertTo.len() > 0) {
    local text = " insert >> "
    foreach(idx, tpl in templInfo.insertTo) {
      if (text.len() >= 120) {
        text = $"{text}, ..."
        break
      }
      if (idx == 0)
        text = $"{text}{tpl}"
      else
        text = $"{text}, {tpl}"
    }
    allInsertTo = {
      flow = FLOW_VERTICAL
      children = [
        txt("", smallSkipStyle)
        txt(text, extraInfoStyle)
      ]
    }
  }

  local allOther = null
  if (templInfo.other.len() > 0) {
    local rows = [ txt("", longSkipStyle) ]
    local text = " Also use: "
    foreach(idx, tpl in templInfo.other) {
      if (text.len() >= 120) {
        text = $"{text},"
        rows.append(txt(text, extraInfoStyle))
        text = $"  {tpl}"
        continue
      }
      if (idx == 0)
        text = $"{text}{tpl}"
      else
        text = $"{text}, {tpl}"
    }
    if (text.len() > 0)
      rows.append(txt(text, extraInfoStyle))
    allOther = {
      flow = FLOW_VERTICAL
      children = rows
    }
  }

  local allTags = []
  local lineLen = 0
  const lineMax = 80
  local lineTags = []
  foreach (idx, tag in templInfo.tags) {
    lineTags.append(mkTemplateInfoTag(tag))
    lineLen += tag.len()
    if (lineLen >= lineMax || idx == templInfo.tags.len()-1) {
      allTags.append({
        gap = hdpx(5)
        flow = FLOW_HORIZONTAL
        children = lineTags
      })
      lineTags = []
      lineLen = 0
    }
  }

  let displayDelay = 0.2

  return {
    rendObj = ROBJ_WORLD_BLUR_PANEL
    fillColor = Color(30, 30, 30, 200)
    children = {
      rendObj = ROBJ_FRAME
      color =  Color(50, 50, 50, 20)
      borderWidth = hdpx(1)
      padding = fsh(1)
      flow = FLOW_VERTICAL
      children = [
        txt($"{templInfo.name}", nameStyle)
        txt("", skipStyle)
        !hasExtends ? null : { flow = FLOW_VERTICAL, children = allExtends }
        allExtendBy
        allInsertTo
        !hasExtends && !allExtendBy && !allInsertTo ? null : txt("", longSkipStyle)
        { flow = FLOW_VERTICAL, children = allDescs }
        { flow = FLOW_VERTICAL, children = allTags }
        allOther
      ]
    }
    animations = [
      { prop = AnimProp.opacity, from = 0, to = 0, duration = displayDelay, play = true }
      { prop = AnimProp.opacity, from = 0, to = 1, delay = displayDelay, duration = 0.1, play = true }
    ]
  }
}

return {
  getTemplateInfo,
  mkTemplateTooltip,
  mkCompMetaInfoText,
}
