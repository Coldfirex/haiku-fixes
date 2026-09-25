# virtio_net: destroy TX/RX mutexes if interrupt setup fails

Status: **submitted**.

- Gerrit: https://review.haiku-os.org/c/haiku/+/11858
- Change-Id: `I166bb2f012cd4fc8c787b227a0befc77f43808ba`
- Topic: `virtio-net`
- Patch set: 1 (`a469679c2fff787ab20296d1f760f9cb8d2b89d5`)
- Submitted: 2026-09-25

Do not amend 11858 unless a reviewer asks. Do not amend 11784 or 11807.
Do not fold `virtio-net-interrupt-uninit/` into this CL.

Split out of `virtio_net-tx-freelist-and-mutex-uninit`.
The TX free-list half is `merged/virtio-tx-freelist/` (11807).

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

## Live vs later follow-up

- PCI `setup_interrupt()` can return an error (`irq == 0`
  "PCI IRQ not assigned", or `install_io_interrupt_handler()`).
  That is the justification for this CL.
- `VirtioQueue::SetupInterrupt()` and the MMIO queue hook currently
  always `return B_OK`. The RX/TX/ctrl `queue_setup_interrupt()`
  checks cannot fire on current master; they still go to `err7`.
- No `interrupt setup failed` in captured QEMU/Proxmox syslogs.
  Do not tell Gerrit this was reproduced on a working virtio-net guest.

`virtio-net-interrupt-uninit/` (`free_interrupts` on queue-setup
failure) stays local. `free_queues()` on `err1`–`err6` is a separate
leftover.

Rebuild-only. Overlay that loaded:

`~/config/non-packaged/add-ons/kernel/drivers/network/virtio_net`
