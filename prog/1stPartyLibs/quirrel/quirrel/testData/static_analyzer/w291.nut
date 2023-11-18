


function _foo(
    x,
    _y,  // EXPECTED
    _z   // FP
    ) {
    return x + _y;
}
