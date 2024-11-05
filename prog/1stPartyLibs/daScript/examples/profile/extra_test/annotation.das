// options log=true
// options log_cpp=true
// options log_nodes = true

module annotation

require testProfile

[es (es_pass="ks")]
def update(var pos:float3&;vel:float3 const&)   // all by ref
    pos += vel

// note, when optimization off won't compile, since float3(...) is not a const, but a function
[es (es_pass="ks2")]
def update2(var pos:float3&;vel:float3 const = float3(1.0f,2.0f,3.0f))   // deref .
    pos += vel

[es (es_pass="ks3")]
def update3(var pos:float3&;vel:float3 const &;dt:float = 1.0f)
    pos += vel * dt

[es (es_pass="ks4")]
def update4(some_name:string = "no_name")
	assert(some_name=="no_name")

def queryOne(dt:float=1.0f)
    testProfile::queryEs() <| $ [es] (var pos:float3&;vel:float3 const)
        pos += vel * dt

def queryTwo(dt:float=0.5f)
    testProfile::queryEs() <| $ [es] (var pos:float3&;vel:float3 const;defT:float=0.5f)
        pos += vel * (defT + dt)

def queryThree()
    var count = 0
    try
        testProfile::queryEs() <| $ [es] (var pos aka position:float3& const)
            testProfile::queryEs() <| $ [es] (var pos:float3& const)
                if position==pos
                    count ++
                    if count==100
                        panic("enough")
    recover
        assert(count==100)

[export]
def main
    testProfile::initEsComponents()
    testProfile::testEsUpdate("ks")
    testProfile::verifyEsComponents()
    testProfile::initEsComponents()
    testProfile::testEsUpdate("ks2")
    testProfile::verifyEsComponents()
    testProfile::initEsComponents()
    testProfile::testEsUpdate("ks3")
    testProfile::verifyEsComponents()
    testProfile::initEsComponents()
    queryOne()
    testProfile::verifyEsComponents()
    testProfile::initEsComponents()
    queryTwo()
    testProfile::verifyEsComponents()
    testProfile::initEsComponents()
    profile(20,"es-update") <|
        for i in range(100)
            testProfile::testEsUpdate("ks")
    profile(20,"es-update-ref") <|
        for i in range(100)
            testProfile::testEsUpdate("ks2")
    profile(20,"es-update-ref-const") <|
        for i in range(100)
            testProfile::testEsUpdate("ks3")
    profile(20,"es-update-def-string") <|
        for i in range(100)
            testProfile::testEsUpdate("ks4")
    profile(20,"es-query-1") <|
        for i in range(100)
            queryOne()
    profile(20,"es-query-2") <|
        for i in range(100)
            queryTwo()
    profile(20,"es-query-3") <|
        queryThree()
    testProfile::releaseEsComponents()

