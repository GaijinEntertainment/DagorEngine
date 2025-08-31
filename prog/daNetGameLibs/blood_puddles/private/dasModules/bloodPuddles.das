require BloodPuddles
options indenting = 2

[generic]
def addPuddleAt(var mgr : BloodPuddles;
                pctx : PuddleCtx#;
                group : int = int(BloodPuddlesGroup.BLOOD_DECAL_GROUP_PUDDLE))
  mgr |> __addPuddleAt(pctx, group)


[generic]
def put_decal(var mgr : BloodPuddles;
              var group : int;
              pos : float3;
              normal : float3;
              var size : float;
              var riDesc : riex_handle_t;
              hitPos : float3;
              var projective : bool;
              var texture_variant : int = INVALID_VARIANT;
              var strenght : float = 1.0)
  let dir = float3(0.0, 0.0, 0.0)
  mgr |> __put_decal(group, pos, normal, dir, size, riDesc, hitPos, projective, texture_variant, strenght)

[generic]
def put_decal(var mgr : BloodPuddles;
              var group : int;
              pos : float3;
              normal : float3;
              dir : float3;
              var size : float;
              var riDesc : riex_handle_t;
              hitPos : float3;
              var projective : bool;
              var texture_variant : int = INVALID_VARIANT;
              var strenght : float = 1.0)
  mgr |> __put_decal(group, pos, normal, dir, size, riDesc, hitPos, projective, texture_variant, strenght)
