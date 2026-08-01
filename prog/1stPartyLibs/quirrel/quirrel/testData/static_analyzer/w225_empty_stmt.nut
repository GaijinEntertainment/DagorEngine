// Ensure there are no false positives after the last semicolon

function _foo(filename) {
  assert(type(filename)=="string");
  return {};
}


function _oneliner() { return []; }
