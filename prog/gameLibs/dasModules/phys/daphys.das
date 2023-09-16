require Daphys

[generic]
def resolve_penetration(var position : DPoint3; var orientation : quat; contacts : array<CollisionContactData>#;
                        inv_mass : double; inv_moi : DPoint3; dt : double; use_future_contacts : bool)
  resolve_penetration(position, orientation, contacts, inv_mass, inv_moi, dt, use_future_contacts, 5, 0.009999999776, 1.f)

[generic]
def resolve_penetration(var position : DPoint3; var orientation : quat; contacts : array<CollisionContactData>#;
                        inv_mass : double; inv_moi : DPoint3; dt : double; use_future_contacts : bool; iterations : int)
  resolve_penetration(position, orientation, contacts, inv_mass, inv_moi, dt, use_future_contacts, iterations, 0.009999999776, 1.f)