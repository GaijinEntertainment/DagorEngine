
function _foo(colorStr, f) {
    if ((type(colorStr) != "string" || (colorStr.len() != 8 && colorStr.len() != 6)) && (type(f) == "null") )
        return f("first param must be string with len 6 or 8")
}