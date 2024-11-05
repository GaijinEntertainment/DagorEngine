from "%darg/ui_imports.nut" import *

let defButtonStyle = {
  text = {
    normal = {
      fontSize = hdpx(25)
      color = Color(200,200,200)
    }
    hover = {
      color = Color(220,240,255)
    }
    active = {
      color = Color(255,255,255)
    }
  }
  box = {
    normal = {
      borderWidth = 0
      fillColor = Color(50,50,50)
      padding = [hdpx(2), hdpx(5)]
      margin = hdpx(2)
    }
  }
}

function textButton(text, handler= @() null, params = {}, style = defButtonStyle){
  let stateFlags = Watched(0)
  let disabled = params?.disabled
  let textStyle = style?.text ?? defButtonStyle.text
  let boxStyle = style?.box ?? defButtonStyle.box
  let textNormal = textStyle?.normal ?? defButtonStyle.text.normal
  let boxNormal = boxStyle?.normal ?? defButtonStyle.box.normal
  return function(){
    let s = stateFlags.value
    local state = "normal"
    if (disabled?.value)
      state = "disabled"
    if (s & S_HOVER)
      state = "hover"
    else if (s & S_ACTIVE)
      state = "active"

    let textS = textStyle?[state] ?? {}
    let boxS = boxStyle?[state] ?? {}
    return {
      rendObj = ROBJ_BOX
      children = {rendObj = ROBJ_TEXT text}.__update(textNormal, textS)
    }.__update(boxNormal, boxS, {
      watch = [stateFlags, disabled]
      onElemState = @(sf) stateFlags(sf)
      behavior = Behaviors.Button
      onClick = handler
    })
  }

}
/*
  description:
    classical vertical customizable menu with back button and isAvailable

  todo:
  - move functionality to common library:
      add styling:
        style of breadcrump buttons, gaps (and existence)
        style of menuButton
        style of CurMenu Title Text
        backButton text (and existence)
        mainMenu tex
        style of menu : place, size, background, margins, etc
     export :
       builtmenu(watchable menuitems, style)
       breadcrumps(style)
  - replace enlist and enlisted menus // FIXME_BROKEN_DEP

  note: breadcrumps can be made different way, with list of state to set, without UI, which can be separate thing than
    in this case update it on proceeding deeper with "what to set"

*/


let menuState = {
  breadCrumps = mkWatched(persist, "breadCrumps",[])
  showMenu = mkWatched(persist, "showMenu", true)
  curMenuItems = mkWatched(persist, "curMenuItems",[])
}

let backButton = {
  text = "Back",
  action = function() {
    let curBreadCrumps = menuState.breadCrumps.value
    if (curBreadCrumps.len() > 1) {
      let m = curBreadCrumps?[curBreadCrumps.len()-2].menu ?? curBreadCrumps[0]?.menu
      menuState.curMenuItems.update(m)
      menuState.breadCrumps.update(curBreadCrumps.slice(0,-1))
    }
  }
}
function menuhandler(menuItemId, params = {leaveAction=null, action=null, submenu=null}) {
  let submenu = params?.submenu
  let leaveAction = params?.leaveAction
  let action = params?.action
  return function(){
    if (action)
      action()
    if (submenu) {
      let nextMenu = []
      nextMenu.extend(submenu)
      nextMenu.append(backButton)
      menuState.curMenuItems.update(nextMenu)
    }
    if (submenu || leaveAction != null) {
      let curBreadCrumps = menuState.breadCrumps.value
      curBreadCrumps.append({text=menuItemId menu=menuState.curMenuItems.value})
      menuState.breadCrumps.update(curBreadCrumps)
    }
    if (leaveAction != null) {
      menuState.showMenu.update(false)
      leaveAction()
    }
  }
}

function buildmenu(_params={addBackBtn=false}) {
  let children = []
  foreach (item in menuState.curMenuItems.value) {
    if (item?.isAvailable == null || item?.isAvailable())
      children.append(textButton(
        item.text,
        menuhandler(item.text, {submenu=item?.submenu, leaveAction=item?.leaveAction action=item?.action})
      ))
  }
  return {
    watch = [menuState.curMenuItems]
    flow = FLOW_VERTICAL
    children = children
  }
}

function breadcrumps(){
  let children  = []
  let curBreadCrumps = menuState.breadCrumps.value
  for(local i=0; i<curBreadCrumps.len(); i++) { //map
    let bc = curBreadCrumps[i]
    let menu=bc.menu
    let idx = i
    let len = curBreadCrumps.len()
    local child = null
    if (i < len-1)
      child = textButton(bc.text,
        function() {
          menuState.curMenuItems.update(menu)
          menuState.breadCrumps.update(curBreadCrumps.slice(0, idx+1))
          menuState.showMenu.update(true)
        }
      )
    else
      child = { margin = sh(1) children = {rendObj=ROBJ_TEXT text=bc.text fontSize = hdpx(40) margin=[sh(1), sh(3)]} key=bc.text}
    children.append(child)
  }
  return {
    flow = FLOW_HORIZONTAL
    children = children
    watch = menuState.breadCrumps
  }
}

function startQuickMatch(){vlog("started")}
function exitgame(){}
function showOptions(){}
function showCreateMatch(){}
function showFindMatch(){}
let menuItems = [
  {text= "Quick Match", leaveAction=startQuickMatch, isAvailable=@() true}
  {text= "Custom Match", isAvailable=@() true
    submenu = [
      {text="Find Match", leaveAction=showFindMatch}
      {text="Create Match", leaveAction=showCreateMatch isAvailable=@() true }
    ]
  }
  {text= "More menu",
    submenu = [
      {text="More menus lv1" submenu =[
        {text="lv2" submenu=[{text="lv3"}]}]
        }
    ]
  }
  {text= "Options", leaveAction=showOptions}
  {text= "Exit", action=exitgame}
]

menuState.curMenuItems.update(menuItems)
menuState.breadCrumps.update([{text="Main Menu", menu = menuItems}])


return function() {
  let children = [breadcrumps]
  if (menuState.showMenu.value)
    children.append({size=flex() padding=[sh(20),sh(20)] children = buildmenu})
  return {
    halign = ALIGN_LEFT
    valign = ALIGN_TOP
    rendObj = ROBJ_SOLID
    color = Color(120,120,120)
    size = flex()
    flow = FLOW_VERTICAL
    watch = [menuState.curMenuItems, menuState.breadCrumps, menuState.showMenu]
    children = children
  }
}
