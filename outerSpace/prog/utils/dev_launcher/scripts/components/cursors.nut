let {mkCursor, setTooltip, tooltipCmp, cursor} = require("%darg/helpers/simpleCursors.nut")

return {
  setTooltip,
  normal = mkCursor(cursor, tooltipCmp),
  hidden = mkCursor(tooltipCmp)
}
