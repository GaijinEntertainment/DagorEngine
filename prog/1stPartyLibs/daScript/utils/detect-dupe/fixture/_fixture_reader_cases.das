options gen2


def case_before_reader(a : int) : int {
    return a + 1
}


var pattern <- %nope~[a-z]+%%


def case_after_reader(a : int) : int {
    return a * 3
}
