from "%darg/ui_imports.nut" import *
from "%darg/laconic.nut" import *
from "string" import regexp
from "%sqstd/ecs.nut" import *

let DataBlock = require("DataBlock")

let nameFilter = require("components/nameFilter.nut")
let textButton = require("%daeditor/components/textButton.nut")
let combobox = require("%daeditor/components/combobox.nut")

let {showMsgbox} = require("editor_msgbox.nut")

let entity_editor = require_optional("entity_editor")
let {propPanelVisible, selectedEntity, editorUnpause} = require("state.nut")
let {registerPerCompPropEdit} = require("%daeditor/propPanelControls.nut")

let math = require("math")
let {startswith} = require("string")


let riSelectShown = Watched(false)
let riSelectSaved = Watched("")
let riSelectValue = Watched("")
local riSelectCB = @(_) null

let riFile = Watched(null)
let riNames = []
let riTags = []

let riNamesGroups = {}

let riPage = Watched(0)
let riPageCount = 25

let riTagsOffset = Watched(0)
let riTagsPageCount = 25

let riTagsShown = Watched(false)
let riSelectTag = Watched("")


const GRPMODE_ALL        = 0
const GRPMODE_PREDEFINED = 1
const GRPMODE_FAVORITES  = 2
const GRPMODE_USER       = 3

let riGroup = Watched("")
let riGroups = []

let function riGroupGetID(name) {
  foreach (idx, nam in riGroups)
    if (nam == name)
      return idx
  return -1
}

let riGroupsChanged = Watched(null)

let riGroupsData = [
  {
    mode = GRPMODE_ALL
    name = "All RendInsts"
    count = 0
  }
]

let function riAddPredefinedGroup(name, tags) {
  riGroupsData.append({
    mode = GRPMODE_PREDEFINED
    name = name
    count = 0
    list = []
    tags = tags
  })
}

let function riAddFavoritesGroup() {
  riGroupsData.append({
    mode = GRPMODE_FAVORITES
    name = "<< Favorites >>"
    count = 0
    list = []
  })
}

let function riCalcNameInGroup(name, group) {
  let isFavorites = group.mode == GRPMODE_FAVORITES
  if (!isFavorites && group.mode != GRPMODE_USER)
    return -1
  foreach (item in group.list)
    if (item == name)
      return isFavorites ? 1 : 2
  return 0
}

let function riCalcNameInGroups(name) {
  local result = 0
  foreach (group in riGroupsData)
    result = math.max(result, riCalcNameInGroup(name, group))
  return result
}

let function riBuildNamesGroups() {
  riNamesGroups.clear()
  foreach (name in riNames) {
    let result = riCalcNameInGroups(name)
    if (result > 0)
      riNamesGroups[name] <- result
  }
}

let function riIsUserGroup(name) {
  let groupID = riGroupGetID(name)
  if (groupID == null || groupID < 0 || groupID >= riGroupsData.len())
    return false
  return riGroupsData[groupID].mode == GRPMODE_USER
}

let function riIsEmptyGroup(name) {
  let groupID = riGroupGetID(name)
  if (groupID == null || groupID < 0 || groupID >= riGroupsData.len())
    return false
  let list = riGroupsData[groupID]?.list
  return list != null && list.len() == 0
}

let function riHasUserGroups() {
  foreach (group in riGroupsData)
    if (group.mode == GRPMODE_USER)
      return true
  return false
}

const EDITOR_RI_GROUPS_CONFIG_FILE_PATHNAME = "editor_ri_groups.blk"

let function riSaveUserGroups() {
  local groupsBlk = DataBlock()

  foreach(group in riGroupsData) {
    if (group.mode == GRPMODE_FAVORITES) {
      let groupBlk = groupsBlk.addNewBlock("favorites")
      foreach(item in group.list)
        groupBlk.addStr("ri", item)
      continue
    }
    if (group.mode != GRPMODE_USER)
      continue
    let groupBlk = groupsBlk.addNewBlock("group")
    groupBlk.setStr("_name", group.name)
    foreach(item in group.list)
      groupBlk.addStr("ri", item)
  }

  groupsBlk.saveToTextFile(EDITOR_RI_GROUPS_CONFIG_FILE_PATHNAME)
}

let function riSortUserGroups() {
  if (!riHasUserGroups())
    return

  foreach (idx, group in riGroupsData)
    if (group.mode != GRPMODE_USER)
      group.index <- idx

  riGroupsData.sort(function(a,b) {
    if (a?.index != null) {
      if (b?.index != null)
        return a.index < b.index ? -1 : 1
      return -1
    }
    if (b?.index != null)
      return 1
    return a.name <=> b.name
  })
}

let function riHasTag(name, tag) {
  let tagLen = tag.len()
  let nameLen = name.len()
  let lastPos = nameLen - tagLen
  for (local j = 0; j < nameLen; ) {
    let at = name.indexof(tag, j)
    if (at == null)
      break
    if ((at == 0 || name[at-1] == '_') && (at == lastPos || name[at+tagLen] == '_'))
      return true
    j = at + 1
  }
  return false
}

let function riGroupListName(name, count) {
  return $"{name} ({count})"
}

let function riRebuildGroupsList() {
  riGroups.clear()
  foreach (group in riGroupsData)
    riGroups.append(riGroupListName(group.name, group.count))
}


let function riIsDigit(ch) {
  return ch != null && ch >= '0' && ch <= '9'
}

let function riBuildTags() {
  riTags.clear()

  local allWords = {}
  for (local i = 0; i < riNames.len(); i++) {
    let name = riNames[i]
    let words = name.split("_")
    foreach (itWord in words) {
      local word = clone itWord

      if (!(word == "1story" || word == "2story" || word == "3story")) {
        while (word.len() > 1 && riIsDigit(word[word.len()-1]))
          word = word.slice(0, word.len()-1)
        if (word.len() <= 2 || riIsDigit(word[0]) || riIsDigit(word[1]) || riIsDigit(word[2]))
          continue
      }

      if (allWords?[word] != null)
        allWords[word] += 1
      else
        allWords[word] <- 1
    }
  }

  foreach (word, count in allWords)
    riTags.append({word = word, count = count})
  riTags.sort(@(a,b) a.word <=> b.word)

  riTagsOffset.trigger()
}

let function riLoadUserGroups() {
  local groupsBlk = DataBlock()
  try
    groupsBlk.load(EDITOR_RI_GROUPS_CONFIG_FILE_PATHNAME)
  catch(e)
    groupsBlk = null
  if (groupsBlk == null)
    return

  local favIdx = -1
  foreach (idx, group in riGroupsData)
    if (group.mode == GRPMODE_FAVORITES)
      favIdx = idx

  for (local i = 0; i < groupsBlk.blockCount(); i++) {
    let groupBlk = groupsBlk.getBlock(i)
    let blockName = groupBlk.getBlockName()
    if (blockName != "group" && blockName != "favorites")
      continue
    let isFavorites = (blockName == "favorites")
    let groupName = groupBlk.getStr("_name", "")
    if (groupName == "" && !isFavorites)
      continue

    let loadGroup = {
      mode = GRPMODE_USER
      name = groupName
      count = 0
      list = []
    }
    for (local j = 0; j < groupBlk.paramCount(); j++) {
      let paramName = groupBlk.getParamName(j)
      let paramType = groupBlk.getParamTypeAnnotation(j)
      if (paramName != "ri" || paramType != "t")
        continue

      let riName = groupBlk.getParamValue(j)
      if (riName != "") {
        if (isFavorites) {
          riGroupsData[favIdx].count += 1
          riGroupsData[favIdx].list.append(riName)
        }
        else {
          loadGroup.count += 1
          loadGroup.list.append(riName)
        }
      }
    }
    if (!isFavorites)
      riGroupsData.append(loadGroup)
  }

  riBuildNamesGroups()
  riSortUserGroups()
  riSaveUserGroups()
}

local riFillPGDone = false
local riFillPGCnt = 0
local riFillPGGrp = 0
local riFillPGTag = 0
local riFillPGHash = {}
let function riFillPredefinedGroups() {
  local done = false
  let maxIters = (riFillPGCnt < 1) ? 1 : 100
  for (local i = 0; i < maxIters; i++) {
    if (riFillPGGrp >= riGroupsData.len()) {
      done = true
      break
    }
    let group = riGroupsData[riFillPGGrp]
    if (group.mode != GRPMODE_PREDEFINED) {
      ++riFillPGGrp
      continue
    }
    if (riFillPGTag == 0) {
      group.count = 0
      group.list = []
      riFillPGHash = {}
    }
    if (riFillPGTag >= group.tags.len()) {
      group.list.sort()
      ++riFillPGGrp
      riFillPGTag = 0
      continue
    }
    let tagged = group.tags[riFillPGTag]
    let tags = tagged.split("-")
    foreach(ri in riNames) {
      local found = 0
      for (local j = 0; j < tags.len(); j++)
        if (riHasTag(ri, tags[j]))
          found += (j == 0) ? 1 : -1
      if (found > 0 && riFillPGHash?[ri] == null) {
        riFillPGHash[ri] <- true
        group.count += 1
        group.list.append(ri)
      }
    }
    ++riFillPGTag
    ++riFillPGCnt
  }

  if (!done)
    gui_scene.resetTimeout(0.2, riFillPredefinedGroups)
  else {
    riFillPGDone = true
    riRebuildGroupsList()
    riGroup.trigger()
  }
}

let function riInits() {
  if (riNames.len() > 0)
    return
  if (riFile.value == null)
    return

  local blk = DataBlock()
  blk.load(riFile.value)
  let cnt = blk.paramCount()
  riNames.resize(cnt)
  for (local i = 0; i < cnt; i++)
    riNames[i] = blk.getParamName(i)

  riBuildTags()
  riLoadUserGroups()

  riGroupsData[0].count = riNames.len()
  riGroup.trigger()

  gui_scene.resetTimeout(0.2, riFillPredefinedGroups)
}


let riEditGroupName = Watched("")
let riEditGroupNameMode = Watched(0)
let riEditGroupNameElem = nameFilter(riEditGroupName, {
  placeholder = "Group name"
  function onChange(text) {
    riEditGroupName(text)
  }
  function onEscape() {
    set_kb_focus(null)
  }
  function onReturn() {
    set_kb_focus(null)
  }
})

let function riGroupRename() {
  let groupID = riGroupGetID(riGroup.value)
  if (groupID == null || groupID < 0 || groupID >= riGroupsData.len())
    return
  if (riGroupsData[groupID].mode != GRPMODE_USER)
    return
  riEditGroupName(riGroupsData[groupID].name)
  riEditGroupNameMode(1)
}

let function riGroupNew() {
  riEditGroupName("")
  riEditGroupNameMode(2)
}

let function riGroupRenameOrNewCancel() {
  riEditGroupNameMode(0)
}

let validRIGroupNameRegExp = regexp(@"[a-z,A-Z,0-9,!,@,#,$,%,^,&,(,),_,+,-,:,',., ]*")
let function riIsValidGroupName(name) {
  if (name == null)
    return false
  return validRIGroupNameRegExp.match(name)
}

let function riGroupNameUsed(name) {
  foreach(group in riGroupsData)
    if (group.mode == GRPMODE_USER && group.name == name)
      return true
  return false
}

let function riDeleteGroup(name) {
  local groupIdx = -1
  foreach(idx, group in riGroupsData)
    if (group.mode == GRPMODE_USER && group.name == name)
      groupIdx = idx
  if (groupIdx < 0)
    return

  riGroupsData.remove(groupIdx)

  riSaveUserGroups()
  riRebuildGroupsList()
  riBuildNamesGroups()

  riGroup("")
  riPage(0)
  riGroupsChanged.trigger()
}

let function riGroupRenameFinish() {
  riEditGroupNameMode(0)

  let groupID = riGroupGetID(riGroup.value)
  if (groupID == null || groupID < 0 || groupID >= riGroupsData.len())
    return
  if (riGroupsData[groupID].mode != GRPMODE_USER)
    return

  let oldName = riGroupsData[groupID].name
  let newName = riEditGroupName.value

  if (newName == "") {
    showMsgbox({text = $"Do you really want to delete group '{oldName}'?", buttons = [
      { text = "Yes, delete this group", action = @() riDeleteGroup(oldName) }
      { text = "No, keep this group",  action = @() null }
    ]})
    return
  }
  if (newName == oldName)
    return
  if (!riIsValidGroupName(newName)) {
    riEditGroupNameMode(1)
    showMsgbox({text = $"Cannot rename to bad name '{newName}'"})
    return
  }
  if (riGroupNameUsed(newName)) {
    riEditGroupNameMode(1)
    showMsgbox({text = $"Group name '{newName}' already used"})
    return
  }

  let wasCount = riGroupsData[groupID].count
  riGroupsData[groupID].name = newName

  riSortUserGroups()
  riSaveUserGroups()
  riRebuildGroupsList()

  riGroup(riGroupListName(newName, wasCount))
  riGroupsChanged.trigger()
}

let function riGroupNewFinish() {
  riEditGroupNameMode(0)
  let newName = riEditGroupName.value
  if (!riIsValidGroupName(newName)) {
    riEditGroupNameMode(2)
    showMsgbox({text = $"Cannot create new group with bad name: {newName}"})
    return
  }
  if (riGroupNameUsed(newName)) {
    riEditGroupNameMode(2)
    showMsgbox({text = $"Group name '{newName}' already used"})
    return
  }

  riGroupsData.append({
    mode = GRPMODE_USER
    name = newName
    count = 0
    list = []
  })

  riSortUserGroups()
  riSaveUserGroups()
  riRebuildGroupsList()

  riGroup(riGroupListName(newName, 0))
  riGroupsChanged.trigger()
}


let function riAddWithExludes(out_filtered, name, excludes) {
  foreach (idx, exclude in excludes)
    if (idx > 0 && exclude != "" && name.contains(exclude))
      return
  out_filtered.append(name)
}

let function riFilterNames(out_filtered, names, by_filter) {
  let excludes = by_filter.split("-")
  let filter = excludes[0]

  let total = names.len()
  out_filtered.resize(total)
  out_filtered.resize(0)

  if (filter.len() <= 0) {
    for (local i = 0; i < total; i++)
      riAddWithExludes(out_filtered, names[i], excludes)
  }
  else if (filter[0] != '#') {
    for (local i = 0; i < total; i++) {
      let name = names[i]
      if (name.contains(filter))
        riAddWithExludes(out_filtered, name, excludes)
    }
  }
  else {
    let filter2 = filter.slice(1,filter.len())
    if (filter2.len() <= 0) {
      for (local i = 0; i < total; i++)
        riAddWithExludes(out_filtered, names[i], excludes)
    }
    else {
      for (local i = 0; i < total; i++) {
        let name = names[i]
        if (riHasTag(name, filter2))
          riAddWithExludes(out_filtered, name, excludes)
      }
    }
  }
}

let riFilter = Watched("")
let riFiltered = Computed(function() {
  let filtered = []
  let groupID = riGroupGetID(riGroup.value)
  if (groupID < 0 || riGroupsData[groupID].mode == GRPMODE_ALL)
    riFilterNames(filtered, riNames, riFilter.value)
  else
    riFilterNames(filtered, riGroupsData[groupID].list, riFilter.value)
  return filtered
})

let riPages = Computed(function() {
  let filtered = riFiltered.value
  local pages = math.floor(filtered.len() / riPageCount)
  if (filtered.len() > pages * riPageCount)
    ++pages
  if (pages < 1)
    pages = 1
  return pages
})

let riPageClamped = Computed(function() {
  local page = riPage.value
  if (page < 0)
    page = 0
  if (page >= riPages.value)
    page = riPages.value-1
  return page
})

let riDisplayed = Computed(function() {
  let filtered = riFiltered.value
  local start = riPageCount * riPageClamped.value
  local count = riPageCount
  if (start < 0)
    start = 0
  if (start+count > filtered.len())
    count = filtered.len() - start
  if (count < 0)
    count = 0
  let displayed = []
  displayed.resize(count)
  for (local i = 0; i < count; i++)
    displayed[i] = filtered[start + i]
  return displayed
})

let function riGotoPage(page) {
  set_kb_focus(null)
  if (page < 0)
    page = 0
  if (page >= riPages.value)
    page = riPages.value-1
  riPage(page)
}

let function riGotoPageByValue(v) {
  let filtered = riFiltered.value
  let fcount = filtered.len()
  for (local i = 0; i < fcount; i++) {
    if (filtered[i] == v) {
      local page = math.floor(i / riPageCount)
      if (page < 0)
        page = 0
      if (page >= riPages.value)
        page = riPages.value-1
      riPage(page)
      return
    }
  }
  riPage(0)
}

let riNameFilter = nameFilter(riFilter, {
  placeholder = "Filter by name"
  function onChange(text) {
    riFilter(text)
    riGotoPageByValue(riSelectValue.value)
  }
  function onEscape() {
    set_kb_focus(null)
  }
  function onReturn() {
    set_kb_focus(null)
  }
  function onClear() {
    riFilter.update("")
    set_kb_focus(null)
  }
})


let function riSelectChange(v) {
  set_kb_focus(null)
  if (riSelectValue.value != v) {
    riSelectValue(v)
    riSelectCB?(v)
  }
}
let function riSelectChangeAndClose(v) {
  riTagsShown(false)
  riSelectChange(v)
  riSelectShown(false)
}


let riTagsDisplayed = Computed(function() {
  let filtered = riTags
  local start = riTagsOffset.value
  local count = riTagsPageCount
  if (start < 0)
    start = 0
  if (start+count > filtered.len())
    count = filtered.len() - start
  if (count < 0)
    count = 0
  let displayed = []
  displayed.resize(count)
  for (local i = 0; i < count; i++)
    displayed[i] = filtered[start + i]
  return displayed
})

let function riTagScroll(offs) {
  let last = riTags.len() - riTagsPageCount
  if (offs > last)
    offs = last
  if (offs < 0)
    offs = 0
  riTagsOffset(offs)
}

let function riTagApply(tag) {
  riGroup("")
  riGotoPage(0)
  riSelectTag(tag)
  riTagsShown(false)
  riFilter($"#{tag}")
}

riFilter.subscribe(function(v) {
  if (!riTagsShown.value)
    return
  foreach (idx, tag in riTags) {
    if (startswith(tag.word, v)) {
      riTagScroll(idx)
      break
    }
  }
})


let function riNavFirst() {
  if (!riTagsShown.value)
    riGotoPage(0)
  else
    riTagScroll(0)
}

let function riNavPrev() {
  if (!riTagsShown.value)
    riGotoPage(riPageClamped.value-1)
  else
    riTagScroll(riTagsOffset.value - riTagsPageCount)
}

let function riNavNext() {
  if (!riTagsShown.value)
    riGotoPage(riPageClamped.value+1)
  else
    riTagScroll(riTagsOffset.value + riTagsPageCount)
}

let function riNavLast() {
  if (!riTagsShown.value)
    riGotoPage(riPages.value-1)
  else
    riTagScroll(riTags.len())
}

let function riNavBy() {
  if (riTagsShown.value)
    return "Tags"
  return "Page"
}

let function riNavAt() {
  if (riTagsShown.value)
    return (riTagsOffset.value + riTagsPageCount)
  return 1+riPageClamped.value
}

let function riNavOf() {
  if (riTagsShown.value)
    return riTags.len()
  return riPages.value
}

riGroup.subscribe(function(_v) {
  riTagsShown(false)
})


let function riGetFavGroupID() {
  foreach (idx, group in riGroupsData)
    if (group.mode == GRPMODE_FAVORITES)
      return idx
  return -1
}

let function riGetUserGroupID(group_name) {
  foreach (idx, group in riGroupsData)
    if (group.mode == GRPMODE_USER && group.name == group_name)
      return idx
  return -1
}

let function riAddToGroup(name, group_idx) {
  if (group_idx < 0)
    return
  let group = riGroupsData[group_idx]
  if (group.list == null)
    return
  let groupSeen = riGroupListName(group.name, group.count) == riGroup.value
  foreach (item in group.list)
    if (item == name)
      return
  local done = false
  foreach (idx, item in group.list) {
    if (item > name) {
      group.list.insert(idx, name)
      done = true
      break
    }
  }
  if (!done)
    group.list.append(name)
  group.count += 1
  let isFavorites = group.mode == GRPMODE_FAVORITES
  riNamesGroups[name] <- math.max(isFavorites ? 1 : 2, riNamesGroups?[name] ?? 0)
  riRebuildGroupsList()
  if (groupSeen)
    riGroup(riGroupListName(group.name, group.count))
}

let function riDelFromGroup(name, group_idx) {
  if (group_idx < 0)
    return
  let group = riGroupsData[group_idx]
  if (group.list == null)
    return
  let groupSeen = riGroupListName(group.name, group.count) == riGroup.value
  local nameIdx = -1
  foreach (idx, item in group.list)
    if (item == name)
      nameIdx = idx
  if (nameIdx < 0)
    return
  group.list.remove(nameIdx)
  group.count -= 1
  let result = riCalcNameInGroups(name)
  riNamesGroups[name] <- result > 0 ? result : null
  riRebuildGroupsList()
  if (groupSeen)
    riGroup(riGroupListName(group.name, group.count))
}


let riEditGroupsData = Watched({
  name = null
  groups = []
})

let function riOpenEditGroups(name, already) {
  if (!already) {
    riAddToGroup(name, riGetFavGroupID())
    riGroupsChanged.trigger()
    return
  }
  riEditGroupsData.value.name = name
  riEditGroupsData.value.groups.clear()
  foreach (group in riGroupsData) {
    let result = riCalcNameInGroup(name, group)
    if (result >= 0) {
      riEditGroupsData.value.groups.append({
        name = group.name
        fave = group.mode == GRPMODE_FAVORITES
        now = result > 0
        was = result > 0
      })
    }
  }
  riEditGroupsData.trigger()
}

let function riEditGroupsToggle(name) {
  foreach (group in riEditGroupsData.value.groups)
    if (group.name == name)
      group.now = !group.now
  riEditGroupsData.trigger()
}

let function riCloseEditGroups() {
  riEditGroupsData.value.name = null
  riEditGroupsData.value.groups.clear()
  riEditGroupsData.trigger()
}

let function riApplyEditGroups() {
  let name = riEditGroupsData.value.name
  foreach (group in riEditGroupsData.value.groups) {
    let groupIdx = group.fave ? riGetFavGroupID() : riGetUserGroupID(group.name)
    if (group.now && !group.was)
      riAddToGroup(name, groupIdx)
    if (!group.now && group.was)
      riDelFromGroup(name, groupIdx)
  }
  riGroupsChanged.trigger()
  riCloseEditGroups()
}

riGroupsChanged.subscribe(function(_v) {
  riCloseEditGroups()
  riSaveUserGroups()
  riGroup.trigger()
})


let function mouseWheelCb(mouseEvent) {
  let ctrl = mouseEvent?.ctrlKey ?? false
  let step = -(mouseEvent?.button ?? 0) * (ctrl ? 10 : 1)
  if (!riTagsShown.value)
    riGotoPage(riPageClamped.value + step)
  else
    riTagScroll(riTagsOffset.value + step * riTagsPageCount)
}

let riTagTextCtor = @(v) $"{v.word} ({v.count})"

let function mkTag(opt, i) {
  let tagWord = riTags?[riTagsOffset.value + i].word ?? "<invalid>"
  let isSelected = Computed(@() riSelectTag.value == tagWord)
  let onClick = @() riTagApply(tagWord)
  return watchElemState(@(sf) {
    size = [flex(), SIZE_TO_CONTENT]
    padding = [hdpx(3), hdpx(10)]
    behavior = [Behaviors.Button, Behaviors.TrackMouse]
    onMouseWheel = mouseWheelCb
    eventPassThrough = false
    watch = isSelected
    onClick
    children = txt(riTagTextCtor(opt), {color = isSelected.value ? null : Color(190,190,190)})
    rendObj = ROBJ_BOX
    fillColor = sf & S_TOP_HOVER  ? Color(120,120,160) : (i%2) ? Color(0,0,0,120) : 0
    borderWidth = isSelected.value ? hdpx(2) : 0
  })
}

let mkSelectLine = kwarg(function(selected, textCtor = null, onSelect=null, onDClick=null){
  textCtor = textCtor ?? @(opt) opt
  return function(opt, i){
    let isSelected = Computed(@() selected.value == opt)
    let onClick = onSelect != null ? @() onSelect?(opt) : @() (!isSelected.value ? selected(opt) : selected(null))
    let onDoubleClick = onDClick != null ? @() onDClick?(opt) : null
    let group = riNamesGroups?[opt]
    let opTxt = group ? "  §  " : "  +  "
    let grp = ElemGroup()
    return watchElemState(@(sf) {
      size = [flex(), SIZE_TO_CONTENT]
      padding = [hdpx(3), hdpx(10)]
      behavior = [Behaviors.Button, Behaviors.TrackMouse, Behaviors.DragAndDrop]
      onMouseWheel = mouseWheelCb
      eventPassThrough = false
      watch = isSelected
      onClick
      onDoubleClick
      group = grp
      children = [
        txt(textCtor(opt), {color = isSelected.value ? null : Color(190,190,190)})
        isSelected.value || group || (sf & S_TOP_HOVER) ?
        {
          color = group || (sf & S_TOP_HOVER) ? Color(240,240,240) : Color(150,150,150)
          group = grp
          rendObj = ROBJ_TEXT
          text = opTxt
          behavior = Behaviors.Button
          eventPassThrough = false
          pos = [hdpx(10), 0]
          onClick = @() riOpenEditGroups(opt, group != null)
          onDoubleClick = function() {
            let groupID = riGroupGetID(riGroup.value)
            if (groupID != null && groupID >= 0 && groupID < riGroupsData.len()) {
              let mode = riGroupsData[groupID].mode
              if (mode == GRPMODE_USER || mode == GRPMODE_FAVORITES) {
                riDelFromGroup(opt, riGroupGetID(riGroup.value))
              }
              else {
                riDelFromGroup(opt, riGetFavGroupID())
              }
            }
            else {
              riDelFromGroup(opt, riGetFavGroupID())
            }
            riGroupsChanged.trigger()
            riCloseEditGroups()
          }
          size = SIZE_TO_CONTENT
          hplace = ALIGN_RIGHT
        } : null
      ]
      rendObj = ROBJ_BOX
      fillColor = sf & S_TOP_HOVER ? Color(120,120,160) : (i%2) ? Color(0,0,0,120) : 0
      borderWidth = isSelected.value ? hdpx(2) : 0
    })
  }
})


let buttonStyle    = {boxStyle  = {normal = {fillColor = Color(0,0,0,120)}}}
let buttonStyleOn  = {textStyle = {normal = {color = Color(0,0,0,255)}},
                      boxStyle  = {normal = {fillColor = Color(255,255,255)}, hover = {fillColor = Color(245,250,255)}}}
let buttonStyleOff = {textStyle = {normal = {color = Color(120,120,120,255)}, hover = {color = Color(120,120,120,255)}}, off = true,
                      boxStyle  = {normal = {fillColor = Color(0,0,0,120)}, hover = {fillColor = Color(0,0,0,120)}}}

let function mkEditGroup(group) {
  return {
    children = [
      { size = [0, sh(0.2)] }
      textButton(group.name, @() riEditGroupsToggle(group.name), group.now ? buttonStyleOn : buttonStyleOff)
    ]
  }
}

let function riGroupUpdate(v) {
  riGroup(v)
  riGotoPageByValue(riSelectValue.value)
}
let riGroupCombo = combobox({value=riGroup, changeVarOnListUpdate=false, update=riGroupUpdate}, riGroups)

let riSelectWindow = function() {
  let mkRI = mkSelectLine({
    selected = riSelectValue
    onSelect = @(v) riSelectChange(v)
    onDClick = @(v) riSelectChangeAndClose(v)
  })
  return {
    hplace = ALIGN_CENTER
    vplace = ALIGN_CENTER
    children = [
      @() {
        watch = [riDisplayed, riFilter, riGroup, riGroupsChanged, riEditGroupName, riEditGroupNameMode, riTagsShown, riTagsDisplayed]
        behavior = [Behaviors.Button, Behaviors.TrackMouse]
        onMouseWheel = mouseWheelCb
        eventPassThrough = false
        pos = [0, fsh(1)]
        size = [sw(29), sh(77)]
        hplace = ALIGN_CENTER
        vplace = ALIGN_CENTER
        rendObj = ROBJ_SOLID
        color = Color(30,30,30, 190)
        padding = hdpx(10)
        children = vflow(
          Flex()
          Gap(hdpx(10))
          @() {
            watch = [riGroup, riEditGroupName, riEditGroupNameMode]
            flow = FLOW_HORIZONTAL
            size = [flex(), SIZE_TO_CONTENT]
            padding = 0
            children = [
              txt(" SELECT RENDINST", {hplace = ALIGN_LEFT, vplace = ALIGN_BOTTOM})
              {
                size = [flex(), sh(2.7)]
              }
              {
                size = [sw(9), sh(2.7)]
                rendObj = ROBJ_SOLID
                color = Color(30,30,30, 150)
                children = !riFillPGDone ? txt("Grouping...", {hplace = ALIGN_CENTER, vplace = ALIGN_CENTER}) : (riEditGroupNameMode.value == 0) ? riGroupCombo : riEditGroupNameElem
              }
              riEditGroupNameMode.value == 0 ? textButton(riIsUserGroup(riGroup.value) ? "Edit" : "New", @() riIsUserGroup(riGroup.value) ? riGroupRename() : riGroupNew(), {vplace = ALIGN_BOTTOM}) : null
              riEditGroupNameMode.value >  0 ? textButton(riEditGroupNameMode.value == 1 ? (riEditGroupName.value.len() ? "Rename" : "Delete!") : "Create", @() riEditGroupNameMode.value == 1 ? riGroupRenameFinish() : riGroupNewFinish(), {vplace = ALIGN_BOTTOM}) : null
              riEditGroupNameMode.value >  0 ? textButton("Back", @() riGroupRenameOrNewCancel(), {vplace = ALIGN_BOTTOM}) : null
            ]
          }
          @() {
            watch = [riTagsShown]
            flow = FLOW_HORIZONTAL
            size = [flex(), SIZE_TO_CONTENT]
            padding = 0
            children = [
              riNameFilter
              textButton("Tags", @() riTagsShown(!riTagsShown.value), {vplace = ALIGN_BOTTOM}.__merge(riTagsShown.value ? buttonStyleOn : buttonStyle))
            ]
          }
          riTagsShown.value ? vflow(Size(flex(), flex()), riTagsDisplayed.value.map(mkTag)) : null
          !riTagsShown.value && riDisplayed.value.len() > 0 ? vflow(Size(flex(), flex()), riDisplayed.value.map(mkRI)) : null
          !riTagsShown.value && riDisplayed.value.len() == 0 ? vflow(
            Size(flex(), flex()),
            { size = [0, sh(25)] },
            riIsEmptyGroup(riGroup.value) ? txt("No render instances in this group", {hplace = ALIGN_CENTER}) : null,
            { size = [0, sh(2)] },
            riIsEmptyGroup(riGroup.value) ? txt("Use + and § at right side of selected RI in other groups", {hplace = ALIGN_CENTER}) : null,
          ) : null
          hflow(
            HCenter,
            textButton("First", @() riNavFirst(), {vplace = ALIGN_BOTTOM}),
            textButton("Prev",  @() riNavPrev(),  {hotkeys = [["Left"]], vplace = ALIGN_BOTTOM}),
            txt($"{riNavBy()} {riNavAt()} / {riNavOf()}", {vplace = ALIGN_CENTER}),
            textButton("Next",  @() riNavNext(),  {hotkeys = [["Right"]], vplace = ALIGN_BOTTOM}),
            textButton("Last",  @() riNavLast(), {vplace = ALIGN_BOTTOM})
          )
          hflow(
            HCenter,
            textButton("Cancel", @() riSelectChangeAndClose(riSelectSaved.value), {vplace = ALIGN_BOTTOM}),
            textButton("Accept", @() riSelectShown(false), {hotkeys = [["Esc"]], vplace = ALIGN_BOTTOM})
          )
        )
      }
      @() {
        watch = [riEditGroupsData]
        hplace = ALIGN_CENTER
        vplace = ALIGN_CENTER
        children = riEditGroupsData.value.name ? {
          behavior = Behaviors.Button
          eventPassThrough = false
          pos = [sw(0.4), sh(-5)]
          size = SIZE_TO_CONTENT
          hplace = ALIGN_CENTER
          vplace = ALIGN_CENTER
          rendObj = ROBJ_SOLID
          color = Color(35,35,35,220)
          padding = hdpx(10)
          flow = FLOW_VERTICAL
          children = [
            txt(riEditGroupsData.value.name)
            { size = [0, sh(0.8)] }
            vflow(Size(flex(), SIZE_TO_CONTENT), riEditGroupsData.value.groups.map(mkEditGroup))
            { size = [0, sh(1)] }
            {
              hplace = ALIGN_CENTER
              flow = FLOW_HORIZONTAL
              children = [
                textButton("Cancel", @() riCloseEditGroups(), {hotkeys = [["Esc"]]})
                textButton("Apply", @() riApplyEditGroups())
              ]
            }
          ]
        } : null
      }
    ]
  }
}

let function openSelectRI(selectedRI, onSelect=null) {
  riInits()
  riSelectCB = onSelect
  riSelectSaved(selectedRI.value)
  riSelectValue(selectedRI.value)
  riSelectShown(true)
  riGotoPageByValue(riSelectValue.value)
}

let riSelectEid = Watched(INVALID_ENTITY_ID)
propPanelVisible.subscribe(function(v) {
  if (v && selectedEntity.value != riSelectEid.value && riSelectShown.value)
    riSelectShown(false)
})

let function onSelectRI(v){
  let eid = riSelectEid.value
  if ((eid ?? INVALID_ENTITY_ID) != INVALID_ENTITY_ID){
    obsolete_dbg_set_comp_val(eid, "ri_extra__name", v)
    entity_editor?.save_component(eid, "ri_extra__name")
    let newEid = entity_editor?.get_instance().reCreateEditorEntity(eid)
    if (newEid != INVALID_ENTITY_ID) {
      riSelectEid(newEid)
      editorUnpause(2.5)
    }
  }
}

let function initRISelect(file, groups) {
  riFile(file)
  foreach (group in groups)
    riAddPredefinedGroup(group.name, group.tags)
  riAddFavoritesGroup()
  registerPerCompPropEdit("ri_extra__name", function(params) {
    let selectedRI = Watched(params?.obj)
    riSelectEid(params.eid)
    return @() {
      watch = selectedRI
      halign = ALIGN_LEFT
      children = textButton(selectedRI.value, @() openSelectRI(selectedRI, onSelectRI))
    }
  })
}

let function openRISelectForEntity(eid) {
  riInits()
  let riName = obsolete_dbg_get_comp_val(eid, "ri_extra__name")
  if (riName == null)
    return
  riSelectEid(eid)
  riSelectCB = onSelectRI
  riSelectSaved(riName)
  riSelectValue(riName)
  riSelectShown(true)
  riGotoPageByValue(riSelectValue.value)
}

riSelectShown.subscribe(function(v) {
  if (!v) {
    riTagsShown(false)
    riCloseEditGroups()
  }
})

return {
  initRISelect
  riSelectShown
  riSelectWindow
  openRISelectForEntity
}
