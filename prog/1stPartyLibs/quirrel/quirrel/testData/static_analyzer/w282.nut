local invite_info = ::fn()
local uid = 0
if (uid := invite_info?.leader.id.tostring()!=null)
  ::addInviteByContact(::Contact(uid))

local a = ::fn()
  while (a := ::fn() || ::fn2())
    print(a)