# virtio_block: delete DMAResource and IOScheduler in uninit_driver

Status: **submitted** Gerrit 11843
https://review.haiku-os.org/c/haiku/+/11843
Change-Id: Ieab2c05ab02b63baa081cecb21975d4214579751
PS4, waddlesplash CR+1, not merged. Do not amend.

`dma_resource` is created in `init_driver`. `io_scheduler` is created
in `set_capacity` after a successful config read. Only `uninit_device`
deletes them.

`uninit_device` runs only if `init_device` set `*_cookie` (after
config read and `alloc_queues`). Publish/`create_id` failure after
`init_driver` is the same: teardown is `uninit_driver` only.

Call `uninit_device` from `uninit_driver`. NULL the pointers in
`uninit_device` so a second call on the normal unload path is not a
double-delete.

Not 11833. Overlay:
`~/config/non-packaged/add-ons/kernel/drivers/disk/virtual/virtio_block`
