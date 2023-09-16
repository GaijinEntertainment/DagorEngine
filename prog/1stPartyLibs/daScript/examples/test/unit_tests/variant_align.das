typedef ResNameId = uint

struct ResUid
    nameId : ResNameId
    history : bool

variant VirtualAttachmentResource
    resUid : ResUid
    name : string

struct VirtualAttachmentRequest
    resource : VirtualAttachmentResource
    mipLevel : uint16
    layer : uint16

[export]
def test
    var t = typeinfo(sizeof type<VirtualAttachmentRequest>)
    t = t
    return true

