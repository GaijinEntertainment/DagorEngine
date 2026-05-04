from "daRg" import set_kb_focus
let nameFilter = require("nameFilter.nut")

return @(filterString, selectedCompName) nameFilter(filterString, {
  placeholder = "Filter by name"
  onChange = function(text) {
    filterString.set(text)

    if ((text.len() > 0 ) && selectedCompName.get()
      && !selectedCompName.get().contains(text)) {
      selectedCompName.set(null)
    }
  }
  onEscape = @() set_kb_focus(null)
  onReturn = @() set_kb_focus(null)
  onClear = function() {
    filterString.set("")
    set_kb_focus(null)
  }
})
