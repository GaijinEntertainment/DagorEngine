
macro ONE_POINT_LIGHT()
hlsl(ps) {

#define LAMBERT_LIGHT 0

#include "clustered/punctualLights.hlsl"
}
endmacro

macro ONE_LAMBERT_POINT_LIGHT()
hlsl(ps) {

#define LAMBERT_LIGHT 1
}

hlsl(ps) {
#include "clustered/punctualLights.hlsl"
}
endmacro
