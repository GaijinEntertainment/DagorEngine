options gen2

module _distinct_fixture shared public

typedef distinct PubId = int
typedef private distinct Secret = int

def make_pub_id(v : int) : PubId => PubId(v)

def pub_id_value(v : PubId) : int => *v
