module resource_slot shared

require daFrameGraph.daFG public
require ResourceSlotCore public

let DEFAULT_READ_PRIORITY : int = INT_MAX

struct Create
  slot : string
  resource : string

struct Update
  slot : string
  resource : string
  priority : int

struct Read
  slot : string
  priority : int = DEFAULT_READ_PRIORITY

struct SlotActions
  create : array<Create>
  update : array<Update>
  read : array<Read>

def registerAccess(var self : NameSpace; name : string; slot_decl : resource_slot::SlotActions; var declaration_callback : lambda<(slots_state : ResourceSlotCore::State; var reg : daFG::Registry) : lambda<void>>) : ResourceSlotCore::NodeHandleWithSlotsAccess

  return <- ResourceSlotCore::prepare_access <| $(var handle : ResourceSlotCore::NodeHandleWithSlotsAccess; var actionList : ActionList)
    for a in slot_decl.create
      actionList |> create(a.slot, a.resource)
    for a in slot_decl.update
      actionList |> update(a.slot, a.resource, a.priority)
    for a in slot_decl.read
      actionList |> read(a.slot, a.priority)

    // This closure saves declaration_callback inside itself
    // This closure can be called several times - declaration_callback will
    //   stay part of this lambda, inner lambda will get pointer to
    //   declaration_callback
    let declCB <- @ <|  capture(<- declaration_callback) (var node_handle : daFgCore::NodeHandle &; var slots_state : ResourceSlotCore::State &)
      let pDeclCB = unsafe(addr(declaration_callback))
      node_handle <- self |> daFG::registerNode(name) <| @(var reg : daFG::Registry)
        return <- deref(pDeclCB) |> invoke(slots_state, reg)

    handle |> ResourceSlotCore::register_access(self.nameSpaceId, name, actionList, declCB)