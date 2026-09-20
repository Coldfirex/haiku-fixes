# udp: free cloned buffer when DeliverData enqueue fails

Status: **merged** [Gerrit 11792](https://review.haiku-os.org/c/haiku/+/11792) / `1ca7d0a6`.
Do not re-push. Do not apply on current master.

- Change-Id: `Ifc524e94fdc09f1ffc7a7c236189f131ec1a2c86`
- Topic: `udp`
- Code-Review: +2 waddlesplash
- Merged: 2026-09-19 by korli

`DeliverData` clones the incoming datagram, deframes it, then
`Enqueue()`s it. `Enqueue` returns `ENOBUFS` when the socket receive
buffer is full and does not take ownership. `EnqueueClone` already
frees on that failure; `DeliverData` did not.

Test: `make && ./udp_deliverdata_leak 60`
