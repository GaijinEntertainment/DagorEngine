let {Point2} = require("dagor.math")
let {get_time_msec} = require("dagor.time")

let test_runtime = require("test_runtime")

let {Mouse, Keys, Modifier} = test_runtime
local tr = test_runtime.getInstance()
println($"Success! Test runtime found: {tr}")

tr.registerTest("quirrel_test", function(_runtime) {
  println("Success!")
})

tr.registerTest("quirrel_yield", async function(runtime) {
  println("Yield #1")
  await runtime.yieldTime(1.0)
  println("Yield #2")
  let start = get_time_msec()
  await runtime.yieldTime(2.0)
  let elapsed = get_time_msec() - start
  if (elapsed < 2000) {
    runtime.markFailed($"yieldTime(2.0) resumed after {elapsed} ms")
    return
  }
  println("Yield #3")
  await runtime.yieldFrame()
  println("Success, Yield!")
})

// Test case that intentionally fails.
// tr.registerTest("quirrel_fail", function(runtime) {
//   runtime.markFailed("intentional failure")
//   println("Success, Test FAILED!")
// })

function printItemInfo(msg, item) {
  local max = item.bb.Max
  local min = item.bb.Min
  local labelStr = item.label != null ? $", label = {item.label}" : ", label = <null>"
  local displayNameStr = item.displayName != null ? $", displayName = {item.displayName}" : ", displayName = <null>"
  local controlTypeStr = item.controlType != null ? $", controlType = {item.controlType}" : ", controlType = <null>"
  local bbStr = $"[Min: ({min.x}, {min.y}), Max: ({max.x}, {max.y})]"
  println("".concat($"{msg}: id = {item.id}, bb = {bbStr}, parentId = {item.parentId}",
    labelStr, displayNameStr, controlTypeStr, $", controlId = {item.controlId}",
    $", hovered = {item.hovered}, active = {item.active}, visible = {item.visible}, toggledOpen = {item.toggledOpen}",
    $", isWindow = {item.isWindow}"))
}

tr.registerTest("quirrel_query_item", function(runtime) {
  local item = null
  item = runtime.queryItem("**/Assets Tree/Collapse all")
  if (item != null) {
    printItemInfo("Query item", item);
  } else {
    runtime.markFailed("queryItem() did not find an existing item");
  }
  // A root item has no level above it, so only the zero-level match of "**" finds it.
  item = runtime.queryItem("**/##MainMenuBar")
  if (item == null) {
    runtime.markFailed("queryItem() did not find a root item")
  }
  // Unlike "**", a single "*" needs one level.
  item = runtime.queryItem("*/##MainMenuBar")
  if (item != null) {
    runtime.markFailed("queryItem() matched '*' to zero levels")
  }
  item = runtime.queryItem("**/No such GUI Item")
  if (item != null) {
    runtime.markFailed("queryItem() found an item that does not exist");
  }
  println($"Success, Query Item!")
})

tr.registerTest("quirrel_query_children", function(runtime) {
  local children = null
  children = runtime.queryChildren("**/Assets Tree")
  if (children != null) {
    println($"Query children len = {children.len()}")
    foreach (item in children) {
      printItemInfo("Query children", item);
    }
  } else {
    runtime.markFailed("queryChildren() did not find an existing item");
  }
  children = runtime.queryChildren("**/No such GUI Item")
  if (children != null) {
    runtime.markFailed("queryChildren() found an item that does not exist");
  }
  println($"Success, Query Children!")
})

// A move or a click can reach no item when ImGui hovers another viewport, and the input calls do not report that.
// A button needs the hover to activate, thus a hovered item shows that the input arrived.
function checkHovered(runtime, path) {
  let item = runtime.queryItem(path)
  if (item == null || item.hovered != true) {
    runtime.markFailed($"{path} is not hovered after the mouse reached it: found {item != null}, hovered {item?.hovered}")
    return false
  }
  return true
}

tr.registerTest("quirrel_mouse_move", async function(runtime) {
  local positions = [Point2(100, 200), Point2(200, 200), Point2(200, 100), Point2(100, 100)];
  await runtime.input().mousePosition(positions[0]);
  await runtime.yieldTime(1.0);
  for (local i = 0; i < positions.len(); i++) {
    await runtime.input().mouseMove(positions[i], positions[(i+1)%positions.len()], 1.0);
    await runtime.yieldTime(1.0);
  }
  foreach (path in ["**/Assets Tree/Collapse all", "**/Assets Tree/Expand all", "**/Assets Tree/All"]) {
    if (!(await runtime.input().mouseMoveItem(path, 1.0)))
      return
    await runtime.yieldTime(1.0)
    if (!checkHovered(runtime, path))
      return
  }
  println("Success, Mouse Move!")
})

tr.registerTest("quirrel_mouse_click", async function(runtime) {
  foreach (path in ["**/Assets Tree/Collapse all", "**/Assets Tree/Expand all", "**/Assets Tree/All"]) {
    // Check before the click, because a click can change the layout or focus, and ImGui hovers no other item then.
    if (!(await runtime.input().mouseMoveItem(path, 1.0)))
      return
    await runtime.yieldTime(1.0)
    if (!checkHovered(runtime, path))
      return
    await runtime.input().mouseClick(Mouse.Left)
    await runtime.yieldTime(1.0)
  }
  // The move and click in one call.
  let expandPath = "**/Assets Tree/Expand all"
  if (!(await runtime.input().mouseClickItem(expandPath, Mouse.Left)))
    return
  await runtime.yieldTime(1.0)
  if (!checkHovered(runtime, expandPath))
    return
  await runtime.input().mouseClick(Mouse.Right);
  println("Success, Mouse Click (Item)!")
})

tr.registerTest("quirrel_mouse_modifier_click", async function(runtime) {
  local assets = runtime.queryItem("**/Assets Tree");
  local collapse = runtime.queryItem("**/Assets Tree/Collapse all");
  if (assets != null && collapse != null) {
    local cSize = collapse.bb.Max - collapse.bb.Min;
    local x = collapse.bb.Max.x - (cSize.y / 2);
    local top = Point2(x, collapse.bb.Max.y + cSize.y);
    local bottom = Point2(x, assets.bb.Max.y - cSize.y * 2);
    local shiftClickScroll = async function(pos) {
      await runtime.input().mousePosition(pos);
      await runtime.yieldFrame();
      await runtime.input().mouseClickModifier(Mouse.Left, Modifier.Shift);
      await runtime.yieldTime(1.0);
    }
    await runtime.yieldTime(1.0);
    await shiftClickScroll(top);
    local checkLastAsset = function(should) {
      local item = runtime.queryItem("**/cdk_color_tex_palette");
      return (should && item != null) || (!should && item == null);
    }
    local checkTop = checkLastAsset(false);
    await shiftClickScroll(bottom);
    local checkBottom = checkLastAsset(true);
    await shiftClickScroll(top);
    local checkTopAgain = checkLastAsset(false);
    if (checkTop && checkBottom && checkTopAgain) {
      println("Success, Mouse Modifier Click!");
    } else {
      runtime.markFailed($"shift click scroll: top {checkTop}, bottom {checkBottom}, top again {checkTopAgain}");
    }
  } else {
    runtime.markFailed("Assets Tree or its Collapse all button not found");
  }
})

tr.registerTest("quirrel_keys", async function(runtime) {
  await runtime.input().mouseMoveItem("**/Viewport", 1.0);
  await runtime.yieldTime(1.0);
  await runtime.input().mouseClick(Mouse.Left);
  await runtime.yieldTime(1.0);
  await runtime.input().keyPress(Keys.Z)
  await runtime.yieldTime(1.0);
  local changePerspective = async function(k, m) {
    await runtime.input().keyPressModifier(k, m)
    await runtime.yieldTime(0.1);
    await runtime.input().keyPress(Keys.Z)
    await runtime.yieldTime(1.0);
  }
  await changePerspective(Keys.F, Modifier.Shift)
  await runtime.input().keyPress(Keys.Z)
  await runtime.yieldTime(1.0);
  await changePerspective(Keys.K, Modifier.Shift)
  await changePerspective(Keys.T, Modifier.Shift)
  await changePerspective(Keys.B, Modifier.Shift)
  await changePerspective(Keys.L, Modifier.Shift)
  await changePerspective(Keys.R, Modifier.Shift)
  await changePerspective(Keys.P, Modifier.Shift)
  println("Success, Keys!")
})

tr.registerTest("quirrel_character_input", async function(runtime) {
  await runtime.input().mouseMoveItem("**/Assets Tree/##searchInput", 1.0);
  await runtime.yieldTime(1.0);
  await runtime.input().mouseClick(Mouse.Left);
  await runtime.yieldTime(1.0);
  await runtime.input().characterInput("cdk_building_doors", 0.2)
  await runtime.yieldTime(1.0);
  await runtime.input().mouseClick(Mouse.Left);
  await runtime.yieldTime(1.0);
  await runtime.input().keyPress(Keys.Home);
  await runtime.yieldTime(1.0);
  await runtime.input().keyHeldModifier(Keys.RightArrow, Modifier.Shift, 3.0);
  await runtime.yieldTime(1.0);
  await runtime.input().keyPress(Keys.LeftArrow);
  await runtime.yieldTime(1.0);
  await runtime.input().keyHeld(Keys.Delete, 5.0);
  println("Success, Character Input!")
})

tr.registerTest("quirrel_mouse_drag", async function(runtime) {
  local viewport = runtime.queryItem("**/Viewport")
  if (viewport == null) {
    runtime.markFailed("Viewport not found")
    return
  }
  local min = viewport.bb.Min
  local max = viewport.bb.Max
  local startx = (max.x - min.x) * 0.1
  local cy = min.y + (max.y - min.y) / 2.0
  local from = Point2(min.x + startx, cy)
  local to = Point2(max.x - startx, cy)
  await runtime.input().mouseDrag(Mouse.Middle, from, to, 5.0)
  println("Success, Mouse drag!")
})

// Checks that only the button under the mouse is hovered, and that both buttons are visible.
// queryChildren() must also report the hovered button.
tr.registerTest("quirrel_item_hovered", async function(runtime) {
  let collapsePath = "**/Assets Tree/Collapse all"
  let expandPath = "**/Assets Tree/Expand all"
  foreach ([underMousePath, otherPath] in [[collapsePath, expandPath], [expandPath, collapsePath]]) {
    await runtime.input().mouseMoveItem(underMousePath, 1.0)
    await runtime.yieldTime(1.0)
    let underMouse = runtime.queryItem(underMousePath)
    let other = runtime.queryItem(otherPath)
    if (underMouse == null || other == null) {
      runtime.markFailed("Collapse all or Expand all not found")
      return
    }
    if (!underMouse.hovered || other.hovered) {
      runtime.markFailed($"mouse on {underMousePath}: hovered {underMouse.hovered}, {otherPath} hovered {other.hovered}")
      return
    }
    if (!underMouse.visible || !other.visible) {
      runtime.markFailed($"{underMousePath} visible {underMouse.visible}, {otherPath} visible {other.visible}")
      return
    }
    let children = runtime.queryChildren("**/Assets Tree")
    if (children == null) {
      runtime.markFailed("queryChildren() did not find Assets Tree")
      return
    }
    let child = children.findvalue(@(c) c.id == underMouse.id)
    if (child == null || !child.hovered) {
      runtime.markFailed($"queryChildren(): {underMousePath} found {child != null}, hovered {child?.hovered}")
      return
    }
  }
  println("Success, Item Hovered!")
})

// Checks that a window is hovered while the mouse is on one of its items, and not while it is on another window.
// Also checks that the window is a window item and its button is not.
tr.registerTest("quirrel_window_hovered", async function(runtime) {
  let windowPath = "**/Assets Tree"
  let buttonPath = $"{windowPath}/Collapse all"
  await runtime.input().mouseMoveItem(buttonPath, 1.0)
  await runtime.yieldTime(1.0)
  let inside = runtime.queryItem(windowPath)
  let button = runtime.queryItem(buttonPath)
  await runtime.input().mouseMoveItem("**/Viewport", 1.0)
  await runtime.yieldTime(1.0)
  let outside = runtime.queryItem(windowPath)
  if (inside == null || outside == null || button == null) {
    runtime.markFailed("Assets Tree or its Collapse all button not found")
  } else if (!inside.isWindow || button.isWindow) {
    runtime.markFailed($"isWindow: Assets Tree {inside.isWindow}, Collapse all {button.isWindow}")
  } else if (inside.visible) {
    runtime.markFailed("Assets Tree reports visible, but a window item never does")
  } else if (!inside.hovered || outside.hovered) {
    runtime.markFailed($"Assets Tree hovered: mouse inside {inside.hovered}, mouse on Viewport {outside.hovered}")
  } else {
    println("Success, Window Hovered!")
  }
})

// Checks that a button is active only while the mouse button holds it down, and that a window is never active.
tr.registerTest("quirrel_item_active", async function(runtime) {
  let path = "**/Assets Tree/Expand all"
  await runtime.input().mouseMoveItem(path, 1.0)
  await runtime.yieldTime(1.0)
  let before = runtime.queryItem(path)
  await runtime.input().mouseButtonDown(Mouse.Left)
  await runtime.yieldTime(1.0)
  let held = runtime.queryItem(path)
  await runtime.input().mouseButtonUp(Mouse.Left)
  await runtime.yieldTime(1.0)
  let released = runtime.queryItem(path)
  if (before == null || held == null || released == null) {
    runtime.markFailed("Expand all not found")
    return
  }
  if (before.active || !held.active || released.active) {
    runtime.markFailed($"Expand all active: before {before.active}, held {held.active}, released {released.active}")
    return
  }

  // The property panel sets ImGui ActiveId to its window ID while the middle mouse button is down on it.
  let panelPath = "**/Properties"
  if (!(await runtime.input().mouseMoveItem(panelPath, 1.0)))
    return
  await runtime.input().mouseButtonDown(Mouse.Middle)
  await runtime.yieldTime(0.5)
  let heldPanel = runtime.queryItem(panelPath)
  await runtime.input().mouseButtonUp(Mouse.Middle)
  await runtime.yieldTime(0.5)
  if (heldPanel == null || !heldPanel.isWindow || heldPanel.active != false) {
    runtime.markFailed($"Properties held with the middle mouse button: found {heldPanel != null}, active {heldPanel?.active}")
    return
  }
  println("Success, Item Active!")
})

// The property panel shows the "Common parameters" group only while an asset is selected.
async function selectAsset(runtime, name) {
  let searchPath = "**/Assets Tree/##searchInput"
  await runtime.input().mouseClickItem(searchPath, Mouse.Left)
  await runtime.yieldTime(1.0)
  await runtime.input().characterInput(name, 0.2)
  await runtime.yieldTime(1.0)
  await runtime.input().mouseClickItem($"**/{name}", Mouse.Left)
  await runtime.yieldTime(1.0)
  await runtime.input().mouseClickItem(searchPath, Mouse.Left)
  await runtime.yieldTime(1.0)
  await runtime.input().keyPressModifier(Keys.A, Modifier.Ctrl)
  await runtime.yieldTime(1.0)
  await runtime.input().keyPress(Keys.Delete)
  await runtime.yieldTime(1.0)
}

// Checks that a group header reports toggledOpen only in the frame of the click that opens or closes it.
tr.registerTest("quirrel_item_toggled_open", async function(runtime) {
  let path = "**/Common parameters"
  if (runtime.queryItem(path) == null)
    await selectAsset(runtime, "cdk_building_doors")
  await runtime.input().mouseMoveItem(path, 1.0)
  await runtime.yieldTime(1.0)
  // Two clicks, so the group ends in the state it started in.
  for (local i = 0; i < 2; i++) {
    let before = runtime.queryItem(path)
    await runtime.input().mouseButtonDown(Mouse.Left)
    await runtime.yieldTime(1.0)
    // The group header toggles on the release, and the mouseButtonUp() Future resolves after that frame.
    await runtime.input().mouseButtonUp(Mouse.Left)
    let toggled = runtime.queryItem(path)
    await runtime.yieldFrame()
    let after = runtime.queryItem(path)
    if (before == null || toggled == null || after == null) {
      runtime.markFailed("Common parameters group not found")
      return
    }
    if (before.toggledOpen || !toggled.toggledOpen || after.toggledOpen) {
      runtime.markFailed("".concat($"click {i + 1}: toggledOpen before {before.toggledOpen}, ",
        $"on release {toggled.toggledOpen}, next frame {after.toggledOpen}"))
      return
    }
    await runtime.yieldTime(1.0)
  }
  println("Success, Item Toggled Open!")
})

// The menu item opens the dialog if it is closed, and closes it if it is open.
async function toggleEnvironmentSettings(runtime) {
  await runtime.input().mouseClickItem("##MainMenuBar/Settings", Mouse.Left)
  await runtime.yieldTime(1.0)
  await runtime.input().mouseClickItem("**/Environment settings...", Mouse.Left)
  await runtime.yieldTime(1.0)
}

// Checks the state of a window that only the previous frame submitted. ImGui still reports the closed window as
// HoveredWindow in the frame after it closed, but the query must not report it hovered.
tr.registerTest("quirrel_previous_frame_window", async function(runtime) {
  let dialogPath = "Environment Settings"
  let wasOpen = runtime.queryItem(dialogPath) != null
  if (!wasOpen)
    await toggleEnvironmentSettings(runtime)
  await runtime.input().mouseMoveItem($"{dialogPath}/Close", 1.0)
  await runtime.yieldTime(1.0)
  await runtime.input().mouseButtonDown(Mouse.Left)
  await runtime.yieldTime(0.2)
  await runtime.input().mouseButtonUp(Mouse.Left)

  // The button closes the dialog from a delayed callback, after an unknown number of frames. Step one frame at a time
  // to see the frame that submitted the dialog last, and the frame after it.
  local lastFrame = null
  local previousFrame = runtime.queryItem(dialogPath)
  for (local i = 0; i < 30 && previousFrame?.hovered == true; i++) {
    lastFrame = previousFrame
    await runtime.yieldFrame()
    previousFrame = runtime.queryItem(dialogPath)
  }
  let previousChildren = runtime.queryChildren(dialogPath)
  await runtime.yieldFrame()
  let closed = runtime.queryItem(dialogPath)
  if (wasOpen)
    await toggleEnvironmentSettings(runtime)

  let childWithState = previousChildren?.findvalue(@(c) c.hovered != null || c.active != null)
  if (lastFrame == null) {
    runtime.markFailed($"Environment Settings is not hovered after the click: found {previousFrame != null}, hovered {previousFrame?.hovered}")
  } else if (previousFrame == null || !previousFrame.isWindow || previousFrame.hovered != null
      || previousFrame.active != null) {
    runtime.markFailed("".concat($"Environment Settings in the next frame: found {previousFrame != null}, ",
      $"isWindow {previousFrame?.isWindow}, hovered {previousFrame?.hovered}, active {previousFrame?.active}"))
  } else if (previousChildren == null || previousChildren.len() == 0 || childWithState != null) {
    runtime.markFailed("".concat($"Environment Settings children in the next frame: count {previousChildren?.len()}, ",
      $"{childWithState?.label} hovered {childWithState?.hovered}, active {childWithState?.active}"))
  } else if (closed != null) {
    runtime.markFailed("Environment Settings is still found two frames after it closed")
  } else {
    println("Success, Previous Frame Window!")
  }
})
