options gen2

module _with_module_helper

def private secret_add(a, b : int) : int => a + b

def private who_am_i() : int => 1

def private local_probe() : int => 7

def private secret_generic(x : auto(TT)) : TT => secret_scale > 0 ? x + x : x

let private secret_scale = 3

var private secret_counter = 0

struct private SecretBox {
    value : int
}

enum private SecretEnum {
    Alpha
    Beta
    Gamma
}

typedef private SecretAlias = int
