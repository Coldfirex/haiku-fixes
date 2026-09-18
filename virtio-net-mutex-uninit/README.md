# virtio_net: destroy TX/RX mutexes if interrupt setup fails

Status: **local**. Split out of `virtio_net-tx-freelist-and-mutex-uninit`.

The TX free-list half is `virtio-tx-freelist/`. Do not combine them
again. Do not amend 11784.

`mutex_init()` for `rxLock` / `txLock` runs before `setup_interrupt()`
and `queue_setup_interrupt()`. Those failures jumped to `err6`, which
does not `mutex_destroy()`. `err6` is also used before the mutexes
exist, so this adds `err7`.

Same pattern as virtio_gpu Gerrit 11776.

Does not call `free_interrupts()` if `setup_interrupt()` succeeded and
a later `queue_setup_interrupt()` failed. That follow-up is
`virtio-net-interrupt-uninit/`. Keep them separate.

Rebuild-only. Overlay that loaded:

`~/config/non-packaged/add-ons/kernel/drivers/network/virtio_net`
