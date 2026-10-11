let {Point2} = require("dagor.math")

let test_runtime = require("test_runtime")

let {Mouse, Keys, Modifier} = test_runtime
local tr = test_runtime.getInstance()
println($"Success! Test runtime found: {tr}")

tr.registerTest("quirrel_test", function(_runtime) {
  println("Success!")
})

tr.registerTest("quirrel_yield", function(runtime) {
  println("Yield #1")
  runtime.yieldTime(1.0)
  println("Yield #2")
  runtime.yieldTime(2.0)
  println("Yield #3")
  runtime.yieldFrame()
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
  item = runtime.queryItem("Root/*/*/Assets Tree/Collapse all")
  if (item != null) {
    printItemInfo("Query item", item);
  } else {
    runtime.markFailed("queryItem() did not find an existing item");
  }
  item = runtime.queryItem("Root/*/*/No such GUI Item")
  if (item != null) {
    runtime.markFailed("queryItem() found an item that does not exist");
  }
  println($"Success, Query Item!")
})

tr.registerTest("quirrel_query_children", function(runtime) {
  local children = null
  children = runtime.queryChildren("Root/*/*/Assets Tree")
  if (children != null) {
    println($"Query children len = {children.len()}")
    foreach (item in children) {
      printItemInfo("Query children", item);
    }
  } else {
    runtime.markFailed("queryChildren() did not find an existing item");
  }
  children = runtime.queryChildren("Root/*/*/No such GUI Item")
  if (children != null) {
    runtime.markFailed("queryChildren() found an item that does not exist");
  }
  println($"Success, Query Children!")
})

tr.registerTest("quirrel_mouse_move", function(runtime) {
  local positions = [Point2(100, 200), Point2(200, 200), Point2(200, 100), Point2(100, 100)];
  runtime.input().mousePosition(positions[0]);
  runtime.yieldTime(1.0);
  for (local i = 0; i < positions.len(); i++) {
    runtime.input().mouseMove(positions[i], positions[(i+1)%positions.len()], 1.0);
    runtime.yieldTime(1.0);
  }
  runtime.input().mouseMoveItem("Root/*/*/Assets Tree/Collapse all", 1.0);
  runtime.yieldTime(1.0);
  runtime.input().mouseMoveItem("Root/*/*/Assets Tree/Expand all", 1.0);
  runtime.yieldTime(1.0);
  runtime.input().mouseMoveItem("Root/*/*/Assets Tree/All", 1.0);
  println("Success, Mouse Move!")
})

tr.registerTest("quirrel_mouse_click", function(runtime) {
  runtime.input().mouseClickItem("Root/*/*/Assets Tree/Collapse all", Mouse.Left);
  runtime.yieldTime(1.0);
  runtime.input().mouseClickItem("Root/*/*/Assets Tree/Expand all", Mouse.Left);
  runtime.yieldTime(1.0);
  runtime.input().mouseClickItem("Root/*/*/Assets Tree/All", Mouse.Left);
  runtime.yieldTime(1.0);
  runtime.input().mouseClick(Mouse.Right);
  println("Success, Mouse Click (Item)!")
})

tr.registerTest("quirrel_mouse_modifier_click", function(runtime) {
  local assets = runtime.queryItem("Root/*/*/Assets Tree");
  local collapse = runtime.queryItem("Root/*/*/Assets Tree/Collapse all");
  if (assets != null && collapse != null) {
    local cSize = collapse.bb.Max - collapse.bb.Min;
    local x = collapse.bb.Max.x - (cSize.y / 2);
    local top = Point2(x, collapse.bb.Max.y + cSize.y);
    local bottom = Point2(x, assets.bb.Max.y - cSize.y * 2);
    local shiftClickScroll = function(pos) {
      runtime.input().mousePosition(pos);
      runtime.yieldFrame();
      runtime.input().mouseClickModifier(Mouse.Left, Modifier.Shift);
      runtime.yieldTime(1.0);
    }
    runtime.yieldTime(1.0);
    shiftClickScroll(top);
    local checkLastAsset = function(should) {
      local item = runtime.queryItem("**/cdk_color_tex_palette");
      return (should && item != null) || (!should && item == null);
    }
    local checkTop = checkLastAsset(false);
    shiftClickScroll(bottom);
    local checkBottom = checkLastAsset(true);
    shiftClickScroll(top);
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

tr.registerTest("quirrel_keys", function(runtime) {
  runtime.input().mouseMoveItem("Root/*/*/Viewport", 1.0);
  runtime.yieldTime(1.0);
  runtime.input().mouseClick(Mouse.Left);
  runtime.yieldTime(1.0);
  runtime.input().keyPress(Keys.Z)
  runtime.yieldTime(1.0);
  local changePerspective = function(k, m) {
    runtime.input().keyPressModifier(k, m)
    runtime.yieldTime(0.1);
    runtime.input().keyPress(Keys.Z)
    runtime.yieldTime(1.0);
  }
  changePerspective(Keys.F, Modifier.Shift)
  runtime.input().keyPress(Keys.Z)
  runtime.yieldTime(1.0);
  changePerspective(Keys.K, Modifier.Shift)
  changePerspective(Keys.T, Modifier.Shift)
  changePerspective(Keys.B, Modifier.Shift)
  changePerspective(Keys.L, Modifier.Shift)
  changePerspective(Keys.R, Modifier.Shift)
  changePerspective(Keys.P, Modifier.Shift)
  println("Success, Keys!")
})

tr.registerTest("quirrel_character_input", function(runtime) {
  runtime.input().mouseMoveItem("Root/*/*/Assets Tree/##searchInput", 1.0);
  runtime.yieldTime(1.0);
  runtime.input().mouseClick(Mouse.Left);
  runtime.yieldTime(1.0);
  runtime.input().characterInput("cdk_building_doors", 0.2)
  runtime.yieldTime(1.0);
  runtime.input().mouseClick(Mouse.Left);
  runtime.yieldTime(1.0);
  runtime.input().keyPress(Keys.Home);
  runtime.yieldTime(1.0);
  runtime.input().keyHeldModifier(Keys.RightArrow, Modifier.Shift, 3.0);
  runtime.yieldTime(1.0);
  runtime.input().keyPress(Keys.LeftArrow);
  runtime.yieldTime(1.0);
  runtime.input().keyHeld(Keys.Delete, 5.0);
  println("Success, Character Input!")
})

tr.registerTest("quirrel_mouse_drag", function(runtime) {
  local viewport = runtime.queryItem("Root/*/*/Viewport")
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
  runtime.input().mouseDrag(Mouse.Middle, from, to, 5.0)
  println("Success, Mouse drag!")
})

// Checks that only the button under the mouse is hovered, and that both buttons are visible.
// queryChildren() must also report the hovered button.
tr.registerTest("quirrel_item_hovered", function(runtime) {
  let collapsePath = "Root/*/*/Assets Tree/Collapse all"
  let expandPath = "Root/*/*/Assets Tree/Expand all"
  foreach ([underMousePath, otherPath] in [[collapsePath, expandPath], [expandPath, collapsePath]]) {
    runtime.input().mouseMoveItem(underMousePath, 1.0)
    runtime.yieldTime(1.0)
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
    let children = runtime.queryChildren("Root/*/*/Assets Tree")
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
tr.registerTest("quirrel_window_hovered", function(runtime) {
  let windowPath = "Root/*/*/Assets Tree"
  let buttonPath = $"{windowPath}/Collapse all"
  runtime.input().mouseMoveItem(buttonPath, 1.0)
  runtime.yieldTime(1.0)
  let inside = runtime.queryItem(windowPath)
  let button = runtime.queryItem(buttonPath)
  runtime.input().mouseMoveItem("Root/*/*/Viewport", 1.0)
  runtime.yieldTime(1.0)
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
tr.registerTest("quirrel_item_active", function(runtime) {
  let path = "Root/*/*/Assets Tree/Expand all"
  runtime.input().mouseMoveItem(path, 1.0)
  runtime.yieldTime(1.0)
  let before = runtime.queryItem(path)
  runtime.input().mouseButtonDown(Mouse.Left)
  runtime.yieldTime(1.0)
  let held = runtime.queryItem(path)
  runtime.input().mouseButtonUp(Mouse.Left)
  runtime.yieldTime(1.0)
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
  let panelPath = "Root/*/*/Properties"
  if (!runtime.input().mouseMoveItem(panelPath, 1.0))
    return
  runtime.input().mouseButtonDown(Mouse.Middle)
  runtime.yieldTime(0.5)
  let heldPanel = runtime.queryItem(panelPath)
  runtime.input().mouseButtonUp(Mouse.Middle)
  runtime.yieldTime(0.5)
  if (heldPanel == null || !heldPanel.isWindow || heldPanel.active != false) {
    runtime.markFailed($"Properties held with the middle mouse button: found {heldPanel != null}, active {heldPanel?.active}")
    return
  }
  println("Success, Item Active!")
})

// The property panel shows the "Common parameters" group only while an asset is selected.
function selectAsset(runtime, name) {
  let searchPath = "Root/*/*/Assets Tree/##searchInput"
  runtime.input().mouseClickItem(searchPath, Mouse.Left)
  runtime.yieldTime(1.0)
  runtime.input().characterInput(name, 0.2)
  runtime.yieldTime(1.0)
  runtime.input().mouseClickItem($"**/{name}", Mouse.Left)
  runtime.yieldTime(1.0)
  runtime.input().mouseClickItem(searchPath, Mouse.Left)
  runtime.yieldTime(1.0)
  runtime.input().keyPressModifier(Keys.A, Modifier.Ctrl)
  runtime.yieldTime(1.0)
  runtime.input().keyPress(Keys.Delete)
  runtime.yieldTime(1.0)
}

// Checks that a group header reports toggledOpen only in the frame of the click that opens or closes it.
tr.registerTest("quirrel_item_toggled_open", function(runtime) {
  let path = "**/Common parameters"
  if (runtime.queryItem(path) == null)
    selectAsset(runtime, "cdk_building_doors")
  runtime.input().mouseMoveItem(path, 1.0)
  runtime.yieldTime(1.0)
  // Two clicks, so the group ends in the state it started in.
  for (local i = 0; i < 2; i++) {
    let before = runtime.queryItem(path)
    runtime.input().mouseButtonDown(Mouse.Left)
    runtime.yieldTime(1.0)
    // The group header toggles on the release, and mouseButtonUp() returns after that frame.
    runtime.input().mouseButtonUp(Mouse.Left)
    let toggled = runtime.queryItem(path)
    runtime.yieldFrame()
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
    runtime.yieldTime(1.0)
  }
  println("Success, Item Toggled Open!")
})

// The menu item opens the dialog if it is closed, and closes it if it is open.
function toggleEnvironmentSettings(runtime) {
  runtime.input().mouseClickItem("##MainMenuBar/Settings", Mouse.Left)
  runtime.yieldTime(1.0)
  runtime.input().mouseClickItem("**/Environment settings...", Mouse.Left)
  runtime.yieldTime(1.0)
}

// Checks the state of a window that only the previous frame submitted. ImGui still reports the closed window as
// HoveredWindow in the frame after it closed, but the query must not report it hovered.
tr.registerTest("quirrel_previous_frame_window", function(runtime) {
  let dialogPath = "Environment Settings"
  let wasOpen = runtime.queryItem(dialogPath) != null
  if (!wasOpen)
    toggleEnvironmentSettings(runtime)
  runtime.input().mouseMoveItem($"{dialogPath}/Close", 1.0)
  runtime.yieldTime(1.0)
  runtime.input().mouseButtonDown(Mouse.Left)
  runtime.yieldTime(0.2)
  runtime.input().mouseButtonUp(Mouse.Left)

  // TODO: we need a yield that waits for actObjects() because the ButtonPropertyControl uses a delayed callback.
  runtime.yieldTime(0.2)

  let lastFrame = runtime.queryItem(dialogPath)
  runtime.yieldFrame()
  let previousFrame = runtime.queryItem(dialogPath)
  let previousChildren = runtime.queryChildren(dialogPath)
  runtime.yieldFrame()
  let closed = runtime.queryItem(dialogPath)
  if (wasOpen)
    toggleEnvironmentSettings(runtime)

  let childWithState = previousChildren?.findvalue(@(c) c.hovered != null || c.active != null)
  if (lastFrame == null || lastFrame.hovered != true) {
    runtime.markFailed($"Environment Settings in its last frame: found {lastFrame != null}, hovered {lastFrame?.hovered}")
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
