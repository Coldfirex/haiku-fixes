# virtio_net: return TX buffer to free list on queue failure

Status: **merged** [Gerrit 11807](https://review.haiku-os.org/c/haiku/+/11807) / `d42d1ebd`.
Do not re-push. Do not apply on current master.

- Change-Id: `Ic054c8cdef3eaa4637a3ea46ecd45655db34fe25`
- Topic: `virtio-net`
- Code-Review: +2 korli
- Merged: 2026-09-21 by waddlesplash

The send path removes a `BufInfo` from `txFreeList` before
`queue_request_v()`. If that call fails the request is not queued and
no completion returns the slot. Put it back on `txFreeList` under
`txLock`.

The mutex-destroy half is still `virtio-net-mutex-uninit/`.
Do not amend 11784. Drop the virtio_net overlay for this change.
