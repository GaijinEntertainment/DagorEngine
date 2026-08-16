Stateful Components
-------------------

Use ``StatefulComp`` when a component needs state of its own: a ``Watched``,
a ``Computed``, or an FRP subscription that should live as long as that one
component is on screen.

A normal builder can run again whenever its parent rebuilds.
State created in that builder is created again too.
A stateful component has a constructor that daRg runs once when
the component is mounted to the scene.
The constructor creates the per-instance state and returns the usual component builder.

Creating a stateful component
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Create the component type once, normally at module scope.
Calling the type (created by `StatefulComp()`) creates a descriptor;
it does not call the constructor yet.
Put that descriptor in ``children``.

.. code-block:: quirrel

   let selectedSquadId = Watched(null)

   let SquadCard = StatefulComp(
     function(squad) {
       let isSelected = Computed(@() selectedSquadId.get() == squad.get().id)

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
constructor parameter names:

.. code-block:: quirrel

   let SeatCard = StatefulComp(
     function(soldier, seat) {
       return @() {
         watch = [soldier, seat]
         rendObj = ROBJ_TEXT
         text = $"{soldier.get().name}, seat {seat.get()}"
       }
     },
     @(soldier, seat) $"{soldier.guid}:{seat}")

Both the constructor and the key function must have fixed parameters;
default parameters and varargs are rejected.
A key must be ``null``, a boolean, a number, or a string.
Returning ``null`` leaves the component unkeyed.

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
   let Card = StatefulComp(function(item) {
     let name = item.get().name
     return @() { rendObj = ROBJ_TEXT, text = name }
   })

   // This follows later values passed for item.
   let Card = StatefulComp(function(item) {
     let name = Computed(@() item.get().name)
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

You can also pass an observable as an argument.
The constructor receives that same observable instead of a private one.
For as long as the instance is kept, later descriptors must pass the same observable again.
Changing between a value and an observable, or replacing the observable, is an error.

A call must provide exactly the constructor's number of arguments.

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

FRP state created in the constructor belongs to the mounted instance.
daRg disposes constructor-created ``Watched`` and ``Computed`` values, along with
FRP subscriptions created there, when the element is finally deleted.
The state remains available while a removed element is finishing a fade-out.

This includes a subscription to an observable from outside the component, such
as a module-scope ``Watched``. daRg removes the callback and keeps the
observable, so you do not need ``unsubscribe``. Only the constructor works this
way: a ``subscribe`` call from ``onAttach`` or from an event handler stays on
the observable after the element is gone.

Timers and other resources outside FRP are not owned by the instance.
Clear them in the normal lifecycle hooks, such as ``onDetach``.

Keep setup out of the builder
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The constructor is the only place in a stateful component where you can
create observables or subscribe.
Creating a ``Watched`` or ``Computed``, or calling ``subscribe``,
from its returned builder is an error.
The builder should only read existing state and return a component description.
