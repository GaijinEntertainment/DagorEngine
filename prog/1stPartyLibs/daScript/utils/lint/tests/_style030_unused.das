options gen2

module _style030_unused

// A plain source module whose symbols are never referenced by the importer —
// requiring it is genuinely dead, so STYLE030 must fire.
def style030_unused_value() : int {
    return 7
}
