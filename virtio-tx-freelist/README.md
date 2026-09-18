# virtio_net: return TX BufInfo if queue_request_v fails

Status: **local**. Split out of `virtio_net-tx-freelist-and-mutex-uninit`.

The mutex-destroy half is `virtio-net-mutex-uninit/` (same pattern as
Gerrit 11776). Do not combine them again. Do not amend 11784.

Send path takes a `BufInfo` off `txFreeList`. If `queue_request_v()`
fails the request is not queued, so `txDone` never returns the slot.
Put it back before dropping `txLock`.

Rebuild-only. Overlay that loaded:

`~/config/non-packaged/add-ons/kernel/drivers/network/virtio_net`
