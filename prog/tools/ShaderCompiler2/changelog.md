* [Removed] support of `(code)hlsl` syntax. Use `hlsl(code)` instead
  Change-Id: I6b667f1f4282605b1470ae25c1cca08f9508b731
  Change-Id: I0415088e3b2e6bb1ccaa5f94a8a126a955b4043e

* [Added] support for if-expression in constant binding block
```
(ps) {
  foo@f4 = foo;
  if (shader == some_shader) {
    bar@f4 = bar;
  }
}
```
  Change-Id: I68b1a1ee74ceed5f10d146cc06f4b2fb6dc5e6b8

* [Added] support for bool variable
```
bool use_wsao = shader == ssao && gi_quality == only_ao;
(ps) {
  if (use_wsao) {
    foo@f4 = foo;
  }
}
hlsl(ps) {
  ##if use_wsao
    return foo;
  ##endif
}
```
  Change-Id: I3d96e4f421888a8fafb9db8a3c0c7f023039ae5b

* [Added] `maybe` intrinsic
```
if (wsao_tex != NULL) {
  if (shader == ssao) {
    bool use_wsao = true;
  }
}
(ps) {
  if (maybe(use_wsao)) {
    foo@f4 = foo;
  }
}
```
  Change-Id: I3c193d60f2e92eacf5fae9cb2a47f3223699e61b

* [Added] `error` intrinsic
```
if (gi_quality == only_ao) {
  // ..
} else if (gi_quality == high) {
  // ..
} else {
  error("Unimplemented gi quality");
}
```
  Change-Id: I19a1b66b616b574bfd9b654ac0594477b6d16a6a

* [Removed] `##aliase`, `##static_ifdef` and `##static_def`
Instead of static_ifdef, you can use the usual HLSL #define. Or, if you don't want to see a false branch, then you can use `bool` variables
  Change-Id: I3e588fe69764edb5b8fdff5c630ccb5fd74448c9

* [Added] support for hardcoded register bindings
```
int reg_no = 3;
shader sh {
  (ps) {
    foo_vec@f4 : register(reg_no);
    foo_tex@smp2d : register(reg_no);
    foo_buf@buf : register(reg_no) hlsl { StructuredBuffer<uint> foo_buf@buf; };
    foo_uav@uav : register(reg_no) hlsl { RWStructuredBuffer<uint> foo_uav@uav; };
  }
}
```
  Change-Id: Id2cc68e2fc2f390ca9417e0f426a605bb98b1956

* [Added] support for assert in shaders
```
hlsl(ps) {
  ##if DEBUG
    uint2 dim;
    tex.GetDimensions(dim.x, dim.y);
    ##assert(all(tc < dim), "Out of bounds");
  ##endif
}
```
  Change-Id: I7e813bf57ecd95081e73ef13d18937fef9c0708f

* [Removed] support '|' and '&' in conditional expressions
  Change-Id: I008e0650f35417f85173b0c79fb32a44460a7764

* [Removed] support of `color4` and `real` types
  Instead, use `float4` and `float` types
  Change-Id: Iba88c68c7f1a65b648461fbf9b10326a400f346d

* [Removed] `use` support
  Change-Id: I17cf07f8c8939fa17e44559f322c2e6b768ac253

* [Added] direct initialization of static variables
```
shader sh {
  float4 foo = material.diffuse;
  texture bar = material.texture[1];
}
```
  Change-Id: Ie1c2572c4f58c72588aac081ee94ce7e0e7ddffb

* [Removed] `init` statement support
  Use direct initialization of static variables
  Change-Id: I678bf454403e05addeb9cbb2fbeb0fd8bf6906b3

* [Added] `get_viewport` intrinsic
```
float4(top_left_x, top_left_y, width, height) = get_viewport();
```
  Change-Id: I74e2fac08503c9998b37d4cdabe73d3e257a0998
