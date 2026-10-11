options gen2

module _distinct_eq_a shared public

typedef distinct Conn = int

def operator ==(a, b : Conn) : bool => *a == *b
