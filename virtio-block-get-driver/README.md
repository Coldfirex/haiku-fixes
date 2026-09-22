# virtio_block: check get_parent_node / get_driver

`init_device` used `info->virtio` with no check. Failed
`get_parent_node` / `get_driver` is a NULL deref KDL.

Same call exists in virtio_scsi. That follow-up is
`virtio-scsi-get-driver/`.
