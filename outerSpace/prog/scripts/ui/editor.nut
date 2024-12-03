from "%darg/ui_imports.nut" import *

local editor = null
local editorState = null
local showUIinEditor = Watched(true)
local editorIsActive = Watched(false)

if (require_optional("daEditorEmbedded") != null) {
   editor = require_optional("%daeditor/editor.nut")
   editorState = require_optional("%daeditor/state.nut")
   showUIinEditor = editorState?.showUIinEditor ?? showUIinEditor
   showUIinEditor.set(true)
   editorIsActive = editorState?.editorIsActive ?? editorIsActive
}

return {
  editor, showUIinEditor, editorIsActive
}