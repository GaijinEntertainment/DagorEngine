
function foo() { return { editorIsActive = 42 } }

function bart(x) { return "" }

local editorState = null
local editorIsActive = "null";

editorState = foo()
editorIsActive = editorState?.editorIsActive ?? editorIsActive

bart(editorIsActive.xx)
