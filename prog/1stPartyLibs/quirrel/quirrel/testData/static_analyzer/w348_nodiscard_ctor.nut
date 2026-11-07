class Squad {
  function [nodiscard] constructor(_n) {}
}
class Free {
  constructor(_n) {}
}
class Unit(Squad) {}

Squad(4)                      //warning:discarded-nodiscard
Free(4)
Unit(4)                       //warning:discarded-nodiscard
let _squad = Squad(4)
class FreeUnit(Free) {}
FreeUnit(4)
