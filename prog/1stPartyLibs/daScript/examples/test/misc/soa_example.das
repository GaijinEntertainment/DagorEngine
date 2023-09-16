require daslib/soa

[soa]
struct Particle
    pos : float3
    color : float4

[export]
def main
    var soa : Particle`SOA
    for i in range(3)
        soa |> push <| [[Particle pos=float3(i)]]
    let t = soa[2].pos     // macro replaces this with soa . . pos[2]
    assert(t==float3(2))
    let len = length(soa)
    assert(len==3)
    soa |> erase(1)
    assert(length(soa)==2)
    assert(soa[1].pos==float3(2))
    for it in soa
        print("it.pos={it.pos} it.color={it.color}\n")
    var count = 0
    for q,it in count(),soa
        count ++
    assert(count==2)

