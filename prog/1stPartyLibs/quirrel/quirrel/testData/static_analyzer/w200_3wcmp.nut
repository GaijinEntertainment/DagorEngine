// This check is (intentionally?) disabled for <=> operator.
// If needed, change isBoolRelationOperator() to isRelationOperator()
// in checkPotentiallyNullableOperands() function in analyzer

function _itemsSorter(a, b) { // -return-different-types
  a = a?.item
  b = b?.item
  if (!a || !b)
    return a <=> b
  return 42
}
