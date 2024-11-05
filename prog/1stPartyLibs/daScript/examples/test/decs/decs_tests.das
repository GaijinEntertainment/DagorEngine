options persistent_heap = true
options gc

options log_compile_time

require daslib/decs_boost
require daslib/apply
require daslib/static_let

// can specify prefix="string", prefix for nothing, or not specify for Particle_
[decs_template(prefix)]
struct Particle
    pos, vel : float3

[export]
def main
    test_query_nested_macro()
    test_arrays()
    test_query_1()
    test_query_2()
    test_create_delete()
    test_update()
    test_type_mismatch()
    test_query()
    test_query_macro()
    test_strings()
    test_decs_macro()

[decs(stage = update)]
def nested_with_break_and_continue(var p: Particle)
    p.pos.x += 13.                  // we don't call this one
    while p.pos.x > 0.0             // it is a compilation only test for break and continue inside loops
        p.pos.x -= 0.1
        if (p.pos.x < 5.0)
            break
    while p.pos.x > 0.0
        p.pos.x -= 1.
        if (p.pos.x < 5.0)
            continue
        p.pos.x -= 0.1

def test_query_nested_macro
    print("\n\nTEST NESTED QUERY\n")
    restart()
    for i in range(3)
        create_entity <| @ ( eid, cmp )
            apply_decs_template(cmp, [[Particle pos=float3(i), vel=float3(i+1)]])
    commit()
    // debug_dump()
    query <| $ ( var q : Particle )
        print("q = {q.pos} += {q.vel}\n")
        let qp = q.pos
        let qv = q.vel
        query <| $ ( var p : Particle )
            print("\tq = ({q.pos} , {q.vel})\n")
            print("\tp = ({p.pos} , {p.vel})\n")
            if p.pos==q.pos
                return
            assert(qp==q.pos)
            assert(qv==q.vel)

def test_query_1
    print("\n\nTEST QUERY ONE MACRO\n")
    restart()
    static_let <|
        var kaboom : EntityId
    for i in range(3)
        create_entity <| @ ( eid, cmp )
            cmp.eid := eid
            cmp.pos := float3(i)
            cmp.vel := float3(1.,2.,3.)
            if i==1
                kaboom = eid
    commit()
    debug_dump()
    query(kaboom) <| $ ( var pos:float3&; vel:float3; col:uint=13u )
        pos += vel
        assert(col==13u)
        print("index = {entity_index}\n")
    commit()
    query() <| $ ( eid:EntityId; pos, vel : float3 )
        print("[{eid}] pos={pos} vel={vel}\n")
    debug_dump()

def test_query_2
    print("\n\nTEST QUERY MACRO (2)\n")
    restart()
    static_let <|
        var kaboom : EntityId
    for i in range(3)
        create_entity <| @ ( eid, cmp )
            cmp.eid := eid
            cmp.pos := float3(i)
            cmp.vel := float3(1.,2.,3.)
            if i==1
                kaboom = eid
    commit()
    var num_found = 0
    find_query <| $ ( var pos:float3&; vel:float3; col:uint=13u )
        print("found {pos}\n")
        num_found ++
        return pos.x==1.
    commit()
    assert(num_found==2)

[decs(stage="update")]
def update_particle ( var pos:float3&; vel:float3 )
    pos += vel

[decs(stage="update")]
def make_visible ( var col:float4& )
    col.w = 1.

[decs(stage="check")]
def check_visible ( col:float4 )
    assert(col.w==1.)

def test_decs_macro
    restart()
    for i in range(3)
        create_entity <| @ ( eid, cmp )
            cmp.pos := float3(i)
            cmp.vel := float3(.1,.2,.3)
            cmp.col := float4(0.)
    decs_stage("update")
    decs_stage("check")
    debug_dump()

def test_arrays
    print("\n\nTEST ARRAYS\n")
    restart()
    for i in range(3)
        create_entity <| @ ( eid, cmp )
            cmp |> set("values", [{for x in range(i+1); x}])
    commit()
    print("BEFORE GC:\n")
    debug_dump()
    before_gc()
    unsafe
        heap_collect()
    after_gc()
    print("AFTER GC:\n")
    debug_dump()
    var j = 0
    query <| $ ( values:array<int> )
        print("values = {values}\n")
        assert(length(values)==j+1)
        for i in range(j+1)
            assert(values[i]==i)
        j ++

def test_strings
    print("\n\nTEST STRINGS\n")
    restart()
    for i in range(3)
        create_entity <| @ ( eid, cmp )
            cmp.pos := float3(i)
            cmp.txt := "hello_{i}"
    commit()
    debug_dump()
    before_gc()
    unsafe
        heap_collect(true)
    after_gc()
    debug_dump()
    var j = 0
    query <| $ ( txt:string )
        assert(txt=="hello_{j}")
        j ++
    debug_dump()

def test_query_macro
    print("\n\nTEST QUERY MACRO\n")
    restart()
    for i in range(3)
        create_entity <| @ ( eid, cmp )
            cmp.pos := float3(i)
            cmp.vel := float3(.1,.2,.3)
    for i in range(2)
        create_entity <| @ ( eid, cmp )
            cmp.pos := float3(i+3)
            cmp.vel := float3(.1,.2,.3)
            cmp.col := 0xffffffff
    commit()
    var count = 0
    var count_ff = 0
    query <| $ ( var pos:float3&; vel:float3; col:uint=0x12345678 )
        pos += vel
        if col==0x12345678
            count_ff ++
        count ++
    assert(count==5)
    assert(count_ff==3)
    debug_dump()
    var count_nc = 0
    query <| $ [REQUIRE_NOT(col)] ( var pos:float3& )
        pos = float3(13)
        count_nc ++
    assert(count_nc==3)
    debug_dump()
    var count_par = 0
    query <| $ ( var p : Particle )
        p.pos += p.vel
        count_par ++
    assert(count_par==5)
    debug_dump()
    var count_npar = 0
    query <| $ ( [[prefix="nothere_"]] var p : Particle )
        p.pos += p.vel
        count_npar ++
    assert(count_npar==0)
    debug_dump()

def test_query
    print("\n\nTEST QUERY\n")
    restart()
    for i in range(3)
        create_entity <| @ ( eid, cmp )
            cmp.pos := float3(i)
            cmp.vel := float3(.1,.2,.3)
    for i in range(3)
        create_entity <| @ ( eid, cmp )
            cmp.pos := float3(i+100)
            cmp.vel := float3(.1,.2,.3)
            cmp.col := 0xffffffff
    commit()
    // query 1
    static_let <|
        var erq1 <- [[EcsRequest
            req <- [{string "pos"; "vel"}],
            at <- EcsRequestPos(get_line_info())
        ]]
    var total1 = 0
    erq1 |> for_each_archetype <| $ ( arch )
        for pos, vel in get(arch,"pos",type<float3>), get(arch,"vel",type<float3>)
            pos += vel
            total1 ++
    assert(total1==6)
    // query 2
    var total2 = 0
    static_let <|
        var erq2 <- [[EcsRequest
            req <- [{string "pos"; "col"}],
            at <- EcsRequestPos(get_line_info())
        ]]
    erq2 |> for_each_archetype <| $ ( arch )
        for col,pos in get(arch,"col",type<uint>),get_ro(arch,"pos",type<float3>)
            if pos.x > 101.
                col =  0xff00ffff
            // pos += float3(1) // this will fail due to "ro"
            total2 ++
    assert(total2==3)
    // query 3
    var total3 = 0
    var found3 = 0
    static_let <|
        var erq3 <- [[EcsRequest
            req <- [{string "pos"}],
            at <- EcsRequestPos(get_line_info())
        ]]
    erq3 |> for_each_archetype <| $ ( arch )
        for col,pos in get_default_ro(arch,"col",0x12345678), get(arch,"pos",type<float3>)
            if col==0xff00ffff
                pos = float3(13)
                found3 ++
            total3 ++
    assert(total3==6)
    assert(found3==2)
    debug_dump()
    var total4 = 0
    static_let <|
        var erq4 <- [[EcsRequest
            req <- [{string "pos"}],
            reqn <- [{string "col"}],
            at <- EcsRequestPos(get_line_info())
        ]]
    erq4 |> for_each_archetype <| $ ( arch )
        for pos in get(arch,"pos",type<float3>)
            pos.y -= 0.001
            total4 ++
    assert(total4==3)
    debug_dump()

def test_type_mismatch
    print("\n\nTEST TYPE MISMATCH\n")
    restart()
    create_entity <| @ ( eid, cmp )
        cmp.pos := float3(1,2,3)
        try
            cmp |> set("pos", 13)
        recover
            print("mismatch set\n")
        try
            cmp |> get("pos", type<int>)
        recover
            print("mismatch get\n")
        let f123 = cmp |> get("pos", float3(13))
        print("get[pos] = {f123}\n")
    commit()
    debug_dump()

def test_update
    print("\n\nTEST UPDATE\n")
    restart()
    static_let <|
        var migr : EntityId
    print("before:\n")
    create_entity <| @ ( eid, cmp )
        cmp.pos := float3(1,2,3)
        cmp.vel := float3(0.,-9.8,0.)
        migr = eid
    commit()
    debug_dump()
    print("after add:\n")
    migr |> update_entity <| @ ( eid, cmp )
        cmp.color := float4(1,0,0,1)
        migr = eid
    commit()
    debug_dump()
    print("after remove:\n")
    migr |> update_entity <| @ ( eid, cmp )
        let t = cmp |> has("vel")
        print("t = {t}\n")
        verify(cmp |> has("vel"))
        cmp |> remove("vel")
        verify(!(cmp |> has("vel")))
        migr = eid
    commit()
    debug_dump()
    print("without change:\n")
    migr |> update_entity <| @ ( eid, cmp )
        print("nothing\n")
        migr = eid
    commit()
    debug_dump()

struct Foo
    pos : float3
    name : string
    eid : EntityId
    foobar : int

def test_create_delete
    print("\n\nTEST CREATE-DELETE\n")
    restart()
    print("creating:\n")
    create_entity <| @ ( eid, cmp )
        print("eid = {eid}\n")
        cmp.pos  := float3(1.,2.,3.)
        cmp.name := "entity_once"
    create_entity <| @ ( eid, cmp )
        print("eid = {eid}\n")
        var foo = [[Foo pos=float3(1.), name="from template",eid=eid]]
        apply(foo) <| $ ( n, v )
            cmp |> set ( n, v)

    static_let <|
        var de : array<EntityId>
    for i in range(3)
        create_entity <| @ ( eid, cmp )
            print("i={i} eid = {eid}\n")
            if i==1 || i==2
                de |> push(eid)
            cmp.pos  := float3(1+i,2+i,3+i)
            cmp.name := "entity{i}"
            cmp.id   := i
    commit()
    debug_dump()

    print("after delete:\n")
    print("deleting {de}\n")
    for d in de
        delete_entity(d)
    commit()
    debug_dump()
