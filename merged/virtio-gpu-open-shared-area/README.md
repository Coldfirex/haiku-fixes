# virtio_gpu: clean up shared area when open fails

Status: **merged**.

- Gerrit: https://review.haiku-os.org/c/haiku/+/11851
- Change-Id: `I74e10a4eafd2a6e030e8c7e273bd98222884e5cb`
- Topic: `virtio-gpu`
- haiku.git: `83d859ac`
- Merged: 2026-09-25
- Reviewed-by: korli +2

Do not re-push 11851. Do not apply this patch on current master.
Do not amend 11771 / 11776 / 11783 / 11824 / 11851.

Follow-up to review on 11824. `open()` created the shared-info area
then `goto error` without `delete_area(sharedArea)`. Area IDs were
left at zero after `memset` in `init_driver()`, so an earlier failure
could `delete_area(0)`. Init both ids to `-1` in `init_driver()` and
at the start of `open()`, and delete the shared area on the error path.

After merge, drop any leftover overlay:

```
rm -f ~/config/non-packaged/add-ons/kernel/drivers/graphics/virtio_gpu
shutdown -r
```
