# virtio_net: free device id if publish_device fails

This is the **virtio_net** id-leak fix (`virtio-free-id`). Status: **local**.

It is **not** virtio_gpu patch 4. The numbered GPU series is only:

- GPU 1: `virtio-gpu-detach-backing` — Gerrit 11771 **MERGED**
- GPU 2: `virtio-gpu-mutex-uninit` — Gerrit 11776 **MERGED**
- GPU 3: `virtio-gpu-clone-fd` — Gerrit 11783 **NEW**
- GPU next (unnumbered leftover): `virtio-gpu-open-shared-area` — still local

Sibling virtio_net change, do not bundle: `virtio-tx-freelist`.

Do not amend 11771, 11776, or 11783. Do not upload the GitHub `.patch`
as-is. Commit inside the Haiku tree so the hook adds a **new** Change-Id.

Overlay that loaded: `~/config/non-packaged/add-ons/kernel/drivers/network/virtio_net`.
Reboot with `shutdown -r`.

Gerrit subject/body (no free_id return-value note):

```
virtio_net: free device id if publish_device fails

create_id() reserves a bit in a per-generator bitmap, limited to 64
IDs. The ID is used only as the /dev/net/virtio/N suffix. If
publish_device() fails, the bit was not released, so repeated failed
probes could exhaust the generator.
```
