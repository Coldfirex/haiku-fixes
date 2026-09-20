# virtio_net: return TX buffer to free list on queue failure

Status: **submitted** [Gerrit 11807](https://review.haiku-os.org/c/haiku/+/11807) PS2.
Split out of `virtio_net-tx-freelist-and-mutex-uninit`.

The mutex-destroy half is `virtio-net-mutex-uninit/` (same pattern as
Gerrit 11776). Do not combine them again. Do not amend 11784.

The send path removes a `BufInfo` from `txFreeList` before
`queue_request_v()`. If that call fails the request is not queued and
no completion returns the slot. Put it back on `txFreeList` under
`txLock`, then drop the lock once. Keep the blank line after unlock.

Rebuild-only. Overlay that loaded:

`~/config/non-packaged/add-ons/kernel/drivers/network/virtio_net`
