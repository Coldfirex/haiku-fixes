# Haiku networking notes

One folder per issue. The patch and its test (if any) live together.
These are notes and reproducers, not a Haiku Gerrit submission.
Do not send the generated patches to review.haiku-os.org as-is.

Patches follow the Haiku coding guidelines:
https://www.haiku-os.org/development/coding-guidelines/

Open and submitted (repo root):

```
udp-receiveerror/        ReceiveError / DeliverError early-return leak
virtio-net-mutex-uninit/ destroy rxLock/txLock if interrupt setup fails
virtio-net-interrupt-uninit/ free_interrupts if queue_setup_interrupt fails
tcp-spawn-abort/         listen-queue child leak when _Spawn fails
icmp-error-reply/        reply buffer leak if get_domain/prepend fails
udp-unicast-enqueue/     #18730 enqueue incoming unicast buffer (no clone)
udp-loopback-checksum/   #18730 skip TX checksum if route is IFF_LOOPBACK
arp-queued-send/         MarkValid NULL protocol KDL + send-fail leak
arp-reject-learn/        #18816 reject never cleared on learn (not 11789)
arp-protocol-teardown/   handler leak on init fail; UAF on uninit
ipv4-multicast-filter/   UnblockSource/DropSSM Remove; last SSM source LeaveGroup
ipv4-multicast-refs/     put_route/put_interface on membership wrappers
ipv4-fragment-reassemble/ 32-bit fragment end; restore buffers on merge fail
virtio-gpu-open-shared-area/ leftover GPU change; shared info area leak if open() fails
network-prefs-gateway-fallback/ #1 keep saved gateway in the IPv4 field
net-server-reapply-gateway/     #1 restore static default route on IFF_UP
virtio-block-uninit-dma/ DMAResource leak if init_device fails before cookie
virtio-block-blksize-zero/ host blk_size == 0 divide-by-zero in set_capacity
virtio-block-get-driver/ NULL parent / get_driver KDL in init_device
virtio-scsi-register-get-driver/     get_driver check in register_device
virtio-scsi-controller-get-driver/   get_parent_node / get_driver check in ctor
```

Merged upstream lives under [`merged/`](merged/README.md):

```
merged/virtio-gpu-detach-backing/   Gerrit 11771 / 0438319c
merged/virtio-gpu-mutex-uninit/     Gerrit 11776 / 768d4e6c
merged/virtio-gpu-clone-fd/         Gerrit 11783 / 55d56e03
merged/virtio-free-id/              Gerrit 11784 / bf90d383
merged/arp-request-buffer-dtor/     Gerrit 11789 / 6c10ad5b
merged/ipv4-multicast-filtermode/   Gerrit 11791 / 8d435047
merged/udp-deliverdata/             Gerrit 11792 / 1ca7d0a6
merged/virtio-tx-freelist/          Gerrit 11807 / d42d1ebd
merged/virtio-gpu-free-areas/       Gerrit 11824
merged/ipv4-multicast-if/          Gerrit 11827 / 04c75f4b
```

Gerrit tracking lives in each submitted folder as `STATUS`.
Do not send the GitHub `.patch` files to review.haiku-os.org as-is.

See also https://github.com/Coldfirex/haiku-fixes/issues/1 (Network prefs
gateway blank after ifconfig down; not ARP).

virtio_gpu notes that burned us on GPU patches 1–3. They also apply
to leftover virtio_net locals and `virtio-gpu-open-shared-area`.
Do not number those as GPU 4 unless you mean the GPU series only.
Steps:

- virtio_net id leak: `merged/virtio-free-id/README.md` (merged 11784)
- virtio_net TX slot leak: `merged/virtio-tx-freelist/README.md` (merged 11807 / d42d1ebd)
- virtio_net mutex teardown: `virtio-net-mutex-uninit/README.md`
- virtio_net interrupt teardown: `virtio-net-interrupt-uninit/README.md`
- GPU shared-area leak on open fail: `virtio-gpu-open-shared-area/README.md`
- GPU area leak on free: `merged/virtio-gpu-free-areas/README.md` (merged 11824)
- ARP request buffer: `merged/arp-request-buffer-dtor/README.md`
- IPv4 filter mode: `merged/ipv4-multicast-filtermode/README.md`
- IPv4 IP_MULTICAST_IF dtor: `merged/ipv4-multicast-if/README.md` (merged 11827 / 04c75f4b)
- UDP DeliverData enqueue free: `merged/udp-deliverdata/README.md` (merged 11792 / 1ca7d0a6)
- Network prefs saved gateway: `network-prefs-gateway-fallback/README.md`
- net_server restore gateway on up: `net-server-reapply-gateway/README.md`
- virtio_block DMAResource teardown: `virtio-block-uninit-dma/README.md`
- virtio_block zero blk_size: `virtio-block-blksize-zero/README.md`
- virtio_block get_driver: `virtio-block-get-driver/README.md`
- virtio_scsi register get_driver: `virtio-scsi-register-get-driver/README.md`
- virtio_scsi controller get_driver: `virtio-scsi-controller-get-driver/README.md`

Shared rules:

- Guest trees: `/boot/home/Desktop/sources/{haiku,haiku-fixes}`
- Always `export HAIKU_SRC` (helpers do not search Desktop)
- `chmod +x work.sh gerrit.sh on-haiku.sh`
- Do **not** use `on-haiku.sh go` for this series
- GPU kernel overlay (11771 / 11776 / open-shared-area):
  `~/config/non-packaged/add-ons/kernel/drivers/graphics/virtio_gpu`
- GPU accelerant overlay (11783 only):
  `~/config/non-packaged/add-ons/accelerants/virtio_gpu.accelerant`
- virtio_net overlay (`virtio-net-mutex-uninit` / `virtio-net-interrupt-uninit`):
  `~/config/non-packaged/add-ons/kernel/drivers/network/virtio_net`
- ARP kernel overlay: `~/config/non-packaged/add-ons/kernel/network/datalink_protocols/arp`
  (jam target is `'<module>arp'`, not userspace `arp`)
- IPv4 protocol overlay (remaining `ipv4-multicast-*` locals only):
  `~/config/non-packaged/add-ons/kernel/network/protocols/ipv4`
  (jam `ipv4`; binary under `generated/objects/haiku/x86_64/release/add-ons/kernel/network/protocols/ipv4/ipv4`)
- UDP protocol overlay (`udp-receiveerror` / `udp-unicast-enqueue` / `udp-loopback-checksum`):
  `~/config/non-packaged/add-ons/kernel/network/protocols/udp`
  (jam `udp`; binary under `generated/objects/haiku/x86_64/release/add-ons/kernel/network/protocols/udp/udp`)
- virtio_block overlay: `~/config/non-packaged/add-ons/kernel/drivers/disk/virtual/virtio_block`
- virtio_scsi overlay: `~/config/non-packaged/add-ons/kernel/busses/scsi/virtio`
- `gerrit.sh` may be missing from the guest clone; commit/push by hand
- Do not `open()` the GPU / run `virtio_gpu_clone` on a live desktop
- New commit + new Change-Id per issue; do not amend 11771, 11776, 11783, 11784, 11789, 11791, 11792, 11807, 11824, or 11827

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
Gerrit. `gerrit.sh` commits one issue inside that Haiku tree and pushes to
`refs/for/master`.

```
export HAIKU_SRC=/boot/home/Desktop/sources/haiku
export HAIKU_OUTPUT=$HAIKU_SRC/generated

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
cd merged/udp-deliverdata
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

`ipv4-multicast-filtermode` merged as Gerrit 11791. Constructor default
(`kInclude`). Rebuild-only. Drop the ipv4 overlay after merge.

`ipv4-multicast-if` merged as Gerrit 11827 / `04c75f4b`. Do not apply on
current master. Drop the ipv4 overlay.

`virtio-gpu-free-areas` merged as Gerrit 11824. Do not apply on current
master. Drop the virtio_gpu kernel overlay if it was only for 11824.

`ipv4-multicast-refs` puts the route/interface taken by the membership
wrappers. Still local. Apply separately from the merged if/filtermode patches.

Virtio, TCP, ICMP error-reply, and the other ARP changes are stack
error paths. They have no userspace flooder in this tree; confirm with
a rebuild and the failing path.
