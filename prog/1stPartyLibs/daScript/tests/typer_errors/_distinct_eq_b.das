options gen2

module _distinct_eq_b shared public

require _distinct_eq_a public

def operator ==(a, b : Conn) : bool => *a == *b
