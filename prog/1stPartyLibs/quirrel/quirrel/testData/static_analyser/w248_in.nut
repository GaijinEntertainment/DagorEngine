
function _foo(dict, text = null) {
    if (text in dict)
        return dict[text]

    return text.d
}