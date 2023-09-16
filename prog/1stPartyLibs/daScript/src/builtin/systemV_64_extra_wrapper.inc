vec4f fastcall64_8_1_255 ( void * fn, vec4f * args ) {
    using call_kind = vec4f ( * ) ( double,double,double,double,double,double,double,double );
    return call_kind(fn) ( AD(0),AD(1),AD(2),AD(3),AD(4),AD(5),AD(6),AD(7) );
}

vec4f fastcall64_11_1_2044 ( void * fn, vec4f * args ) {
    using call_kind = vec4f ( * ) ( int64_t,int64_t,double,double,double,double,double,double,double,double,double );
    return call_kind(fn) ( AX(0),AX(1),AD(2),AD(3),AD(4),AD(5),AD(6),AD(7),AD(8),AD(9),AD(10) );
}

vec4f fastcall64_10_1_512 ( void * fn, vec4f * args ) {
    using call_kind = vec4f ( * ) ( int64_t,int64_t,int64_t,int64_t,int64_t,int64_t,int64_t,int64_t,int64_t,double );
    return call_kind(fn) ( AX(0),AX(1),AX(2),AX(3),AX(4),AX(5),AX(6),AX(7),AX(8),AD(9) );
}

vec4f fastcall64_9_1_256 ( void * fn, vec4f * args ) {
    using call_kind = vec4f ( * ) ( int64_t,int64_t,int64_t,int64_t,int64_t,int64_t,int64_t,int64_t,double );
    return call_kind(fn) ( AX(0),AX(1),AX(2),AX(3),AX(4),AX(5),AX(6),AX(7),AD(8) );
}

vec4f fastcall64_8_1_128 ( void * fn, vec4f * args ) {
    using call_kind = vec4f ( * ) ( int64_t,int64_t,int64_t,int64_t,int64_t,int64_t,int64_t,double );
    return call_kind(fn) ( AX(0),AX(1),AX(2),AX(3),AX(4),AX(5),AX(6),AD(7) );
}

vec4f fastcall64_13_1_8193 ( void * fn, vec4f * args ) {
    using call_kind = vec4f ( * ) ( double,int64_t,int64_t,int64_t,int64_t,int64_t,int64_t,int64_t,int64_t,int64_t,int64_t,int64_t,int64_t );
    return call_kind(fn) ( AD(0),AX(1),AX(2),AX(3),AX(4),AX(5),AX(6),AX(7),AX(8),AX(9),AX(10),AX(11),AX(12) );
}

vec4f fastcall64_13_0_8193 ( void * fn, vec4f * args ) {
    using call_kind = int64_t ( * ) ( double,int64_t,int64_t,int64_t,int64_t,int64_t,int64_t,int64_t,int64_t,int64_t,int64_t,int64_t,int64_t );
    return Rx ( call_kind(fn) ( AD(0),AX(1),AX(2),AX(3),AX(4),AX(5),AX(6),AX(7),AX(8),AX(9),AX(10),AX(11),AX(12) ) );
}

FastCallExtraWrapper fastcall64_extra_table [] = {
    8,1,255,&fastcall64_8_1_255,
    11,1,2044,&fastcall64_11_1_2044,
    10,1,512,&fastcall64_10_1_512,
    9,1,256,&fastcall64_9_1_256,
    8,1,128,&fastcall64_8_1_128,
    13,1,8193,&fastcall64_13_1_8193,
    13,0,8193,&fastcall64_13_0_8193,
};

