options gen2
var a : array<int[1]>

[export]
def main() {
    var toCopy = fixed_array(42)
    a |> push_clone <| toCopy
    a |> push <| toCopy
    a |> emplace <| toCopy
}

options log
