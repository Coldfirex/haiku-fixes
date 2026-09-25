# virtio_block: delete DMAResource and IOScheduler in uninit_driver

Status: **merged**.

- Gerrit: https://review.haiku-os.org/c/haiku/+/11843
- Change-Id: `Ieab2c05ab02b63baa081cecb21975d4214579751`
- Topic: `virtio-block`
- haiku.git: `40f89b3f`
- Merged: 2026-09-25
- Reviewed-by: waddlesplash +1, korli +2

Do not re-push 11843. Do not apply this patch on current master.
Do not amend 11843. Not 11833.

`dma_resource` is created in `init_driver`. `io_scheduler` is created
in `set_capacity` after a successful config read. Only `uninit_device`
deleted them. `uninit_device` runs only if `init_device` set `*_cookie`.
Call `uninit_device` from `uninit_driver` and NULL the pointers so a
second call on the normal unload path is not a double-delete.

After merge, drop any leftover overlay:

```
rm -f ~/config/non-packaged/add-ons/kernel/drivers/disk/virtual/virtio_block
shutdown -r
```
