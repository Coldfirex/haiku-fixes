# Abandoned

Dropped changes. Keep the folder as a record.
Do not re-push. Do not `git apply` these on current master.

| Folder | Gerrit | Why |
|---|---|---|
| virtio-block-blksize-zero | [11833](https://review.haiku-os.org/c/haiku/+/11833) | Spec does not define a fallback for blk_size == 0; no known host sends it. |
| udp-receiveerror | never submitted | ReceiveError early return is not a leak. Consumer frees on error. |
| ipv4-error-received | never submitted | Only freed on error returns. Consumer already frees. |
| ipv6-error-received | never submitted | Same as ipv4-error-received. |
