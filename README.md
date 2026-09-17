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
virtio-gpu-mutex-uninit/ submitted Gerrit 11776; commandLock leak if interrupt setup fails
virtio-gpu-detach-backing/ merged Gerrit 11771 / 0438319c; zero-init DETACH_BACKING
virtio-gpu-clone-fd/     accelerant clone path closed fd twice
virtio-gpu-open-shared-area/ shared info area leak if open() fails
```

Gerrit tracking lives in each submitted folder as `STATUS`
(`local` / `submitted` / `merged` / `abandoned`).

- Patch 1 merged: https://review.haiku-os.org/c/haiku/+/11771
  https://github.com/haiku/haiku/commit/0438319c127429a416086d1220f79ff94d71f2d0
- Patch 2 submitted: https://review.haiku-os.org/c/haiku/+/11776

## Apply a patch

From a Haiku source tree:

```
git apply /path/to/<folder>/<name>.patch
```

## Run a userspace test (Haiku)

```
cd udp-deliverdata
make
./udp_deliverdata_leak 60
```

Unpatched: `used pages` climbs after the socket FIFO fills.
Patched: pages flatten.

`udp-receiveerror` needs raw ICMP (`SOCK_RAW`). Default target is `127.0.0.1`.

`udp-unicast-enqueue` has a localhost send/recv check (`make && ./udp_unicast_loopback`).
That only proves ownership and the loopback path still delivers. Measure
#18730 with:

```
iperf3 -s
iperf3 -c localhost -u -b 0 -t 20
```

`arp-reject-learn` talks to the ARP generic syscall (`make && ./arp_reject_learn`).
Needs an IPv4 ethernet interface so the ARP module is loaded.
Unpatched: GET_ENTRY fails after SET reject then SET without reject.
Patched: prints `reject lifted`.

`ipv4-multicast-filter` uses setsockopt (`make && ./ipv4_multicast_filter`).
Unpatched: a second IP_UNBLOCK_SOURCE / IP_DROP_SOURCE_MEMBERSHIP returns 0.
Patched: the second call returns EADDRNOTAVAIL.

`ipv4-multicast-filtermode` is a constructor default. Confirm with a rebuild.

`ipv4-multicast-refs` and `ipv4-fragment-reassemble` are stack error
paths. Confirm with a rebuild. The membership test also exercises the
get_route / get_interface path that leaked references. The refs patch
also NULL-inits `multicast_address` so the destructor delete is safe.

Virtio, virtio_gpu, TCP, ICMP error-reply, and the other ARP changes are
driver/stack error paths. They have no userspace flooder in this tree;
confirm with a rebuild and the failing path.
