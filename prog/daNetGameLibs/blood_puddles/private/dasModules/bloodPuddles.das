require BloodPuddles

[generic]
def addPuddleAt(var mgr : BloodPuddles;
                pctx : PuddleCtx#;
                group : int = int(BloodPuddlesGroup BLOOD_DECAL_GROUP_PUDDLE))
  mgr |> __addPuddleAt(pctx, group)