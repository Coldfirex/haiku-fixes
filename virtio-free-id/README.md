# virtio_net: free device id if publish_device fails

Status: **merged**.

- Gerrit: https://review.haiku-os.org/c/haiku/+/11784
- Change-Id: `Ic1a3fe547841207d5968a71e7bea0c8e8b70214f`
- Topic: `virtio-net`
- haiku.git: `bf90d383c30ecaa3a85ea9938b049680dbc78145`
- Merged: 2026-09-18
- Reviewed-by: Jérôme Duval (korli)

Do not re-push 11784.

## Same bug, other drivers (not ours)

korli followed 11784 with
https://review.haiku-os.org/c/haiku/+/11786
(`I378eb5ebefebb3306c74b0d30bc6fe5fcce69d09`, still NEW as of 2026-09-18):

- virtio_gpu `register_child_devices` (publish_device)
- virtio_block, virtio_input
- nvme_disk, mmc_disk
- usb_ecm
- scsi sim_interface, ata ATAModule, i2c I2CModule
- ACPI EC / ac / battery / lid / thermal / als, pch_thermal

We never had local copies of those. Do not add them.

`virtio-gpu-open-shared-area` is a different leak (`open()` shared-info
area + uninit area ids). Keep that folder. It is not 11786.

`virtio-tx-freelist` is a different virtio_net TX/init leak. Keep it.

## Overlay that loaded (historical)

`~/config/non-packaged/add-ons/kernel/drivers/network/virtio_net`

After merge, drop the overlay and `shutdown -r` to run packaged.
