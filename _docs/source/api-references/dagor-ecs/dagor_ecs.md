# Dagor ECS

## Basic concepts

**Entity:** An entity is a set of components created based on a template using
an initializer (a data set) for each component. It is important to note that an
entity is *not* a container; components are not stored "within" it.

**EntityId:** This is the identifier for an entity, acting as a weak reference
with a limited number of reuse cycles (generation).

**Component:** Essentially, a component is a pure data class without any code
and behaviour description, which is instead contained in ECS systems. While
components can be written in an Object-Oriented Programming (OOP) style, like
legacy (e.g., C with classes but without polymorphism), the methods should only
be invoked from ECS systems that receive components of the same type as
parameter. Components may include a copy constructor, comparison operator or/and
assignment operator, but at a minimum, they must have a constructor and
destructor, with additional elements necessary for serialization and tracking of
changes.

**Templates:** The only way to create an entity is to specify a fully data-driven
template, which lists all the components of the entity (they might be assigned
to some default values). An optional initializer can be provided to
override these default values within the entity.

**Archetype:** A specific list of components defines an archetype. All
components of entities sharing the same archetype are stored optimally in memory
(Structs of Arrays, SoA). Multiple templates can correspond to the same
archetype. Direct interaction with archetypes is not possiblle, as they are
framework entities, but it is useful to understand which archetype your template
corresponds to.

**ComponentTypeManager:** This object manages the lifecycle (i.e., creation and
destruction) of non-PoD (Plain old Data) types, such as `visual_effect`,
`animchar`, `human_phys_actor`, etc. This is a table of functions, which
implement constructor, destructor, copy-constructor, comparison operator,
assignment operator, move operator, replicateCompare (comparison + assignment).
Only constructor and destructor would be essential. Copy constructor is required
for creating components with an initializer or template with data, but not
required when a template doesn’t contain data (only description is present).
Comparison / assignment operators are required for serialization and tracked
changes. Thus, when serialization and tracking are not needed operators can be
omitted.

```{important}
In 99.9% cases ComponentTypeManager implementation is not needed. You should
only identify if the specified data type is relocatable or not (i.e. does
anything else contain a pointer to it), and declare it with standard macro
`ECS_DECLARE_BOXED_TYPE` (or `ECS_DECLARE_RELOCATABLE_TYPE`). Relocatable type
is faster and ensures better locality. It cannot be used if a data type (e.g.
`fixed_vector`) stores a pointer to itself (or if size of type is greater than
65536 bytes, which, hopefully, will never happen).
```

**System**: Pure functions, which work with a predefined list of components:

- `onUpdate`. Pure function that is invoked each frame. Essentially, it is
BroadCast Event Immediate, but it is optimized for the case when there are a lot
of ES listeners of this Event (Stage).
- `onEvent`. Usually works with a specific Entity (the event is sent to a
particular entity). However, there could be BroadCast Events as well. `onEvent`
receives not only components tuples, but also a typed Event. Event can be
regularly postponed (and, in fact, it will be), but then it will be processed by
all of the relevant systems at once.
- `performQuery`. Pure function, which is invoked inside another function (see
below). Both BroadCast Query and `onUpdate` are queries + one parameter
(Event/Stage).

**ChildComponent:** The component that is not a part of Entity. Initializers
(for creation) or “child” components in Object (a table) or Array.

**Query:** A fast and convenient way of getting all necessary entities with
specified attributes (Components). That is to say, a query like `Get_All_Humans`
or `Get_All_Humans_With_Transform`. In fact, simple systems perform function on
all Query results for Stage/Event.

**EntityManager:** This represents "the whole world" in the ECS context.

**Mutation of Entity Archetype:** (a change in Entity Components). Archetypes
can only be mutated via `reCreateEntity`, which allows transitioning to a new
template by adding missing components and removing unnecessary ones from the old
template. If the new template corresponds to the same archetype, the operation
would be empty (no change).

**Tracked/Replicated:** Dagor ECS can automatically track changes and trigger
events and/or replicate objects over a network. To enable this, entity
components need to be marked appropriately in the template. Non-PoD types must
implement comparison and assignment operators.

There is an "optimized" variant of `replicateCompare(from, to)` (by default,
it’s `if(!(from == to)) {from = to; return true; } else return false;)`. If the
data type is rather complicated and large, but all changes in the object of this
type are securely encapsulated via "set" and "get" (mutable), then
generation/hash logic can be used for more optimal comparison. If the generation
hasn’t changed, then data hasn’t changed too. Example: `ecs::Objects`,
`ecs::Array`.

**Replication:** Changes are replicated using an "eventually consistent" model,
meaning not all changes will be "visible" to the client in the exact order of
their appearance the server (some could be even missed), but all changes will
eventually be received by the clients. It is done in order to not resend
"obsolete" data, if the packet containing replication has been lost, while
relevant components have been changed.

## Characteristics

- **Components are Code-Free:** Components do not contain any code. They are not
  OOP objects, as encapsulation, inheritance, and polymorphism contradict the
  principles of ECS (Entity-Component-System). Methods, aside from getters and
  mathematical operators, are also not present. (Refer to this
  [video](https://youtu.be/QM1iUe6IofM) for more details.)

- **Systems (`onUpdate`/`onEvent`) Are Data-Free:** Systems do not store or modify
  data, including global or static variables (except for configuration data).
  Systems operate exclusively on predefined data registered in advance. If an
  Entity lacks the required components, it will not be processed by the System.
  Conversely, if it contains the required components, it will be processed.

  However, the System remains unaware of any other components the Entity might
  contain and cannot interact with them. An exception exists when working with
  components of other Entities, which must be explicitly declared during
  registration. The System is responsible for transforming the world from one
  consistent state to another and cannot rely on the existence or behavior of
  other systems.

- **Deferred Creation and Destruction of Entities:** Entities are created and
  destructed in a deferred manner, but no later than the end of the update
  cycle.

- **Deferred onEvent Execution:** The execution of `onEvent` is deferred, with
  an unknown number of frames delay.

- **System (`onUpdate`) Executes All at Once:** All Systems (`onUpdate`) are
  executed simultaneously. The execution order of Systems (before/after) is
  specified during ES registration and is subject to topological sorting.

- **Optimal Memory Storage for Components:** Component data is stored in memory
  optimally (Structure of Arrays, SoA), except for Boxed components. Boxed
  components have optimization in the standard BoxedCreator template, which
  allows sequential memory allocation, though fragmentation can still occur.

- **Complex Systems Using Multiple Jobs/Queries:** You can write Systems
  consisting of multiple Jobs/Queries. For this, the wrapping System is defined as
  accepting no components, and Queries (of any required number and types) are
  described within it.

- **Multiple Invocations of Simple Systems and Queries:** Each simple System
  (and each Query) can and will be invoked multiple times — once for each data
  chunk of every Archetype that meets the requirements.

- **Parallelism:** The execution of any `onUpdate` or broadcast `onEvent` can be
  parallelized. This can happen per System (executing the code of a single
  system across multiple threads, classic SIMD) or by executing multiple Systems
  simultaneously.

  Since System(`onUpdate`)/`onEvent` explicitly declares which components are
  read-only, and which are read-write during registration, the EntityManager can
  accurately determine if there will be any Read/Write conflicts for any set of
  Systems, allowing parallel execution provided no other ECS-related code is
  invoked. However, this parallelism is not yet supported. SIMD execution within
  a single system is possible but extremely dangerous and undesirable unless you
  fully understand what you are doing.

  In *daNetGame-based* games this is specified in a query during declaration as

  ```
  `template<typename Callable> void animchar_update_ecs_query(Callable ECS_CAN_PARALLEL_FOR(c, 4));
  ```

  and for ES as

  ```
  void animchar_update_es(const UpdateStageInfo &, EntityId eid, Callable ECS_CAN_PARALLEL_FOR(c, 4));
  ```
  or in `es_order` as

  ```
  `es_name {mt{stage_name:i=quant_size;}}
  ```

  where
  `stage_name` could be `es_act`, `es_before_render`, or others, and
  `quant_size` is the minimum "quantum" of work (i.e., the minimum number of
  tuples to be processed). This quantum size depends on the system: the larger
  it is, the better (fewer switches), but if the system only needs to process 10
  tuples and the quantum is 10, there will be no parallelism. Generally, a good
  quantum size is around "expected number of tuples/16" (so each of 4 threads
  gets 4 chunks of work). However, if the work chunks are uneven in terms of
  time (animations can be very simple or very complex), it's better to keep the
  quantum small, such as 4 elements.

  ```{caution}
  Do not use parallelism except in a small amount of isolated, slow code.
  ```

- **Deferred `onEvent` Execution Based on Budget:** The execution of any
  `onEvent` can be deferred by an arbitrary number of frames depending on
  available resources ("budget"). Unfortunately, the budget is not in
  milliseconds (because measuring that is slow) but in Events. Therefore, it is
  crucial that a single `onEvent` does not cause significant delays.

## Working Directly

- **Working with Components Directly:** Direct manipulation of components is
  allowed only under one condition — it must not occur inside
  `onUpdate`/`onEvent`.

  If it does happen inside `onUpdate`/`onEvent` (for example, a trigger zone is
  checking whether someone has entered or not), this must be explicitly declared
  (via a Query).

  Such a system cannot be parallelized, either entirely (if it
  potentially (re)writes to the entire world), with specific other systems (if it
  explicitly declares which components it writes to), or it may only be
  parallelized within itself (if it reads from the entire world).

- **Avoid Get/Set:** In general, it is advisable to avoid using `get`/`set`
  methods.

## Entity States

When writing network code, especially when dealing with an external Entity Id
(not included in a query/es) through get/set operations (which should be avoided
as much as possible), it is crucial to understand that an entity can exist in
one of four primary states during its lifecycle:

1. **Non-Existent:** The entity does not exist.
2. **Exists but Empty (aka Allocated Handle):** The entity exists but does not
   have any components. This can occur on the client side when there are
   references to the entity, but it has not yet been created by the server.
3. **Exists and Empty, but Queued for Loading:** The entity exists, is empty,
   but is queued for loading. This happens when an entity is created
   asynchronously.
4. **Exists and Created:** The entity exists, and all required components are
   present and loaded.

For each of these states, there is a corresponding check (which may vary
depending on the ECS version but is always present).

There are also more exotic states, such as an entity that has been
created/loaded but is missing some components because the entity is a composite
created through recreation (e.g., "base+foo+bar", where "foo" has not yet been
added).

To avoid problems, instead of using `get`/`getRW` (which, in dev builds, will
assert in such cases but in release builds will still return a reference to
empty memory), use `getNullable`/g`etNullableRW`. An entirely empty entity will
return an "empty" template name.

## Legacy (OOP)

If you have an OOP object that you want to turn into a component, this does not
inherently violate the ECS paradigm. However, all the code (object methods)
should only be invoked within an update.

Essentially, this is just about moving shared code for several
`onUpdate`/`onEvent` into a common location.

However, the recommended approach is to extract such common code into a separate
shared library rather than leaving it as a class method to clearly distance it
from OOP. If the OOP object is polymorphic, this should not be used at all (and
it's better not to do this).

## Difference in Approaches

Let’s say you want to create triggers (e.g., something happens when entering a
trigger). Previously, you had a spherical trigger, and now you need a cubic one
as well.

**OOP Approach:**

```cpp
class IShapedTrigger {
  virtual bool isInside(const Point3 &p) = 0;
}

class BoxTrigger : public IShapedTrigger {
  bool isInside(const Point3&) override;
}

class SphereTrigger : public IShapedTrigger {
  bool isInside(const Point3&) override;
}

// Update code:
IShapedTrigger *trigger;
foreach (unit) {
  if(trigger->isInside(unit.pos)) {
    do_something();
  }
}
```

**ECS Approach:**

```cpp
void sphere_trigger_es(int trigger, const Point3 &sphere_c, const float sphere_r) {
  performQuery([&](Unit &unit) {
    if (length(sphere_c - unit.pos) < sphere_r) {
      sendEvent(IN_TRIGGER, Event(unit, trigger));
    }
  });
}

void box_trigger_es(int trigger, const Point3 &box_0, const Point3 &box_1) {
  performQuery([&](Unit &unit) {
    if (unit.pos & BBox3(box_0, box_1)) {
      sendEvent(IN_TRIGGER, Event(unit, trigger));
    }
  });
}

onEvent(int trigger, Unit &unit) {
  do_something();
}
```

As you can see, both approaches are possible, but:

1. **Deferred Execution:** `do_something` can be deferred in ECS, which is
   beneficial rather than problematic.

2. **Decoupling:** The `onEvent` system has no knowledge of what triggered the
   event. It could be a spherical or cubic area, or even a special ability of a
   Unit. The system doesn't care and doesn't need to know. This promotes
   decoupling.

3. **Debugging and Serialization:** The entire process can be fully debugged
   graphically, serialized, and sent over the network as both an event and its
   result. This allows transfer necessary calculations to the client (e.g.,
   graphical effects) without using `if` statements.

4. **Extensibility:** The IShapeArea interface can be extended as broadly as
   needed. A unit can be in multiple zones simultaneously if required.

5. **Less Code:** Although it may seem that the amount of code is similar, the
   OOP approach lacks members (e.g., box, sphere), the actual function
   implementations, and instance creation code. The ECS version contains all the
   necessary code (excluding what is generated by codegen).

## Performance

- **Typical Entity Characteristics:** In most game worlds, typical entities have
  at least 5-10 components, and sometimes even several hundred, consuming
  hundreds of bytes to tens of kilobytes of memory.

- **Component Characteristics:** Components (members) are typically diverse, and
  rarely does a single method interact with all components simultaneously. Thus,
  OOP often results in patterns that are highly suboptimal in terms of cache
  efficiency and branch prediction.

- **Polymorphism in Entities:** Real-world entities often exhibit some form of
  polymorphism, meaning multiple entities perform the same behavior on
  semantically similar data.

- **ECS vs. OOP:** ECS provides more efficient access to entities and their data
  (via data duck typing) than classical OOP, but how does it fare in terms of
  performance?

- **DagorECS - Data-Oriented and Fast:** Dagor ECS is designed with a
  data-oriented approach, it features high speed and excellent cache locality.


### Entity Creation

- **Framework Overhead:** The overhead of the framework during entity creation
  is negligible.

- **Batch Creation:** Creating many entities (even with initializers) is
  generally faster than creating a `vector<Entity>` and adding them one by one.
  It is slightly (~40%) slower than creating all entities at once (a speed that
  is not achievable in real-world tasks, as it represents an upper limit) and
  slightly less (~30%) slower than using `vector<unique_ptr<Entity>>` (which, as
  we'll see, incurs a significant performance penalty).

Here are some measurements for creating 30,000 entities, each consisting of 15
POD components, with a total entity size of 516 bytes (one component is tracked,
no tracked means the component is not tracked):

```cpp
struct Entity {
  TMatrix transform = {TMatrix::IDENT};
  int iv = 10, ic2 = 10;
  Point3 p = {1,0,0};
  TMatrix d[9] = {TMatrix::IDENT};
  Point3 v = {1,0,0};
  int ivCopy = 10;
};
```

**Time to create 30,000 Entities:**

```
- daECS create (no tracked):                 6788 us
- grow vector create:                       10088 us
- best possible (single allocation) create:  4768 us
```

**Average time for mass creation:** `0.22 microseconds` per entity. This is the
synchronous creation time without any Event Systems (ES) catching creation
events for these entities.

**Conclusion:** Entity creation in daECS is very fast! There is no need to
implement spawn pools or other such complexities.

### Data Handling/Frame Update

To evaluate this, we use the same entity and a trivial kinematic update: `pos +=
dt * vel`.

**Times marked as "best possible" represent the maximum achievable speeds for a
data-oriented design, utilizing two parallel arrays (NOT entities).**

**With a "cold" cache:**

```
- daECS update:                     49.45 us
- vector<Entity>, inline:          460.20 us
- vector<Entity*>, inline:         502.40 us
- vector<Entity*>, virtual update: 683.45 us
- best possible, inline:            44.55 us
```

**With a "hot" cache:**

```
- daECS update:                     35.7 us
- vector<Entity>, inline:          299.8 us
- vector<Entity*>, inline:         346.0 us
- vector<Entity*>, virtual update: 561.8 us
- best possible:                    34.7 us
```

**Performance Summary:** Despite the vastly greater convenience of the ECS
framework, it is more than 10 times faster than working with OOP entities in any
form (and especially faster than OOP with polymorphism).

**Optimal Speeds:** The best achievable speeds are no more than 10% better with
a "cold" cache and practically identical with a "hot" cache.

**Comparison with Other Frameworks:** For comparison with another well-known
data-oriented framework, Unity 2018 ECS: we have an exact implementation of the
Unity algorithm, and daECS performs approximately 4-8 times faster.

**Conclusion:** Working with daECS is incredibly fast!

## Codegen

We have a *codegen* (code generation) tool that automatically generates
standardized bindings for system registration using "lambda" that process one
entity (tuple of components).

This is a Python script that parses files to identify functions containing the
following patterns:

- `*_es`
- `*_es_event_handler`
- `*_ecs_query`

It then registers the system with the corresponding name. For example, functions
like `water_es(UpdateStageInfoAct)` and `water_es(UpdateStageInfoRender)` or
`water_es_event_handler(Event1)`/`water_es_event_handler(Event2)` would be
registered as two separate systems with their own set of components.

The *codegen* recognizes typed `on_update` functions (where the first parameter
is the stage, with a specified type) and typed `event_handler` functions.

**Parameter Binding:**

- **Strict Name Binding:** All parameters are bound strictly by name. Names
  cannot be omitted! If a name is required for behavior but the data itself is
  not needed, use `ECS_REQUIRE(type name)`.

  For parameters listed in `ECS_REQUIRE` you can use `ecs::auto_type` (this is
  not an actual type but serves to avoid unnecessary includes, etc.).

- **Optional Parameters:** These either have a default value (e.g., `float
  water_wind = 1.0f`) or are passed via pointer (e.g., `float *water_wind`). If
  the parameter is optional, entities that lack such a parameter will be processed
  with the default parameter (or `nullptr` if it’s a pointer).

- **Implicit Component:** The `eid` component, of type `ecs::EntityId`, is
  implicitly added to all templates.

**Codegen Benefits:**

All of this generated code is human-readable, meaning you can review and even
write it manually (though this is not recommended). *Codegen* makes the code
easier to read, reduces manual effort (and the errors that come with it), and
enables faster refactoring of selected ECS framework APIs.

**ECS Directives:**

- `ECS_TRACK(name1, name2)`: The system will "track" changes to components
  `name1` and `name2`.
- `ECS_BEFORE(name1, name2) ECS_AFTER(name3)`: The system will execute before
  `name1` and `name2`, but after `name3`. `nameX` can refer to the names of
  other systems or synchronization points listed in `es_order`. Any system not
  specified to run before `__first_sync_point` will always run after it.
- `ECS_NO_ORDER`: The execution order of this system is not important.
- `ECS_TAG(render, sound)`: The system will only run if the program is
  configured with these tags (e.g., `server` for server-side, `gameClient` for
  client-side, etc.).
- `ECS_ON_EVENT(on_appear)`: Which is a shortcut for
  `ECS_ON_EVENT(ecs::EventEntityCreated, ecs::EventComponentsAppear)`
- `ECS_ON_EVENT(on_disappear)`: Which is a shortcut for
  `ECS_ON_EVENT(ecs::EventEntityDestroyed, ecs::EventComponentsDisappear)`.
- `ECS_REQUIRE`, `ECS_REQUIRE_NOT`: These macros for *codegen* should be placed
  directly before the ES function. For example:

  ```cpp
  ECS_REQUIRE(int someName) ECS_REQUIRE_NOT(ecs::auto_type SomeAbsentName)
  void foo_es(float hp)
  {
      // This function will only receive `hp` for entities that have the `someName` component (of type `int`)
      // but do not have the `SomeAbsentName` component (type irrelevant).
  }
  ```

  ```{important}
  `ECS_REQUIRE` works via the annotate attribute, which unfortunately does not
  annotate literals — only names (of arguments or functions).
  ```

  Therefore, the following will not work:

  ```cpp
  void foo(int some_component = 1 ECS_REQUIRE(ecs::Tag some_tag)) // This won't work
  ```

  However, this will work:

  ```cpp
  ECS_REQUIRE(ecs::Tag some_tag)) void foo(int some_component = 1)
  ```

- `ECS_ON_EVENT(EventName, EventName2, ...)`: If the handler body needs to
  perform the same operation for different events, use `ECS_ON_EVENT` to avoid
  copy-pasting.

**ECS core events:**

Unicast events:

- `EventEntityCreated`: Sent after entity was fully created & loaded. Sent only once.
- `EventEntityRecreated`: As previous, but might be sent several times. This
  event takes place after reCreateEntity calls.
- `EventComponentsDisappear`: Event is called on recreate if this ES will no
  longer apply (i.e. list of components no longer matches).
- `EventComponentsAppear`: Event is called on recreate if this ES will start
  apply (i.e. list of components matches).
- `EventEntityDestroyed`: Sent before entity destruction.
- `EventComponentChanged`: Sent after existing component is changed.

  ```{important}
  This is a unique, optimized event. It is triggered on the Entity System (ES)
  only if the ES requires the component that has been modified. Unlike other
  events, this event will not be received by an ES even if it matches the list
  of components unless the modified component is included in the ES's list of
  components.
  ```

Broadcast events:

- `EventEntityManagerEsOrderSet`: Sent after es order has been set.
- `EventEntityManagerBeforeClear`, `EventEntityManagerAfterClear`: Sent
  before&after all scene (all entities) destruction.

## Code Example

```
rect
{
    pos:p2 = 0,0
    rectSize:p2 = 0,0
    color:c=255,255,255,255
}

brick
{
    _extends:t="rect"
    brick.pos:ip2 = 0,0
}

pad
{
    _extends:t="rect"
    isPad:b=yes
}

ball
{
    pos:p2 = 600,700
    vel:p2 = 330,-550
    radius:r = 5
    color:c=25,255,255,255
}
```

## Video Lecture

Anton Yudintsev's video lecture on ECS:

- [Gameplay-Architecture-and-Design.mp4,
3.65 GB](https://drive.google.com/drive/folders/14Uj5yKAjt6ZgpOOr1q-cG417G_YE-MHg?usp=sharing)


