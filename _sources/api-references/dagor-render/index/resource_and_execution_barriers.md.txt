# Resource and Execution Barriers

With low-level Graphics APIs (GAPI) comes the responsibility of managing
resource states explicitly.

## When is Barrier Required?

Textures and buffers that are modified by CPU write access are typically in a
"generic read" state once CPU access completes. This state allows the resource
to be used as a Shader Resource View (SRV), Constant Buffer View (CBV), or any
other read source in GPU commands (corresponding to all `RB_RO_*` barrier
states).

Barriers are necessary to transition resources into the appropriate state after
a GPU operation has modified the resource. For example, resources may need to
transition to a state for use as a render target, depth-stencil target,
unordered access view (UAV), blit target, or copy target (corresponding to
`RB_RW_*` barrier states). Many commands that write to resources automatically
trigger an implicit transition without the need for an additional barrier.

Using a resource in a read state will *not* implicitly transition the resource
to the required state. Manually generating the correct barrier for all future
read states can be expensive and requires knowledge of the entire frame's
execution. This is why explicit barrier management was introduced in the
Direct3D (D3D) interface.

Render targets with automatic mip generation (MipGen) will automatically be in a
state that supports SRV access in the pixel shader without additional barriers
(e.g., `RB_RO_SRV | RB_STAGE_PIXEL`). A barrier is only required if the resource
will be used differently after mip generation completes.

Readback operations automatically transition the resource to the correct state
for copying its contents into a readback buffer during the next flush (e.g.,
frame present or a blocking readback is an implicit flush). Drivers assume that
the resource is not used for other purposes until the next flush. After a
readback completes, the resource must be transitioned to its next intended
state, unless the transition is handled implicitly.

Issuing barriers on resources with pending asynchronous readback is invalid and
results in undefined behavior. For instance, if a texture is read back using a
blocking readback and is later used as an SRV in a pixel shader, the user must
manually insert a barrier to transition the resource into the SRV state for the
pixel shader stage.

## UAV Barrier (`RB_FLUSH_UAV`)

A UAV barrier ensures that multiple sequential operations, such as dispatches,
on the same resource are completed in the correct issue order, preventing race
conditions on the UAV resource. For example, if resource X is accessed by
dispatch A followed by dispatch B, and dispatch B depends on the changes made by
dispatch A, a UAV barrier is required between A and B.

If A and B access X in an order-independent manner (such as atomic operations or
non-overlapping sections), a UAV barrier is not needed and should not be issued.
In this case, the device can overlap the execution of A and B if it has
available resources, achieving similar effects to async compute queues but
within the same queue. In GAPIs that do not offer explicit barrier control,
sequential consistency is maintained by default, and drivers are unlikely to
allow overlapping work where a barrier is necessary.

Drivers implementing the D3D barrier interface may issue warnings if they detect
dependency chains missing barriers. To suppress these warnings, insert a
`RB_NONE` barrier for the affected resource, which will remove it from the
dependency chain test. A `RB_NONE` barrier is a no-op.

UAV barriers require defining both source and destination stages. Source stages
are selected using `RB_SOURCE_STAGE_*` flags, while destination stages are
specified with `RB_STAGE_*`. Multiple stages can be selected for both source and
destination. The device waits at the earliest destination stage on the latest
source stage.

UAV barriers can also be issued with a null resource (using the
`ResourceBarrierDesc` overload that only takes a `ResourceBarrier` parameter).
In this case, all pending UAV accesses from the selected source stages will
complete before execution continues in the destination stages.

UAV barriers are generally unnecessary between draw calls, as the relative order
of draw calls remains the same as in older GAPIs, where the order between draw
calls is preserved (though ordering within draw calls is undefined).

Resource barriers that transition a resource to a state other than UAV (e.g.,
SRV) implicitly handle UAV barriers and do not require additional barriers, and
doing so may negatively impact performance.

## Begin/End Barriers

Begin and end barriers mark the start and end of a resource state transition,
enabling the device to execute parallel work while transitioning the resource.
This parallel execution minimizes the overhead of the barrier, making the
transition process nearly free. These begin/end pairs can only be used on
textures currently in use as render targets (color and depth/stencil),
transitioning the resource into a different state.

Both the begin and end barriers must use the same state and target shader stage
(when applicable). If they do not, the behavior is undefined. Best practices
recommend issuing the begin barrier as early as possible (e.g., immediately
after the last use as a render target) and the end barrier as late as possible.

If there is no work between the begin and end barrier pair, the driver will
automatically merge them into a standard barrier. Validation may issue a warning
in this case if enabled.

A resource is considered inaccessible from the begin barrier until the end
barrier is reached. Violating this by using the resource or issuing other
barriers in between results in undefined behavior.

## Release/Acquire Ownership Barriers

When a texture is used across multiple queues, the ownership of the resource
must be transferred from one queue to another. Textures created with the
`TEXCF_SIMULTANEOUS_MULTI_QUEUE_USE` flag or static textures are exempt from
this rule and can be accessed by any queue in their current state. Failing to
properly transfer ownership may corrupt the resource's contents.

A resource is considered inaccessible from the release barrier until the acquire
barrier. Violating this rule by using the resource or issuing other barriers
during this period results in undefined behavior. A release/acquire pair can
also perform state transitions simultaneously, but both the release and acquire
barriers must match in state and target shader stage (when applicable);
otherwise, the behavior is undefined. Ensure that the release on one queue is
completed before the acquire on another queue begins (e.g., use a queue sync
fence).

If a resource is currently owned by queue A and the next use is, for example, as
a UAV resource on queue B, and the operation on queue B will completely replace
the resource's contents, ownership transfer may be skipped. In this case, the
driver will automatically perform a fast, uninitialized transfer. Ownership
transfer is only required when the resource's content must be preserved. Ensure
that all previous accesses by other queues are complete before skipping the
transfer, or undefined behavior may occur.

## Implicit Barriers

As a general rule, you can expect the driver to automatically insert a resource
barrier for any operation that requires write access to a (sub-)resource. This
is because write states are unique in that they prevent simultaneous use with
other read or write operations, making it efficient for the driver to handle
this transition implicitly.

To be more specific, the following D3D commands will automatically execute an
implicit resource barrier to transition the resource to the required state
before the command is executed:

- **`stretch_rect`**:
  - A `RB_RW_BLIT_DEST` barrier is applied to the destination resource.
  - No implicit barrier is applied to the source, except when the source is the
    swapchain color target (e.g., passing `nullptr` or the return value of
    `get_backbuffer_tex`), in which case a `RB_RO_BLIT_SOURCE` barrier is
    applied.

- **`copy_from_current_render_target`**:
  - Similar to `stretch_rect`, as it typically shares the same underlying
    implementation.

- **`set_rwtex`, `clear_rwtex{i|f}`, `zero_rwbuf{i|f}`, `set_rwbuffer`**:
  - A `RB_RW_UAV` barrier is applied for the affected subresource range of the
    resource.

- **`set_depth`**:
  - If `readonly` is false, a `RB_RW_RENDER_TARGET` barrier is applied to the
    affected subresource.
  - If `readonly` is true, no implicit barrier is applied.

- **`set_render_target`**:
  - A `RB_RW_RENDER_TARGET` barrier is applied to the affected subresource range
    of the resource.

    ```{note}
    For volumetric textures, all slices of a mip level are considered the same
    subresource, so applying different barriers per slice is not feasible.
    ```

## Implementation Details of DX12 Driver

DirectX 12 (DX12) lacks fine-grained control for targeting specific shader
stages for resource state transitions. There are only three primary states: UAV,
SRV for non-pixel shader stages, and SRV for pixel shader stages. Additionally,
DX12 does not distinguish between blit source and blit destination states; these
are treated as SRV for the pixel shader stage and RTV, respectively. DX12 also
does not have a concept of resource ownership; as long as a resource is in a
state that is usable by a queue, the resource can be accessed by that queue. The
driver only manages pipeline stage-independent UAV resource-specific and global
barriers, meaning the source and destination flags of UAV barriers have no
effect.

On PC platforms, the driver tracks buffer states because buffers are subject to
decay and auto-promotion to intermediate states between command buffer
submission and execution. This special behavior, which is not predictable by the
user of the D3D interface, means that the driver continuously monitors buffer
states and automatically inserts barriers as needed to maintain consistency.
Consequently, missing barriers may still work in some cases because the driver
compensates for them. This is specific to PC platforms, and for Xbox (One and
Scarlett), the driver does not support such automatic behavior. In these cases,
resources do not decay or auto-promote, as we have moved away from PC
compatibility mode.

## Validation

The DX12 driver includes an extensive set of checks to detect erroneous,
missing, or superfluous barriers. These issues will be reported as warnings to
avoid excessive error messages in the debug overlay. It is important to note
that the driver can only validate states that are crucial for correct
functionality and may not catch errors that other drivers may report.

The driver is capable of detecting the following issues:

- Missing barriers for expected resource states.
- Missing UAV barriers

  ```{note}
  UAV barriers apply to the entire resource, and the driver cannot identify
  which subresource is affected.
  ```

- Redundant resource state barriers.
- Missing split barrier begin/end pairs.
- Ending a split barrier set with a non-end barrier.
- Using split barriers where no advantage is gained (e.g., when an end
  immediately follows the begin without other work in between).

When operating in threaded mode, the driver can also suggest locations for split
barrier begin and end pairs. It will report the "path" (e.g.,
beginEvent/endEvent hierarchy) for the identified locations. However, this only
works if no other barriers for the resource are generated by the user within the
possible span of a split barrier set.

When barrier validation is enabled, it is possible that some barriers may not be
generated exactly when requested by the user but slightly earlier. This occurs
because the automatic barrier generation behavior may trigger a barrier slightly
earlier when validation is enabled.

## Implementation Details of Vulkan Driver

These details are preliminary and reflect expectations for a future
implementation.

Begin and end barriers will be implemented as standard barriers at the end
barrier stage (implementation still requires further research).

UAV flush barriers will be translated into execution barriers for the source and
destination stages.

Stages in Vulkan map directly to the selected stages, with resources becoming
available at the earliest stage of the barrier and implicitly becoming available
in subsequent stages (e.g., targeting the vertex stage makes the resource
available to the pixel stage without additional specification).

For all buffers, static textures, and non-static textures with the
`TEXCF_SIMULTANEOUS_MULTI_QUEUE_USE` flag, the driver will use
`VK_SHARING_MODE_CONCURRENT` for all queues that access the resource. For other
textures, `VK_SHARING_MODE_EXCLUSIVE` is used, requiring an explicit resource
ownership transfer.


