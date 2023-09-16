require math
require raster

[sideeffects]
def test_gather_scatter
    unsafe
        let idx = uint4(1,3,7,8)
        let ridx = uint4(0,2,6,7)
        // int
        var arr <- [{int 10;11;12;13;14;15;16;17;18;19}]
        var rarr : array<int>; rarr |> resize(10)
        gather_scatter(addr(rarr[0]),ridx,addr(arr[0]),idx)
        assert(rarr[0]==11 && rarr[2]==13 && rarr[6]==17 && rarr[7]==18)
        // uint
        var arru <- [{uint 10u;11u;12u;13u;14u;15u;16u;17u;18u;19u}]
        var rarru : array<uint>; rarru |> resize(10)
        gather_scatter(addr(rarru[0]),ridx,addr(arru[0]),idx)
        assert(rarru[0]==11u && rarru[2]==13u && rarru[6]==17u && rarru[7]==18u)
        // float
        var arrf <- [{float 10.;11.;12.;13.;14.;15.;16.;17.;18.;19.}]
        var rarrf : array<float>; rarrf |> resize(10)
        gather_scatter(addr(rarrf[0]),ridx,addr(arrf[0]),idx)
        assert(rarrf[0]==11. && rarrf[2]==13. && rarrf[6]==17. && rarrf[7]==18.)

[sideeffects]
def test_gather
    unsafe
        let idx = uint4(1,3,7,8)
        // int
        var arr <- [{int 10;11;12;13;14;15;16;17;18;19}]
        let res_i4 = gather(addr(arr[0]), idx)
        assert(int4(11,13,17,18)==res_i4)
        // uint
        var arru <- [{uint 10u;11u;12u;13u;14u;15u;16u;17u;18u;19u}]
        let res_u4 = gather(addr(arru[0]), idx)
        assert(uint4(11,13,17,18)==res_u4)
        // float
        var arrf <- [{float 10.;11.;12.;13.;14.;15.;16.;17.;18.;19.}]
        let res_f4 = gather(addr(arrf[0]), idx)
        assert(float4(11,13,17,18)==res_f4)

[sideeffects]
def test_scatter
    unsafe
        let idx = uint4(1,3,7,8)
        // int
        var arr : array<int>; arr |> resize(10)
        scatter(addr(arr[0]), idx, int4(11,13,17,18))
        assert(arr[1]==11 && arr[3]==13 && arr[7]==17 && arr[8]==18)
        // uint
        var arru : array<uint>; arru |> resize(10)
        scatter(addr(arru[0]), idx, uint4(11,13,17,18))
        assert(arru[1]==11u && arru[3]==13u && arru[7]==17u && arru[8]==18u)
        // float
        var arrf : array<float>; arrf |> resize(10)
        scatter(addr(arrf[0]), idx, float4(11,13,17,18))
        assert(arrf[1]==11. && arrf[3]==13. && arrf[7]==17. && arrf[8]==18.)

[sideeffects]
def test_scatter_mask
    unsafe
        let idx = uint4(1,3,7,8)
        let mask = uint4(1,3,2,3)
        // int
        var arr : array<int>; arr |> resize(10)
        scatter_neq_mask(addr(arr[0]), idx, int4(11,13,17,18), int4(1,13,2,3))
        assert(arr[1]==11 && arr[3]==0 && arr[7]==17 && arr[8]==18)
        // uint
        var arru : array<uint>; arru |> resize(10)
        scatter_neq_mask(addr(arru[0]), idx, uint4(11,13,17,18), uint4(1,3,17,3))
        assert(arru[1]==11u && arru[3]==13u && arru[7]==0u && arru[8]==18u)
        // float
        var arrf : array<float>; arrf |> resize(10)
        scatter_neq_mask(addr(arrf[0]), idx, float4(11,13,17,18), float4(11,1,2,18))
        assert(arrf[1]==0. && arrf[3]==13. && arrf[7]==17. && arrf[8]==0.)

[sideeffects]
def test_store_mask
    unsafe
        // int
        var res = int4(-1,-2,-3,-4)
        store_neq_mask(addr(res), int4(11,13,17,18), int4(1,13,2,3))
        assert(int4(11,-2,17,18)==res)
        // uint
        var resu = uint4(1u,2u,3u,4u)
        store_neq_mask(addr(resu), uint4(11,13,17,18), uint4(1,3,17,3))
        assert(uint4(11,13,3,18)==resu)
        // float
        var resf = float4(1.,2.,3.,4.)
        store_neq_mask(addr(resf), float4(11,13,17,18), float4(11,1,2,18))
        assert(float4(1.,13.,17.,4.)==resf)

[sideeffects]
def test_gather_scatter_mask
    unsafe
        let idx = uint4(1,3,7,8)
        let ridx = uint4(0,2,6,7)
        // int
        var arr <- [{int 10;11;12;13;14;15;16;17;18;19}]
        var rarr : array<int>; rarr |> resize(10)
        gather_scatter_neq_mask(addr(rarr[0]),ridx,addr(arr[0]),idx,int4(1,13,2,3))
        assert(rarr[0]==11 && rarr[2]==0 && rarr[6]==17 && rarr[7]==18)
        // uint
        var arru <- [{uint 10u;11u;12u;13u;14u;15u;16u;17u;18u;19u}]
        var rarru : array<uint>; rarru |> resize(10)
        gather_scatter_neq_mask(addr(rarru[0]),ridx,addr(arru[0]),idx,uint4(1,3,17,3))
        assert(rarru[0]==11u && rarru[2]==13u && rarru[6]==0u && rarru[7]==18u)
        // float
        var arrf <- [{float 10.;11.;12.;13.;14.;15.;16.;17.;18.;19.}]
        var rarrf : array<float>; rarrf |> resize(10)
        gather_scatter_neq_mask(addr(rarrf[0]),ridx,addr(arrf[0]),idx,float4(11,1,2,18))
        assert(rarrf[0]==0. && rarrf[2]==13. && rarrf[6]==17. && rarrf[7]==0.)

[sideeffects]
def test_gather_store_mask
    unsafe
        let idx = uint4(1,3,7,8)
        // int
        var arr <- [{int 10;11;12;13;14;15;16;17;18;19}]
        var rres = int4(-1,-2,-3,-4)
        gather_store_neq_mask(addr(rres),addr(arr[0]),idx,int4(1,13,2,3))
        assert(int4(11,-2,17,18)==rres)
        // uint
        var arru <- [{uint 10u;11u;12u;13u;14u;15u;16u;17u;18u;19u}]
        var rresu = uint4(1u,2u,3u,4u)
        gather_store_neq_mask(addr(rresu),addr(arru[0]),idx,uint4(1,3,17,3))
        assert(uint4(11,13,3,18)==rresu)
        // float
        var arrf <- [{float 10.;11.;12.;13.;14.;15.;16.;17.;18.;19.}]
        var rresf = float4(1.,2.,3.,4.)
        gather_store_neq_mask(addr(rresf),addr(arrf[0]),idx,float4(11,1,2,18))
        assert(float4(1.,13.,17.,4.)==rresf)

[sideeffects]
def test_gather_store_stride
    unsafe
        let idx = uint4(1,3,7,8)
        let ridx = uint4(0,2,6,7)
        // int
        var arr <- [{int 10;11;12;13;14;15;16;17;18;19}]
        var rarr : array<int>; rarr |> resize(10)
        gather_store_stride(addr(rarr[0]),2,addr(arr[0]),idx)
        assert(rarr[0]==11 && rarr[2]==13 && rarr[4]==17 && rarr[6]==18)


[export]
def test
    test_gather()
    test_scatter()
    test_scatter_mask()
    test_store_mask()
    test_gather_scatter()
    test_gather_scatter_mask()
    test_gather_store_mask()
    test_gather_store_stride()
    return true

[export]
def main
    test()
    print("passed\n")
