options gen2

expect 30254

typedef distinct Id = int

[export]
def main {
    var t : table<Id; int>      // 30254: distinct can't be a table key
}
