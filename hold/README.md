# Hold — do not submit

Local patches we are keeping but will not send to Gerrit until there is
evidence the failure actually happens (same lesson as abandoned 11833).

Not abandoned: they were never uploaded.
Not merged. Do not `gerrit.sh` these. `work.sh` should ignore this tree.

| Folder | Why it is on hold |
|---|---|
| virtio-block-get-driver | get_parent_node / get_driver NULL in init_device |
| virtio-scsi-register-get-driver | get_driver NULL in register_device |
| virtio-scsi-controller-get-driver | get_parent_node / get_driver NULL in ctor |
| virtio-net-interrupt-uninit | queue_setup_interrupt always returns B_OK |
| virtio-rx-freelist | RX re-queue after dequeue is not B_BUSY |

Device-manager is supposed to have a parent when these run. No QEMU
repro. Move back to the repo root only after a real fail is shown.

`virtio-net-interrupt-uninit`: VirtioQueue::SetupInterrupt() only stores
the callback and returns B_OK. The err8 path never runs. Keep the mutex
patch; that one is setup_interrupt() which can fail.

`virtio-rx-freelist`: rxSizes is queue_size/2 and each buffer uses two
descriptors, so open() fits. receive() only re-queues after a dequeue.
drain_queues runs from free(); those BufInfos stay in rxBufInfos[] and
are deleted on uninit. Not a heap leak. Keep virtio-tx-freelist (merged
11807); TX can hit B_BUSY.
