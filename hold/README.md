# Hold — do not submit

Local patches we are keeping but will not send to Gerrit until there is
evidence the failure actually happens (same lesson as abandoned 11833).

Not abandoned: they were never uploaded.
Not merged. Do not `gerrit.sh` these. `work.sh` should ignore this tree.

| Folder | Why it is on hold |
|---|---|
| virtio-block-get-driver | get_parent_node / get_driver NULL in init_device |
| virtio-scsi-register-get-driver | get_driver NULL in register_device |
| virtio-scsi-controller-get-driver | get_parent_node / get_driver NULL in ctor |

Device-manager is supposed to have a parent when these run. No QEMU
repro. Move back to the repo root only after a real fail is shown.
