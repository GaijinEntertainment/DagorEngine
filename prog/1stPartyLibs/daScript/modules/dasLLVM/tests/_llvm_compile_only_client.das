options gen2

def private compute(a, b : int) : int => a * 100 + b

[export]
def answer(var out : int?) {
    *out = compute(4, 2)
}
