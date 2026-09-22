# virtio_scsi: check get_parent_node / get_driver

Same class of bug as virtio-block-get-driver.

Two call sites:

1. `virtio_scsi_register_device` — `get_driver(parent)` then
   `virtio->read_device_config`.
2. `VirtioSCSIController` ctor — `get_parent_node` twice then
   `fVirtio->negotiate_features`. Failed InitCheck already deletes
   the object (`sim_init_bus`).

Not the virtio_block DMAResource / blk_size issues. Those objects
do not exist here.

Jam: module under `src/add-ons/kernel/busses/scsi/virtio`.
Overlay: `~/config/non-packaged/add-ons/kernel/busses/scsi/virtio`
(confirm with `listimage` after reboot).

No userspace test: needs a broken device-manager parent.
