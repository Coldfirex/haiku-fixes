# Haiku networking notes

One folder per issue. The patch and its test (if any) live together.
These are notes and reproducers, not a Haiku Gerrit submission.
Do not send the generated patches to review.haiku-os.org as-is.

**How to pull, baseline on packaged modules, apply, overlay, retest,
and upload to Gerrit:** [`GUEST-WORKFLOW.md`](GUEST-WORKFLOW.md).
Stock numbers are taken *before* any file is copied into
`~/config/non-packaged`. Do not use an issue folder as the howto.

Patches follow the Haiku coding guidelines:
https://www.haiku-os.org/development/coding-guidelines/

Open and submitted (repo root):

```
tcp-error-received/      free quoted buffer only if ErrorReceived returns B_OK (PMTU)
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
network-prefs-gateway-fallback/ #1 keep saved gateway in the IPv4 field
net-server-reapply-gateway/     #1 restore static default route on IFF_UP
syslog-remote-forward/   optional RFC 3164 UDP client in syslog_daemon
```

Do-not-submit locals live under [`hold/`](hold/README.md):

```
hold/virtio-block-get-driver/
hold/virtio-scsi-register-get-driver/
hold/virtio-scsi-controller-get-driver/
```

Merged upstream lives under [`merged/`](merged/README.md).
Abandoned changes live under [`abandoned/`](abandoned/README.md):

```
abandoned/virtio-block-blksize-zero/  Gerrit 11833 (spec has no blk_size==0 fallback)
abandoned/udp-receiveerror/           never submitted; consumer already frees on error
abandoned/ipv4-error-received/        never submitted; consumer already frees on error
abandoned/ipv6-error-received/        never submitted; consumer already frees on error
```

Merged:

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
merged/virtio-block-uninit-dma/     Gerrit 11843 / 40f89b3f
merged/virtio-gpu-open-shared-area/ Gerrit 11851 / 83d859ac
```

Gerrit tracking lives in each submitted folder as `STATUS`.
Do not send the GitHub `.patch` files to review.haiku-os.org as-is.
Do not submit `hold/` or re-push `abandoned/` / `merged/`.

See also https://github.com/Coldfirex/haiku-fixes/issues/1 (Network prefs
gateway blank after ifconfig down; not ARP).

virtio_gpu notes that burned us on GPU patches 1-3. They also apply
to leftover virtio_net locals.
Do not number those as GPU 4 unless you mean the GPU series only.
Steps:

- virtio_net id leak: `merged/virtio-free-id/README.md` (merged 11784)
- virtio_net TX slot leak: `merged/virtio-tx-freelist/README.md` (merged 11807 / d42d1ebd)
- virtio_net mutex teardown: `virtio-net-mutex-uninit/README.md`
- virtio_net interrupt teardown: `virtio-net-interrupt-uninit/README.md`
- GPU shared-area leak on open fail: `merged/virtio-gpu-open-shared-area/README.md` (merged 11851 / 83d859ac)
- GPU area leak on free: `merged/virtio-gpu-free-areas/README.md` (merged 11824)
- ARP request buffer: `merged/arp-request-buffer-dtor/README.md`
- IPv4 filter mode: `merged/ipv4-multicast-filtermode/README.md`
- IPv4 IP_MULTICAST_IF dtor: `merged/ipv4-multicast-if/README.md` (merged 11827 / 04c75f4b)
- UDP DeliverData enqueue free: `merged/udp-deliverdata/README.md` (merged 11792 / 1ca7d0a6)
- UDP ReceiveError early return (abandoned): `abandoned/udp-receiveerror/README.md`
- TCP ErrorReceived PMTU free: `tcp-error-received/README.md`
- Network prefs saved gateway: `network-prefs-gateway-fallback/README.md`
- net_server restore gateway on up: `net-server-reapply-gateway/README.md`
- virtio_block DMAResource/IOScheduler teardown: `merged/virtio-block-uninit-dma/README.md` (merged 11843 / 40f89b3f)
- virtio get_driver checks (hold): `hold/README.md`
- virtio_block zero blk_size (abandoned 11833): `abandoned/virtio-block-blksize-zero/README.md`
- syslog UDP forwarder (local): `syslog-remote-forward/README.md`

Shared rules:

- Guest trees: `/boot/home/Desktop/sources/{haiku,haiku-fixes}`
- Always `export HAIKU_SRC` (helpers do not search Desktop)
- `chmod +x work.sh gerrit.sh on-haiku.sh`
- Do **not** use `on-haiku.sh go` for this series
- Full order (packaged baseline first): `GUEST-WORKFLOW.md`
- GPU kernel overlay (historical 11771 / 11776 / 11824 / 11851):
  `~/config/non-packaged/add-ons/kernel/drivers/graphics/virtio_gpu`
- GPU accelerant overlay (11783 only):
  `~/config/non-packaged/add-ons/accelerants/virtio_gpu.accelerant`
- virtio_net overlay (`virtio-net-mutex-uninit` / `virtio-net-interrupt-uninit`):
  `~/config/non-packaged/add-ons/kernel/drivers/network/virtio_net`
- ARP kernel overlay: `~/config/non-packaged/add-ons/kernel/network/datalink_protocols/arp`
  (jam target is `'<module>arp'`, not userspace `arp`)
- IPv4 protocol overlay (remaining `ipv4-multicast-*` locals only):
  `~/config/non-packaged/add-ons/kernel/network/protocols/ipv4`
- UDP protocol overlay (`udp-unicast-enqueue` / `udp-loopback-checksum`):
  `~/config/non-packaged/add-ons/kernel/network/protocols/udp`
- TCP protocol overlay (`tcp-error-received` / `tcp-spawn-abort`):
  `~/config/non-packaged/add-ons/kernel/network/protocols/tcp`
- virtio_block overlay (historical 11843):
  `~/config/non-packaged/add-ons/kernel/drivers/disk/virtual/virtio_block`
- virtio_scsi overlay: `~/config/non-packaged/add-ons/kernel/busses/scsi/virtio`
- syslog_daemon overlay (`syslog-remote-forward`):
  `~/config/non-packaged/servers/syslog_daemon` then restart
  `x-vnd.Haiku-SystemLogger`
- New commit + new Change-Id per issue; do not amend 11771, 11776, 11783, 11784, 11789, 11791, 11792, 11807, 11824, 11827, 11843, or 11851
- Do not re-push abandoned/ (11833, udp-receiveerror, ipv4-error-received, ipv6-error-received), merged/, or hold/
