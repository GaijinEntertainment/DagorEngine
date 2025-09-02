from "daRg" import set_kb_focus
let nameFilter = require("nameFilter.nut")

return @(filterString, selectedCompName) nameFilter(filterString, {
  placeholder = "Filter by name"

  function onChange(text) {
    filterString.set(text)

    if ((text.len() > 0 ) && selectedCompName.get()
      && !selectedCompName.get().contains(text)) {
      selectedCompName.set(null)
    }
  }

  function onEscape() {
    set_kb_focus(null)
  }

  function onReturn() {
    set_kb_focus(null)
  }

  function onClear() {
    filterString.set("")
    set_kb_focus(null)
  }
})
