# Schemeful Events (daScript, Quirrel, C++, Net)

## Advantages of Schemeful Events

- Events are universally compatible across scripting languages (daScript,
  Quirrel).
- They can be transmitted both locally and over the network.
- They can be modified in real-time without restarting the game.
- They have a strict, validated structure, making all fields visible in Quirrel
  and accessible as an instance, e.g., `evt.someField`.
- Full runtime information on the event structure (reflection) is available.
- C++ API support is provided for handling these events, if required.

## Declaring an Event

Each game directory contains an event declaration file, named in the format
`events_<game_name>.das` (e.g., `events_cuisine_royale.das`). The event
declaration consists of an annotation and a description of the event structure.
The annotation specifies whether the event is `unicast` or `broadcast`, and
network routing, if needed, which is covered below.

**Example:** `events_<game>.das`

```
[event(unicast)]
struct CmdCreateMapPoint
  x: float
  z: float
```

```{note}
All events in the file `events_<game_name>.das` are loaded before Quirrel,
enabling them to be accessed within it. Therefore, events declared for Quirrel
should be placed in this file. Although not mandatory for other events, it is
recommended for consistency.
```

## Creating an Event

- **daScript:** An event is created like any regular instance structure, e.g.,
  `[[RqUseAbility ability_type=ability_type]]`.
- **Quirrel:** Here, strict validation ensures no typographical errors or
  extraneous fields are included, `RqUseAbility({ability_type="ultimate"})`.

## Subscribing to an Event

- **daScript:** Use `on_event=RqUseAbility`, or explicitly set the type of the
  first argument in the system (e.g., `evt: RqUseAbility...`).
- **Quirrel:** Use `local {OnAbilityCanceled} = require("dasevents")...
  ::ecs.register_es("ability_canceling_es", { [OnAbilityCanceled] = @( evt, eid,
  comp) ::dlog(evt.ability_type)`.
- **C++:** Events can be listened to by subscribing to the event name, e.g.,
  `ECS_ON_EVENT(eastl::integral_constant<ecs::event_type_t,
  str_hash_fnv1("EventOnSeatOwnersChanged")>)`.

## Sending Events (Server-to-Server, Client-to-Client)

- **daScript:** Use the standard `sendEvent`, `broadcastEvent`.
- **Quirrel:** Similarly, use `::ecs.g_entity_mgr.sendEvent`,
  `::ecs.g_entity_mgr.broadcastEvent`.

## Sending Events Over the Network

When declaring an event, specify the routing to determine its network path,
e.g., `[event(unicast, routing=ROUTING_SERVER_TO_CLIENT)]`,
`ROUTING_CLIENT_TO_SERVER`, or `ROUTING_CLIENT_CONTROLLED_ENTITY_TO_SERVER`.

- **daScript:** Use `require net ... send_net_event(eid, evt)` or
  `broadcast_net_event(evt)`.
- **Quirrel:** Use `local {CmdBlinkMarker, sendNetEvent, broadcastNetEvent} =
  require("dasevents") ... sendNetEvent(eid, CmdBlinkMarker()) ...
  broadcastNetEvent(CmdBlinkMarker(...))`.

## Network Protocol Version

All declared network events contribute to the protocol version. If the server
and client versions do not match, the client will disconnect from the session.
For script events (`[event]`), you can control this behavior by excluding
certain events from protocol calculations. In cases of a mismatch, this may
result in either an on-screen error or no notification.

- `event(... net_liable=strict ...)` – The event participates in protocol
  versioning; any mismatch triggers a disconnect (default behavior).
- `event(... net_liable=logerr ...)` – The event does not affect protocol
  versioning; a log error is recorded if a mismatch occurs.
- `event(... net_liable=ignore ...)` – The event does not affect protocol
  versioning; a log warning (`logwarn`) is recorded if a mismatch occurs.

C++ events follow a similar logic but use the `NET_PROTO_VERSION` constant and
the count of network C++ events, without exceptions.

## Event Version

An explicit version can be assigned to an event. By default, all events are set
to version `0`. When working with `BitStream`, the version is required and will
assist in adapting the protocol if the stream content changes significantly.

**Example:** `code.das`

```
[event(broadcast, version=1)]
struct TestEvent {}
```

## Sending Containers (Offline and Online)

Dynamic arrays/containers can be sent along with events. Currently, the
supported types are `ecs::Object`, `ecs::IntList`, `ecs::FloatList`,
`ecs::Point3List`, and `ecs::Point4List`.

Here's an example of sending such an event from daScript:

**Example:** `code.das`

```
[event(broadcast)]
struct TestEvent
  str : string
  i : int
  obj : ecs::Object const?

...
  using() <| $(var obj : Object)
    obj |> set("foo", 1)
    broadcastEvent([[TestEvent str="test event", i = 42, obj=ecs_addr(obj)]])
```

```{important}
1. All container types in an event are stored as pointers.
2. When sending a container in an event, use the helper function
   `ecs_addr(container)`.
```

Sending events from Quirrel follows a similar process:

**Example:** `code.nut`

```nut
let {CompObject} = require("ecs")
let {TestEvent, broadcastNetEvent} = require("dasevents")
...
let obj = CompObject()
obj["foo"] = 1
broadcastNetEvent(TestEvent({str="test event", i=42, obj=obj}))
```

```{important}
- Any `ecs::Object` within an event will automatically include a field called
  `fromconnid`, which stores the sender's connection ID (on the client side,
  this is always `0`, indicating the server; on the server side, it holds the
  actual connection number).
- If the container contents undergo substantial changes, it is advisable to
  specify an event version (e.g., `[event(... version=1)]`). This will ensure
  that clients or servers with outdated versions will no longer support the
  event.
```

## Sending BitStream

Similar to containers, a raw data stream (`BitStream`) can also be sent in an
event. When sending a `BitStream`, specifying an event version is mandatory.

## Reflection

Events possess an exact schema, accessible at runtime and retrievable from any
script or C++ code.

- **C++:** All event structure information is stored in `ecs::EventsDB`, which
  provides various methods such as `getEventScheme`, `hasEventScheme`,
  `getFieldsCount` (for argument count), `getFieldOffset` (for field offset),
  `getFieldName` (for field name), `findFieldIndex` (for field index), and
  `getEventFieldValue<T>` (for direct access to parameter values).

- **daScript:** All of functions for C++ are also available in the `ecs` module
  for daScript (e.g., `events_db_getFieldsCount`). For example, the **Events
  DB** window in *ImGui* uses this API, see
  `<project_name>/prog/scripts/game/es/imgui/ecs_events_db.das`.

- **Quirrel:** A detailed event printout is available when calling `::log(evt)`,
  which outputs all event fields. An API with reflection support is also
  provided, as demonstrated below:

**Example:** `describe_event.nut`

```nut
local function describeEvent(evt) {
  if (evt == null) {
    ::dlog("null event")
    return
  }

  local eventType = evt.getType()
  local eventId = ::ecs.g_entity_mgr.getEventsDB().findEvent(eventType)

  local hasScheme = ::ecs.g_entity_mgr.getEventsDB().hasEventScheme(eventId)
  if (!hasScheme) {
    ::dlog($"event without scheme #{eventType}")
    return
  }
  local fieldsCount = ::ecs.g_entity_mgr.getEventsDB().getFieldsCount(eventId)
  ::dlog($"Event {eventType} fields count #{fieldsCount}")

  for (local i = 0; i < fieldsCount; i++)
  {
    local name = ::ecs.g_entity_mgr.getEventsDB().getFieldName(eventId, i)
    local type = ::ecs.g_entity_mgr.getEventsDB().getFieldType(eventId, i)
    local offset = ::ecs.g_entity_mgr.getEventsDB().getFieldOffset(eventId, i)
    local value = ::ecs.g_entity_mgr.getEventsDB().getEventFieldValue(evt, eventId, i)
    ::dlog($"field #{i} {name} <{type}> offset={offset} = '{value}'")
  }
}
```

## C++ Event (cpp_event)

In addition to dynamic events, it is possible to declare C++ events, for which
C++ code and SQ bindings will be generated. In Quirrel, handling these events is
identical to working with standard events, as is the case in daScript.

When declaring a C++ event, the `with_scheme` argument is required. This is
necessary because some events cannot be converted into schemeful events due to
restrictions (fields must be basic ECS types or compatible containers only).

**Example:** `events_<game>.das`

```
[cpp_event(unicast, with_scheme)]
struct EventOnPlayerDash
  from: float3
  to: float3
```

The utility `<game>/scripts/genDasevents.bat` will generate a `.h` and `.cpp`
file for this event (currently located at `prog/game/dasEvents.h/cpp`).

## Quirrel Stubs / C++ Code Generation

To generate the Quirrel stubs and C++ code automatically, run the batch file
`<game>/scripts/genDasevents.bat`. If the batch file does not work, build the
daScript compiler manually once by running `jam -sPlatform=win64
-sCheckedContainers=yes` in `<project_name>/prog/aot`.

## Filters

You can manage the list of recipients for server-side das-events using filters.
This is helpful for targeting specific groups, such as only the player or the
player's team. When sending an event, specify the filter as an additional
argument. For instance, `send_net_event(eid, [[EnableSpectator]],
target_entity_conn(eid))`. The following filters are currently supported:

- `broadcast` (default) – Sends to all recipients.
  - equivalent in C++: `&net::broadcast_rcptf`.
- `target_entity_conn` – Sends the event only to the player (the `eid` receiving
  the event must be the player's hero or player `eid`).
  - equivalent in C++: `&rcptf::entity_ctrl_conn<SomeNetMsg,
    rcptf::TARGET_ENTITY>`.
- `entity_team` – Sends the event to the player's hero and team.
  - equivalent in C++: `&rcptf::entity_team<SomeNetMsg, rcptf::TARGET_ENTITY>`.
- `possessed_and_spectated` – Sends the event to the player and any spectators
  watching them.
  - equivalent in C++: `&rcptf::possessed_and_spectated`.
- `possessed_and_spectated_player` – Similar to `possessed_and_spectated` but
  targets the player instead of the hero.
  - equivalent in C++: `&rcptf::possessed_and_spectated_player`.

In daScript, a filter is a function returning an `array<net::IConnection>`,
which is referred to as a "filter" for consistency with C++ terminology.

## Filters in Squirrel

In Squirrel, as in daScript, event-sending methods have an optional parameter
where you can pass an array of connection IDs (i.e., an array of `int`). Below
is an example filter implemented in Squirrel:

**Example:** `sq_filter.nut`

```nut
local filtered_by_team_query = ecs.SqQuery("filtered_by_team_query", {comps_ro=[["team", ecs.TYPE_INT], ["connid",ecs.TYPE_INT]], comps_rq=["player"], comps_no=["playerIsBot"]})

local function filter_connids_by_team(team){
  local connids = []
  filtered_by_team_query.perform(function(eid, comp){
    connids.append(comp["connid"])
  },"and(ne(connid,{0}), eq(team,{1}))".subst(INVALID_CONNECTION_ID, team))
  return connids
}
```

And here is an example of sending an event using this filter:

**Example:** `sq_send_event.nut`

```nut
sendNetEvent(eid, RequestNextRespawnEntity({memberEid=eid}), filter_connids_by_team(target_team))
```

## Filters in cpp_event

When annotating a `cpp_event` with the `filter=` parameter and one of the
filters listed above, the code generation process will produce C++ code that
includes the specified filter as described above in parentheses.

## Event Delivery Reliability

By default, all events are sent with a reliability level of `RELIABLE_ORDERED`.
This can be modified using the `reliability` argument. Available reliability
levels include:

- `UNRELIABLE`
- `UNRELIABLE_SEQUENCED`
- `RELIABLE_ORDERED`
- `RELIABLE_UNORDERED`

## Enums

If you need an enumerated type available in both scripts, there's no need to
write it in C++ and bind it separately for each language. The `genDasevents.bat`
utility now supports generating Squirrel code with enums directly from daScript.

**Follow these steps:**

1. Define the enum in daScript where needed (preferably in a separate file for
   easy parsing during code generation).
2. Explicitly mark the enum with the `[export_enum]` annotation.
3. Add the file path to `genDasevents.bat` with the `--module
   scripts/file_with_enum.das` argument.
4. Run `genDasevents.bat`.
5. Constructors for all enums will be available in
   `<game_prog>/sq_globals/dasenums.nut`.

## Utilities

- `ecs.dump_events` – This console command prints all events, their schemas, and
  schema hashes. If there are mismatches between client and server events, this
  command can be run on both to compare outputs (the log will already contain
  all necessary information for analysis).
- Additionally, there's an in-game window with detailed event information: open
  the **ImGui menu** (`F2`) ▸ **Window** ▸ **ECS** ▸ **Events db**.

<img src="_images/schemeful_events_01.png" alt="Events db" align="center">

<br>

## FAQ

###### I have a C++ network event and want to move its declaration to daScript while keeping the event in C++. (Example: `ECS_REGISTER_NET_EVENT(EventUserMarkDisabled, net::Er::Unicast, net::ROUTING_SERVER_TO_CLIENT, (&rcptf::entity_ctrl_conn<EventUserMarkDisabledNetMsg, rcptf::TARGET_ENTITY>));`

Define the event in daScript with the `[cpp_event(unicast, with_scheme,
routing=ROUTING_SERVER_TO_CLIENT, filter=direct_connection)]` annotation, then
run `genDasevents.bat`. This will generate stubs, and the event will appear or
update in the `.h` and `.cpp` files. (`cpp_event + with_scheme` activates code
generation).

---

###### I have a C++ network event and want to move it entirely to scripts (no need for it in C++).

Follow the same steps as above, but use `[event(unicast,
routing=ROUTING_SERVER_TO_CLIENT)]`. Replace all `sendEvent` calls with
`send_net_event/sendNetEvent`, passing a filter function call like
`target_entity_conn(eid)` as the final argument. Running `genDasevents.bat` will
still be necessary to generate the stubs.

---

###### I have a script-based event and need to migrate it to C++.

Simply change the event annotation from `event` to `cpp_event`. Then, replace
all `send_net_event` calls with standard `sendEvent` calls. Run
`genDasevents.bat` to generate the stubs and C++ code.

---

###### I added an event, but I see the following error in Squirrel: `[E] daRg: the index 'CmdHeroSpeech' does not exist`.

Make sure the event is registered in the system before Quirrel loads. Each game
has an initialization script (e.g., `<game>_init.das`). Load the script
containing the event in this initialization script to resolve the error.

---

###### `genDasevents.bat` shows compilation errors and won't run.

Rebuild the compiler.

```{seealso}
For more information, see [daScript plugin for
VSCode](https://marketplace.visualstudio.com/items?itemName=profelis.dascript-plugin).
```


