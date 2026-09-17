# virtio_net: free device id if publish_device fails

Status: **submitted**.

- Gerrit: https://review.haiku-os.org/c/haiku/+/11784
- Change-Id: `Ic1a3fe547841207d5968a71e7bea0c8e8b70214f`
- Topic: `virtio-net`
- Submitted: 2026-09-17

This is **not** virtio_gpu patch 4. GPU series:

- GPU 1: 11771 **MERGED** `0438319c127429a416086d1220f79ff94d71f2d0`
- GPU 2: 11776 **MERGED** `768d4e6cba315d55c9469c85d9219243408152d7`
- GPU 3: 11783 **MERGED** `55d56e03a0834af356f883d3e148284cd9d286e9`
- GPU leftover (local): `virtio-gpu-open-shared-area`

Do not amend 11771 / 11776 / 11783. Do not re-push 11784 unless Gerrit
asks for a new patch set.

## Commit message used on Gerrit

```
virtio_net: free device id if publish_device fails

create_id() reserves a bit in a per-generator bitmap, limited to 64
IDs. The ID is used only as the /dev/net/virtio/N suffix. If
publish_device() fails, the bit was not released, so repeated failed
probes could exhaust the generator.
```

## Overlay that loaded

`~/config/non-packaged/add-ons/kernel/drivers/network/virtio_net`

`drivers/bin` + `dev/net` did not load. Reboot with `shutdown -r`.

Smoke-tested 2026-09-17: listimage showed the non-packaged path,
`/dev/net/virtio/0` up, ping 1.1.1.1 ok.
