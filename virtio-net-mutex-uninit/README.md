# virtio_net: destroy TX/RX mutexes if interrupt setup fails

Status: **local**. Split out of `virtio_net-tx-freelist-and-mutex-uninit`.

The TX free-list half is `virtio-tx-freelist/` (merged 11807). Do not
combine them again. Do not amend 11784.

Still applies verbatim to current Haiku master `virtio_net.cpp`
(`@@ -434`). 11807 only touched `uninit_device`.

## What is wrong

`mutex_init()` for `rxLock` / `txLock` runs before `setup_interrupt()`.
On failure `init_device()` jumps to `err6`, which deletes the TX
BufInfo array and returns. The device manager does not call
`uninit_device()`, so the only existing `mutex_destroy()` pair never
runs. `uninit_driver()` is just `free(info)`.

`err6` is also the TX BufInfo allocation / `get_memory_map` label
*before* `mutex_init()`. Destroying on `err6` would hit uninitialized
mutexes. This patch adds `err7` for the post-init path.

Same pattern as virtio_gpu Gerrit 11776.

## Live vs defensive

- **Live:** PCI `setup_interrupt()` can return an error (`irq == 0`
  "PCI IRQ not assigned", or `install_io_interrupt_handler()`). That
  is a real bus-manager return, not a spec ghost. That is the
  justification for this CL.
- **Defensive:** `VirtioQueue::SetupInterrupt()` and the MMIO queue
  hook currently always `return B_OK`. The RX/TX/ctrl
  `queue_setup_interrupt()` gotos cannot fire on current master.
  Leave them on `err7` anyway.
- No `interrupt setup failed` / `PCI IRQ not assigned` /
  `can't install interrupt handler` in captured QEMU/Proxmox
  syslogs. Do not tell Gerrit this was reproduced on a working
  virtio-net guest.

Not the 11833 class of change. Do not claim a QEMU hit you do not
have.

## Keep separate

Does not call `free_interrupts()` if `setup_interrupt()` succeeded
and a later `queue_setup_interrupt()` failed. That follow-up is
`virtio-net-interrupt-uninit/` and is mostly theoretical today
(queue setup cannot fail). Keep them separate.

Does not call `free_queues()` on `err1`–`err6`. Separate leftover.

Rebuild-only. Overlay that loaded:

`~/config/non-packaged/add-ons/kernel/drivers/network/virtio_net`
