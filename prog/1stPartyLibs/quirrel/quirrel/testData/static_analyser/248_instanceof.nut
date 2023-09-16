
class Watched {}

function _foo(text = null) {
    if (text instanceof Watched)
        text = text.value
}