# virtio_net: free interrupts if queue interrupt setup fails

Status: **local**. Follow-up to `virtio-net-mutex-uninit/`.
Apply that patch first. Do not fold this into the mutex CL.
Do not amend 11784.

`setup_interrupt()` can succeed and a later `queue_setup_interrupt()`
can fail. `init_device()` then returns an error. The device manager
does not call `uninit_device()`, so the interrupt stays configured.

`uninit_device()` already does `free_interrupts()` then
`mutex_destroy()`. Mirror that on the init-fail path with `err8`.

`setup_interrupt()` failure still goes to `err7`. There is nothing
to free if setup itself failed.

Does not call `free_queues()` on the earlier labels. That leak exists
on every init failure after `alloc_queues()` and is a separate change.

Rebuild-only. Overlay:

`~/config/non-packaged/add-ons/kernel/drivers/network/virtio_net`
