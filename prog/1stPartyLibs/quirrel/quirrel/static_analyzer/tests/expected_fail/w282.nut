//expect:w282

::fn <- require("sq3_sa_test").fn
::addInviteByContact <- require("sq3_sa_test").addInviteByContact
::Contact <- require("sq3_sa_test").Contact


local invite_info = ::fn()
if (local uid := invite_info?.leader.id.tostring()!=null)
  ::addInviteByContact(::Contact(uid))
