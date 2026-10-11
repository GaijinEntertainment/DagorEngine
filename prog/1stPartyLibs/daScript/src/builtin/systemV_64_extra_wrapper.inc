vec4f fastcall64_8_0_255 ( void * fn, vec4f * args ) {
    using call_kind = int64_t ( * ) ( double,double,double,double,double,double,double,double );
    return Rx ( call_kind(fn) ( AD(0),AD(1),AD(2),AD(3),AD(4),AD(5),AD(6),AD(7) ) );
}

vec4f fastcall64_11_0_2044 ( void * fn, vec4f * args ) {
    using call_kind = int64_t ( * ) ( int64_t,int64_t,double,double,double,double,double,double,double,double,double );
    return Rx ( call_kind(fn) ( AX(0),AX(1),AD(2),AD(3),AD(4),AD(5),AD(6),AD(7),AD(8),AD(9),AD(10) ) );
}

vec4f fastcall64_10_0_512 ( void * fn, vec4f * args ) {
    using call_kind = int64_t ( * ) ( int64_t,int64_t,int64_t,int64_t,int64_t,int64_t,int64_t,int64_t,int64_t,double );
    return Rx ( call_kind(fn) ( AX(0),AX(1),AX(2),AX(3),AX(4),AX(5),AX(6),AX(7),AX(8),AD(9) ) );
}

vec4f fastcall64_9_0_256 ( void * fn, vec4f * args ) {
    using call_kind = int64_t ( * ) ( int64_t,int64_t,int64_t,int64_t,int64_t,int64_t,int64_t,int64_t,double );
    return Rx ( call_kind(fn) ( AX(0),AX(1),AX(2),AX(3),AX(4),AX(5),AX(6),AX(7),AD(8) ) );
}

vec4f fastcall64_8_0_128 ( void * fn, vec4f * args ) {
    using call_kind = int64_t ( * ) ( int64_t,int64_t,int64_t,int64_t,int64_t,int64_t,int64_t,double );
    return Rx ( call_kind(fn) ( AX(0),AX(1),AX(2),AX(3),AX(4),AX(5),AX(6),AD(7) ) );
}

vec4f fastcall64_7_0_126 ( void * fn, vec4f * args ) {
    using call_kind = int64_t ( * ) ( int64_t,double,double,double,double,double,double );
    return Rx ( call_kind(fn) ( AX(0),AD(1),AD(2),AD(3),AD(4),AD(5),AD(6) ) );
}

vec4f fastcall64_7_0_120 ( void * fn, vec4f * args ) {
    using call_kind = int64_t ( * ) ( int64_t,int64_t,int64_t,double,double,double,double );
    return Rx ( call_kind(fn) ( AX(0),AX(1),AX(2),AD(3),AD(4),AD(5),AD(6) ) );
}

vec4f fastcall64_9_0_96 ( void * fn, vec4f * args ) {
    using call_kind = int64_t ( * ) ( int64_t,int64_t,int64_t,int64_t,int64_t,double,double,int64_t,int64_t );
    return Rx ( call_kind(fn) ( AX(0),AX(1),AX(2),AX(3),AX(4),AD(5),AD(6),AX(7),AX(8) ) );
}

vec4f fastcall64_8_1_64 ( void * fn, vec4f * args ) {
    using call_kind = vec4f ( * ) ( int64_t,int64_t,int64_t,int64_t,int64_t,int64_t,double,int64_t );
    return call_kind(fn) ( AX(0),AX(1),AX(2),AX(3),AX(4),AX(5),AD(6),AX(7) );
}

vec4f fastcall64_8_1_192 ( void * fn, vec4f * args ) {
    using call_kind = vec4f ( * ) ( int64_t,int64_t,int64_t,int64_t,int64_t,int64_t,double,double );
    return call_kind(fn) ( AX(0),AX(1),AX(2),AX(3),AX(4),AX(5),AD(6),AD(7) );
}

vec4f fastcall64_20_1_262399 ( void * fn, vec4f * args ) {
    using call_kind = vec4f ( * ) ( double,double,double,double,double,double,double,double,int64_t,int64_t,int64_t,int64_t,int64_t,int64_t,int64_t,int64_t,int64_t,int64_t,double,int64_t );
    return call_kind(fn) ( AD(0),AD(1),AD(2),AD(3),AD(4),AD(5),AD(6),AD(7),AX(8),AX(9),AX(10),AX(11),AX(12),AX(13),AX(14),AX(15),AX(16),AX(17),AD(18),AX(19) );
}

FastCallExtraWrapper fastcall64_extra_table [] = {
    8,0,255,&fastcall64_8_0_255,
    11,0,2044,&fastcall64_11_0_2044,
    10,0,512,&fastcall64_10_0_512,
    9,0,256,&fastcall64_9_0_256,
    8,0,128,&fastcall64_8_0_128,
    7,0,126,&fastcall64_7_0_126,
    7,0,120,&fastcall64_7_0_120,
    9,0,96,&fastcall64_9_0_96,
    8,1,64,&fastcall64_8_1_64,
    8,1,192,&fastcall64_8_1_192,
    20,1,262399,&fastcall64_20_1_262399,
};

