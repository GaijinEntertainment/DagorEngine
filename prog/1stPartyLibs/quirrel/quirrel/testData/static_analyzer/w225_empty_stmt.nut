// Ensure there are no false positives after the last semicolon

let function _foo(filename) {
  assert(type(filename)=="string");
  return {};
}


let function _oneliner() { return []; }
