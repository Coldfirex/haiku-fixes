# udp: free cloned buffer when DeliverData enqueue fails

Status: **submitted** (not merged yet).

- Gerrit: https://review.haiku-os.org/c/haiku/+/11792
- Change-Id: `Ifc524e94fdc09f1ffc7a7c236189f131ec1a2c86`
- Topic: `udp`
- Patch set 1: `0776b8d2e52c0fb5e6c075b895d8a775873f7fd5`
- Submitted: 2026-09-18
- Code-Review: +2 waddlesplash

Do not re-push 11792.

`DeliverData` clones the incoming datagram, deframes it, then
`Enqueue()`s it. `Enqueue` returns `ENOBUFS` when the socket receive
buffer is full and does not take ownership. `EnqueueClone` already
frees on that failure; `DeliverData` did not.

Test: `make && ./udp_deliverdata_leak 60`
Unpatched: used pages climb after the FIFO fills.
Patched: pages flatten.

## Jam / overlay

```
export HAIKU_SRC=/boot/home/Desktop/sources/haiku
cd $HAIKU_SRC/generated
jam -q udp
```

Overlay that loaded:

`~/config/non-packaged/add-ons/kernel/network/protocols/udp`

Do not use `on-haiku.sh go`. After merge, drop the overlay and
`shutdown -r` to run packaged.
