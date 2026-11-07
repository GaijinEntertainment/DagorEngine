Stateful Components
-------------------

Use ``StatefulComp`` when a component needs state of its own: a ``Watched``,
a ``Computed``, or an FRP subscription that should live as long as that one
component is on screen.

A normal builder can run again whenever its parent rebuilds.
State created in that builder is created again too.
A stateful component has a constructor that daRg runs once when
the component is mounted to the scene.
The constructor receives a ``scope``, creates the per-instance state through
it, and returns the usual component builder.

Creating a stateful component
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Create the component type once, normally at module scope.
Calling the type (created by `StatefulComp()`) creates a descriptor;
it does not call the constructor yet.
Put that descriptor in ``children``.

.. code-block:: quirrel

   let selectedSquadId = Watched(null)

   let SquadCard = StatefulComp(
     function(scope, squad) {
       let isSelected = scope.Computed(@() selectedSquadId.get() == squad.get().id)

       return @() {
         watch = [squad, isSelected]
         rendObj = ROBJ_SOLID
         behavior = Behaviors.Button
         onClick = @() selectedSquadId.set(squad.get().id)
         children = {
           rendObj = ROBJ_TEXT
           text = isSelected.get() ? $"Selected: {squad.get().name}" : squad.get().name
         }
       }
     },
     @(squad) squad.id)

   let squadList = @() {
     watch = squads
     children = squads.get().map(@(squad) SquadCard(squad))
   }

``StatefulComp(...)`` returns a component type. Its identity is part of a
component's identity, so do not create that type inside a builder.

The constructor's first parameter must be named ``scope``, or ``_scope`` /
``_`` when the constructor does not use it. The scope is the owner of the
per-instance FRP state:

- ``scope.Watched(value)`` and ``scope.Computed(fn)`` create observables that
  daRg releases when the instance is deleted.
- ``scope.subscribe(obs, callback)`` subscribes to any observable; daRg removes
  the callback when the element is detached. ``obs`` itself is kept.
- ``scope.onDetach(callback)`` runs ``callback`` once when the element is
  detached, to release a resource that FRP does not own (a timer, a native
  listener). Register several; they run in registration order. The scope is
  tearing down inside the callback, so it must not create state or subscribe.
- ``scope.WatchedImmediate(value)`` and ``scope.ComputedImmediate(fn)`` create
  observables whose subscribers run synchronously inside ``set()`` instead of
  at the deferred FRP flush in ``update()``. Values are always fresh on read
  either way, so treat these as a last resort. ``ComputedImmediate`` also
  makes all of the computed's upstream observables propagate synchronously,
  shared ones included.

A plain ``Watched`` or ``Computed`` made in the constructor is not owned by the
instance; it lives as long as something references it. Use the plain form for
state that is deliberately shared or module-scope.

``SquadCard(squad)`` returns a lightweight descriptor.
It holds the type, the arguments, and the key.
The key function runs when the descriptor is made, but the constructor does not.
Keep the key function pure, because it normally runs from a parent builder.

When daRg mounts a descriptor, it runs the constructor once.
The constructor usually returns a builder closure, as in the example.
That builder runs again when one of its own ``watch`` values changes or
whenever daRg needs to do it.
A constructor can return a table instead, but then the component must be
completely static: a table cannot be rebuilt from its ``watch`` field or
from later argument changes.

Keys and matching
~~~~~~~~~~~~~~~~~

On a parent rebuild, daRg compares each stateful descriptor with the mounted
children of that parent. A descriptor matches an existing instance when both
the component type and the key are the same. A match keeps the existing
instance and updates its arguments. No match creates a new instance.

The key belongs to the component type, not to each call. Use a stable ID from
the data, such as ``squad.id``. Do not use an array index if the list can be
reordered.

The key function receives ordinary values. If an argument is an observable,
the key function receives its current value. It may use any subset of the
constructor arguments, in any order, but its parameter names must match the
constructor parameter names. It never receives ``scope``:

.. code-block:: quirrel

   let SeatCard = StatefulComp(
     function(scope, soldier, seat) {
       return @() {
         watch = [soldier, seat]
         rendObj = ROBJ_TEXT
         text = $"{soldier.get().name}, seat {seat.get()}"
       }
     },
     @(soldier, seat) $"{soldier.guid}:{seat}")

Both the constructor and the key function must have fixed parameters;
default parameters and varargs are rejected.
Returning ``null`` leaves the component unkeyed.

Keys are compared with ``==``: a number, a string or a boolean matches by
value, and a table, an array or an instance matches only when it is the same
object. Therefore, do not build the key. ``@(item) {id = item.id}`` returns a
new table on each rebuild, so it never matches and the instance loses its
state. Use the object itself when the data keeps it stable: ``@(item) item``.

Keys should be unique among stateful siblings of the same type.
Unkeyed siblings, and siblings with duplicate keys, are matched in their current order.
That means their state can move to the wrong item after a reorder.

The key is calculated when the parent creates a descriptor.
If the next descriptor has a different key, daRg replaces that instance.
Do not add a ``key`` field to the table returned by a stateful component;
daRg uses the key from ``StatefulComp`` instead.

If a key depends on an observable, it is recalculated only when the parent
builds a new descriptor.
Watch that observable in the parent when a change should remount the component.

Arguments are observables
~~~~~~~~~~~~~~~~~~~~~~~~~

Every constructor argument is an observable.
For a plain value, daRg creates a private ``Watched`` for that instance.
When the same instance is found on a later parent rebuild, daRg writes
the new value to that ``Watched``.
Read it with ``.get()`` and include it, or a value derived from it, in ``watch``.

.. code-block:: quirrel

   // This keeps the first name forever.
   let Card = StatefulComp(function(scope, item) {
     let name = item.get().name
     return @() { rendObj = ROBJ_TEXT, text = name }
   })

   // This follows later values passed for item.
   let Card = StatefulComp(function(scope, item) {
     let name = scope.Computed(@() item.get().name)
     return @() {
       watch = name
       rendObj = ROBJ_TEXT
       text = name.get()
     }
   })

Reading an argument once in the constructor is still useful for deliberate
mount-time state, such as an animation's starting value.
Do not use a one-time read for data the component renders.
In developer builds, daRg reports a changed value argument that has no reactive consumer.
This catches a common stale-data mistake, but it does not replace a complete ``watch`` list.

When an argument is a mount-time constant on purpose - a stable config bag the
constructor reads once, that a parent rebuild still re-supplies as a fresh table -
name it with a ``mount`` prefix (``mountConfig`` or ``mount_config``) to opt out
of that report. The cell still updates on every rebuild, so a later ``.get()``
reads the newest value; only the report is muted. The prefix must sit at a word
boundary - end, ``_``, or an upper-case letter - so ``mount_config``,
``mountConfig`` and ``mount`` opt out but ``mountainInfo`` does not.

You can also pass an observable as an argument.
The constructor receives that same observable instead of a private one.
For as long as the instance is kept, later descriptors must pass the same observable again.
Changing between a value and an observable, or replacing the observable, is an error.

A call must provide exactly the constructor's number of arguments, not
counting ``scope``; daRg supplies the scope itself.

Where descriptors are allowed
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

A descriptor is valid only as a value of ``children``, either by itself or in
the children array.
It cannot be a scene or panel root, a builder result, a constructor result, or ``gap``.
Wrap it in a component description when one of those places needs it:

.. code-block:: quirrel

   // Valid as a root or a gap component.
   { children = SquadCard(squad) }

Lifetime and cleanup
~~~~~~~~~~~~~~~~~~~~

FRP state created through ``scope`` belongs to the mounted instance.
``scope.subscribe`` callbacks are removed as soon as the element is detached,
so they do not fire during a fade-out. ``scope``-created ``Watched`` and
``Computed`` values are disposed later, when the element is finally deleted,
so they stay readable while a removed element is finishing a fade-out.

``scope.subscribe`` works on any observable, including one from outside the
component, such as a module-scope ``Watched``. daRg removes the callback and
keeps the observable, so you do not need ``unsubscribe``. A plain
``obs.subscribe(callback)`` is not owned by the instance: the callback stays on
the observable after the element is gone.

An observable holds one subscription per function. ``scope.subscribe`` with a
function that is already subscribed to that observable outside the scope is an
error, because the scope could not remove it at detach. Pass a distinct
function instead.

Timers and other resources outside FRP are not owned by the instance.
Release them from ``scope.onDetach``, which runs when the element is detached,
the same moment ``scope.subscribe`` callbacks are removed.
``scope``-created observables are still readable inside the
callback; they are disposed later, after any fade-out. The callback must not
create state or subscribe, because the scope is tearing down.

.. code-block:: quirrel

   let Ticker = StatefulComp(function(scope) {
     let value = scope.Watched(0)
     let handle = gui_scene.setInterval(1.0, @() value.modify(@(v) v + 1))
     scope.onDetach(@() gui_scene.clearTimer(handle))
     return @() { watch = value, rendObj = ROBJ_TEXT, text = value.get().tostring() }
   })

Keep setup out of the builder
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

A builder must not create state.
Creating a ``Watched`` or ``Computed``, calling ``subscribe``, or registering
``scope.onDetach``, whether through ``scope`` or not, from the returned builder
is an error.
The builder should only read existing state and return a component description.

The constructor is the place for instance state, but it is not the only one:
a kept ``scope`` also works after the mount. An event handler can create
state through it or call ``scope.subscribe``, and that state still dies with
the instance. ``scope.subscribe`` from an event handler is the leak-free
form of the plain ``subscribe`` that would stay on the observable forever.
