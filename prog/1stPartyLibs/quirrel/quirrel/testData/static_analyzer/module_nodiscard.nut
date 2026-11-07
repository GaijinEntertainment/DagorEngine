function [nodiscard] mkLoadout(slots) {
  return array(slots)
}

function mkSpareLoadout(slots) {
  return array(slots)
}

return { mkLoadout, mkSpareLoadout, slots = [1] }
