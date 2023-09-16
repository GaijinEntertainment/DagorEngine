require fio

let MAX_ARGUMENTS = 16

def genWrapperFunc(fout:FILE const?; nargs,res,perm:int )
    let call_result = res!=0 ? "vec4f" : "int64_t"
    let args_prefix = nargs!=0 ? "args" : ""
    fprint(fout,"vec4f fastcall64_{nargs}_{res}_{perm} ( void * fn, vec4f * {args_prefix} ) \{\n")
    fprint(fout,"    using call_kind = {call_result} ( * ) ( ")
    for arg in range(nargs)
        if arg!=0
            fprint(fout,",")
        if (perm & (1<<arg))!=0
            fprint(fout,"double")
        else
            fprint(fout,"int64_t")
    fprint(fout," );\n")
    fprint(fout,"    return ")
    if res==0
        fprint(fout,"Rx ( ")
    fprint(fout,"call_kind(fn) ( ")
    for arg in range(nargs)
        if arg!=0
            fprint(fout,",")
        if (perm & (1<<arg))!=0
            fprint(fout,"AD({arg})")
        else
            fprint(fout,"AX({arg})")
    if res==0
        fprint(fout," )")
    fprint(fout," );\n")
    fprint(fout,"\}\n\n")

def genWrapper(name:string; iregs:int )
    var max_perm = (1<<iregs)
    fopen(name,"wb") <| $ ( fout )
        for nargs in range(MAX_ARGUMENTS)
            for res in range(2)
                for perm in range(max_perm)
                    genWrapperFunc(fout, nargs, res, perm)
        fprint(fout, "#define MAX_WRAPPER_ARGUMENTS  {MAX_ARGUMENTS}\n\n")
        fprint(fout, "FastCallWrapper fastcall64_table [] = \{\n")
        for nargs in range(MAX_ARGUMENTS)
            for res in range(2)
                fprint(fout,"    ")
                for perm in range(max_perm)
                    fprint(fout,"&fastcall64_{nargs}_{res}_{perm},")
                fprint(fout,"\n")
        fprint(fout, "};\n\n")

let ARG0_D = (1<<0)
let ARG1_D = (1<<1)
let ARG2_D = (1<<2)
let ARG3_D = (1<<3)
let ARG4_D = (1<<4)
let ARG5_D = (1<<5)
let ARG6_D = (1<<6)
let ARG7_D = (1<<7)
let ARG8_D = (1<<8)
let ARG9_D = (1<<9)
let ARG10_D = (1<<10)
let ARG11_D = (1<<11)
let ARG12_D = (1<<12)
let ARG13_D = (1<<13)
let ARG14_D = (1<<14)
let ARG15_D = (1<<15)

let RES_D = 0
let RES_X = 1

struct WrapperFn
    nargs : int
    res   : int
    perm  : int

def genExtraWrapper(name:string; extra:array<WrapperFn>)
    fopen(name,"wb") <| $ ( fout )
        for fn in systemV_extra
            genWrapperFunc(fout,fn.nargs,fn.res,fn.perm)
        fprint(fout, "FastCallExtraWrapper fastcall64_extra_table [] = \{\n")
        for fn in extra
            fprint(fout,"    {fn.nargs},{fn.res},{fn.perm},&fastcall64_{fn.nargs}_{fn.res}_{fn.perm},\n")
        fprint(fout, "};\n\n")

let systemV_extra <- [{WrapperFn
// glPrimitiveBoundingBoxARB
    nargs=8, res=1, perm=0|ARG0_D|ARG1_D|ARG2_D|ARG3_D|ARG4_D|ARG5_D|ARG6_D|ARG7_D;
// glDrawVkImageNV
    nargs=11, res=1, perm=0|ARG2_D|ARG3_D|ARG4_D|ARG5_D|ARG6_D|ARG7_D|ARG8_D|ARG9_D|ARG10_D;
// glPathGlyphsNV
    nargs=10, res=1, perm=0|ARG9_D;
// glPathGlyphRangeNV, glPathMemoryGlyphIndexArrayNV
    nargs=9, res=1, perm=0|ARG8_D;
// glPathGlyphIndexArrayNV
    nargs=8, res=1, perm=0|ARG7_D;
// DUMMY
    nargs = 13, res = RES_X,    perm=ARG0_D|ARG13_D;
    nargs = 13, res = RES_D,    perm=ARG0_D|ARG13_D
}]

[export]
def main
    genWrapper("{get_das_root()}/src/builtin/win_x86_64_wrapper.inc",4)
    genWrapper("{get_das_root()}/src/builtin/systemV_64_wrapper.inc",6)
    genExtraWrapper("{get_das_root()}/src/builtin/systemV_64_extra_wrapper.inc",systemV_extra)
