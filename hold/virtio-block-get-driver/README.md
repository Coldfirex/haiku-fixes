# virtio_block: check get_parent_node / get_driver (hold)

Do not submit. `init_device` used `info->virtio` with no check.
Failed get_parent_node / get_driver would NULL-deref, but we have not
shown that path on QEMU. Parked after 11833.
