# Abandoned

Dropped changes. Keep the folder as a record.
Do not re-push. Do not `git apply` these on current master.

| Folder | Gerrit | Why |
|---|---|---|
| virtio-block-blksize-zero | [11833](https://review.haiku-os.org/c/haiku/+/11833) | Spec does not define a fallback for blk_size == 0; no known host sends it. |
| udp-receiveerror | never submitted | ReceiveError early return is not a leak. `device_consumer_thread` frees on error. Free-then-error double-freed (`deadbeef` in `stack::free_buffer`). A/B 60s used_pages did not improve. |
