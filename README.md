# Haiku networking notes

One folder per issue. The patch and its test (if any) live together.
These are notes and reproducers, not a Haiku Gerrit submission.
Do not send the generated patches to review.haiku-os.org as-is.

Patches follow the Haiku coding guidelines:
https://www.haiku-os.org/development/coding-guidelines/

```
udp-deliverdata/         DeliverData clone leak when FIFO is full
udp-receiveerror/        ReceiveError / DeliverError early-return leak
virtio-tx-freelist/      TX BufInfo leak + mutex teardown on init fail
virtio-free-id/          device id leak on publish_device fail
tcp-spawn-abort/         listen-queue child leak when _Spawn fails
icmp-error-reply/        reply buffer leak if get_domain/prepend fails
udp-unicast-enqueue/     #18730 enqueue incoming unicast buffer (no clone)
udp-loopback-checksum/   #18730 skip TX checksum if route is IFF_LOOPBACK
arp-request-buffer-dtor/ ~arp_entry leaked the request template
arp-queued-send/         MarkValid NULL protocol KDL + send-fail leak
arp-reject-learn/        #18816 reject never cleared on learn
arp-protocol-teardown/   handler leak on init fail; UAF on uninit
ipv4-multicast-filter/   UnblockSource/DropSSM call Remove, not Add
ipv4-multicast-filtermode/ init MulticastGroupInterface fFilterMode
ipv4-multicast-refs/     put_route/put_interface; IP_MULTICAST_IF dtor + NULL init
ipv4-fragment-reassemble/ 32-bit fragment end; restore buffers on merge fail
virtio-gpu-detach-backing/ merged Gerrit 11771 / 0438319c; zero-init DETACH_BACKING
virtio-gpu-mutex-uninit/ merged Gerrit 11776 / 768d4e6c; commandLock leak if interrupt setup fails
virtio-gpu-clone-fd/     submitted Gerrit 11783; accelerant double-close; do not live-run virtio_gpu_clone
virtio-gpu-open-shared-area/ shared info area leak if open() fails
```

Gerrit tracking lives in each submitted folder as `STATUS`
(`local` / `submitted` / `merged` / `abandoned`).

- Patch 1 merged: https://review.haiku-os.org/c/haiku/+/11771
  https://github.com/haiku/haiku/commit/0438319c127429a416086d1220f79ff94d71f2d0
- Patch 2 merged: https://review.haiku-os.org/c/haiku/+/11776
  haiku.git `768d4e6cba315d55c9469c85d9219243408152d7`
- Patch 3 submitted: https://review.haiku-os.org/c/haiku/+/11783
  Change-Id `I42d3007704c57958050eba997ba4537718e5f619`
  Jam `virtio_gpu.accelerant`.
  Overlay `~/config/non-packaged/add-ons/accelerants/virtio_gpu.accelerant`.
  Do not use `on-haiku.sh go`. Do not run `virtio_gpu_clone` on a live desktop.

## Apply a patch

From a Haiku source tree:

```
git apply /path/to/<folder>/<name>.patch
```
