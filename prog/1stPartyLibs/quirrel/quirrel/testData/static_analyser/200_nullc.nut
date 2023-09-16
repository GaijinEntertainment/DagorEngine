function foo() {}

let item = foo()
let res = foo()


let _x =  (((item?.isPrimaryBuy ?? false) > (res?.isPrimaryBuy ?? null) ? item : res))