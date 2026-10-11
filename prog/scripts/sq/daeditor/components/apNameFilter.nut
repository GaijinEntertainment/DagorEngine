from "daRg" import set_kb_focus
let nameFilter = require("nameFilter.nut")

return @(filterString, selectedCompKey, deselectComp) nameFilter(filterString, {
  placeholder = "Filter by name"
  onChange = function(text) {
    filterString.set(text)

    let key = selectedCompKey.get()
    if (text.len() > 0 && key != null && !key.contains(text))
      deselectComp()
  }
  onEscape = @() set_kb_focus(null)
  onReturn = @() set_kb_focus(null)
  onClear = function() {
    filterString.set("")
    set_kb_focus(null)
  }
})
