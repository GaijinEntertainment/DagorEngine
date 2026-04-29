options gen2
module tutorial_helpers public

struct Item {
    name : string
    value : int
}

def make_item(n : string; v : int) : Item {
    return Item(name = n, value = v)
}

def describe(item : Item) : string {
    return "{item.name} (worth {item.value})"
}

def private internal_id(item : Item) : int {
    return item.value * 31
}

let MAX_ITEMS = 100
