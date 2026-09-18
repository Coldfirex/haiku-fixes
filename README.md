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
virtio-free-id/          merged Gerrit 11784 / bf90d383; virtio_net free_id if publish_device fails
tcp-spawn-abort/         listen-queue child leak when _Spawn fails
icmp-error-reply/        reply buffer leak if get_domain/prepend fails
udp-unicast-enqueue/     #18730 enqueue incoming unicast buffer (no clone)
udp-loopback-checksum/   #18730 skip TX checksum if route is IFF_LOOPBACK
arp-request-buffer-dtor/ merged Gerrit 11789 / 6c10ad5b; ~arp_entry leaked the request template
arp-queued-send/         MarkValid NULL protocol KDL + send-fail leak
arp-reject-learn/        #18816 reject never cleared on learn (not 11789)
arp-protocol-teardown/   handler leak on init fail; UAF on uninit
ipv4-multicast-filter/   UnblockSource/DropSSM Remove; last SSM source LeaveGroup
ipv4-multicast-filtermode/ init MulticastGroupInterface fFilterMode
ipv4-multicast-refs/     put_route/put_interface; IP_MULTICAST_IF dtor + NULL init
ipv4-fragment-reassemble/ 32-bit fragment end; restore buffers on merge fail
virtio-gpu-detach-backing/ merged Gerrit 11771 / 0438319c; zero-init DETACH_BACKING
virtio-gpu-mutex-uninit/ merged Gerrit 11776 / 768d4e6c; commandLock leak if interrupt setup fails
virtio-gpu-clone-fd/     merged Gerrit 11783 / 55d56e03; accelerant double-close
virtio-gpu-open-shared-area/ leftover GPU change; shared info area leak if open() fails
```

Gerrit tracking lives in each submitted folder as `STATUS`.
Do not send the GitHub `.patch` files to review.haiku-os.org as-is.

See also https://github.com/Coldfirex/haiku-fixes/issues/1 (Network prefs
gateway blank after ifconfig down; not ARP).

virtio_gpu notes that burned us on GPU patches 1–3. They also apply
to `virtio-free-id` (virtio_net) and `virtio-gpu-open-shared-area`.
Those two are separate leftover locals — do not number them as GPU 4
unless you mean the GPU series only. Steps:

- virtio_net id leak: `virtio-free-id/README.md`
- GPU shared-area leak: `virtio-gpu-open-shared-area/README.md`
- ARP request buffer: `arp-request-buffer-dtor/README.md`

Shared rules:

- Guest trees: `/boot/home/Desktop/sources/{haiku,haiku-fixes}`
- Always `export HAIKU_SRC` (helpers do not search Desktop)
- `chmod +x work.sh gerrit.sh on-haiku.sh`
- Do **not** use `on-haiku.sh go` for this series
- GPU kernel overlay (11771 / 11776 / open-shared-area):
  `~/config/non-packaged/add-ons/kernel/drivers/graphics/virtio_gpu`
- GPU accelerant overlay (11783 only):
  `~/config/non-packaged/add-ons/accelerants/virtio_gpu.accelerant`
- virtio_net overlay (`virtio-free-id` / `virtio-tx-freelist`):
  `~/config/non-packaged/add-ons/kernel/drivers/network/virtio_net`
- ARP kernel overlay: `~/config/non-packaged/add-ons/kernel/network/datalink_protocols/arp`
  (jam target is `'<module>arp'`, not userspace `arp`)
- `gerrit.sh` may be missing from the guest clone; commit/push by hand
- Do not `open()` the GPU / run `virtio_gpu_clone` on a live desktop
- New commit + new Change-Id per issue; do not amend 11771, 11776, 11783, 11784, or 11789

## Test a Gerrit change on the guest

After `bootstrap` + `configure` once:

```
./on-haiku.sh try 11584
# reboot
screenmode 1920 1080 32
./on-haiku.sh test-log
./on-haiku.sh untry
# reboot to packaged modules
```

`try` fetches the latest patch set, `git apply`s it (no commit), jams the
touched add-ons, and overlays kernel driver + accelerant into
`~/config/non-packaged`. Kernel modules still need a reboot.

## Local apply / build / test

`work.sh` is meant to run **on the machine that has the Haiku git tree**
(the guest, or a Linux host that cross-builds). It does not talk to
Gerrit. `gerrit.sh` commits one issue inside that tree and pushes to
`refs/for/master`.

```
export HAIKU_SRC=$HOME/haiku
export HAIKU_OUTPUT=$HOME/haiku/generated   # after configure

./work.sh list
./work.sh check                            # all patches, apply --check
./work.sh all udp-unicast-enqueue          # check, apply, jam udp, run test
./work.sh apply arp-reject-learn
./work.sh reverse arp-reject-learn
./work.sh jam ipv4-multicast-filter
./work.sh test-build
./work.sh test udp-deliverdata
```

Jam targets and test binaries are in `MANIFEST`. `work.sh jam arp-*`
hits the userspace `arp` binary; kernel module is `jam -q '<module>arp'`.
Issues with no test binary are rebuild-only (error-path leaks). After
`jam` you still have to get the new add-on into the running image
(non-packaged overlay or reboot into a rebuilt image).

One-off without the script, from a Haiku source tree:

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
That is the #18816 fix; it is not in Gerrit 11789.

`ipv4-multicast-filter` uses setsockopt (`make && ./ipv4_multicast_filter`).
Unpatched: a second IP_UNBLOCK_SOURCE / IP_DROP_SOURCE_MEMBERSHIP returns 0.
Patched: the second call returns EADDRNOTAVAIL, and ADD_SOURCE after the
last DROP_SOURCE succeeds (group was left, not left dangling).

`ipv4-multicast-filtermode` is a constructor default. Confirm with a rebuild.

`ipv4-multicast-refs` and `ipv4-fragment-reassemble` are stack error
paths. Confirm with a rebuild. The membership test also exercises the
get_route / get_interface path that leaked references. The refs patch
also NULL-inits `multicast_address` so the destructor delete is safe.

Virtio, TCP, ICMP error-reply, and the other ARP changes are stack
error paths. They have no userspace flooder in this tree; confirm with
a rebuild and the failing path.
