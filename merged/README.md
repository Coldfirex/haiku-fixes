# Merged upstream

These issues landed in haiku.git. Keep the folder as a record.
Do not re-push. Do not `git apply` them on current master.

| Folder | Gerrit | haiku.git | Subject |
|---|---|---|---|
| virtio-gpu-detach-backing | [11771](https://review.haiku-os.org/c/haiku/+/11771) | `0438319c` | virtio_gpu: initialize detach_backing command |
| virtio-gpu-mutex-uninit | [11776](https://review.haiku-os.org/c/haiku/+/11776) | `768d4e6c` | virtio_gpu: destroy commandLock if interrupt setup fails |
| virtio-gpu-clone-fd | [11783](https://review.haiku-os.org/c/haiku/+/11783) | `55d56e03` | virtio_gpu accelerant: do not close clone fd twice |
| virtio-free-id | [11784](https://review.haiku-os.org/c/haiku/+/11784) | `bf90d383` | virtio_net: free device id if publish_device fails |
| arp-request-buffer-dtor | [11789](https://review.haiku-os.org/c/haiku/+/11789) | `6c10ad5b` | arp: free request_buffer when the cache entry is destroyed |
| ipv4-multicast-filtermode | [11791](https://review.haiku-os.org/c/haiku/+/11791) | `8d435047` | ipv4: initialize MulticastGroupInterface filter mode to include |
| udp-deliverdata | [11792](https://review.haiku-os.org/c/haiku/+/11792) | `1ca7d0a6` | udp: free cloned buffer when DeliverData enqueue fails |

Move a folder here only after Gerrit status is **merged** (not submitted).
