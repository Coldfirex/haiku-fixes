# virtio_gpu: clean up shared area when open fails

Status: **local**. Follow-up to korli review on 11824.
Not `virtio-free-id` (virtio_net). Not `virtio-gpu-free-areas` (11824 MERGED).

- 11771 detach-backing MERGED
- 11776 mutex-uninit MERGED
- 11783 clone-fd MERGED (accelerant)
- 11824 free-areas MERGED
- This change: `virtio_gpu_open()` shared-area leak + area-id sentinels

Do not amend 11771 / 11776 / 11783 / 11824. New Change-Id.
Do not `open()` `/dev/graphics/...` from a tester on a live desktop.
Do not patch `commandDone` here.

Overlay after jam + reboot:

`~/config/non-packaged/add-ons/kernel/drivers/graphics/virtio_gpu`

Not `drivers/bin`, not `accelerants/`. Do not use `on-haiku.sh go`.

General method: repo-root `GUEST-WORKFLOW.md`.

## Smoke (no device-open tester)

Error-path leak is not safe to force on a live app_server session.
Baseline and patched runs are boot + mode switch only.

After overlay reboot:

```
listimage | grep virtio_gpu
listarea | grep virtio_gpu
```

Must show `.../config/non-packaged/add-ons/kernel/drivers/graphics/virtio_gpu`.
Expect `virtio_gpu shared info` and `virtio_gpu framebuffer` areas.
Two Screen Preferences mode switches. No KDL. Syslog must not grow
`detach_backing failed` / `attach_backing failed` from this change.

## Gerrit

Topic `virtio-gpu`. New Change-Id. Body is the .patch text above `---`.
