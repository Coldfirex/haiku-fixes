# virtio_block: delete DMAResource and IOScheduler in uninit_driver

`dma_resource` is created in `init_driver`. `io_scheduler` is created
in `set_capacity` after a successful config read. Only `uninit_device`
deletes them.

`uninit_device` runs only if `init_device` set `*_cookie` (after
config read and `alloc_queues`). Publish/`create_id` failure after
`init_driver` is the same: teardown is `uninit_driver` only.

Delete both in `uninit_driver`. NULL them in `uninit_device` so the
normal unload path is not a double-delete.

Not 11833. Overlay:
`~/config/non-packaged/add-ons/kernel/drivers/disk/virtual/virtio_block`
