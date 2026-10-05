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
mediaplayer-stop-uninit/ #18714 Stop/Pause before node Init
tcp-spawn-abort/         listen-queue child leak when _Spawn fails
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
net-server-interface-network/   read saved network name from the network block
syslog-remote-forward/   optional RFC 3164 UDP client in syslog_daemon
wireless-autojoin-retry/ #18238 retry auto-join if the first scan misses
```

Do-not-submit locals live under [`hold/`](hold/README.md):

```
hold/virtio-block-get-driver/
hold/virtio-scsi-register-get-driver/
hold/virtio-scsi-controller-get-driver/
hold/virtio-net-interrupt-uninit/
hold/virtio-rx-freelist/
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
merged/tcp-error-received/          Gerrit 11847 / 6c12327e
merged/virtio-gpu-open-shared-area/ Gerrit 11851 / 83d859ac
merged/virtio-net-mutex-uninit/     Gerrit 11858 / 1832af68
merged/icmp-error-reply/            Gerrit 11869 / a4aa8679
```

Gerrit tracking lives in each submitted folder as `STATUS`.
Do not send the GitHub `.patch` files to review.haiku-os.org as-is.
Do not submit `hold/` or re-push `abandoned/` / `merged/`.

See also https://github.com/Coldfirex/haiku-fixes/issues/1 (Network prefs
gateway blank after ifconfig down; not ARP).

Shared rules:

- Guest trees: `/boot/home/Desktop/sources/{haiku,haiku-fixes}`
- Always `export HAIKU_SRC` (helpers do not search Desktop)
- `chmod +x work.sh gerrit.sh on-haiku.sh`
- Do **not** use `on-haiku.sh go` for this series
- Full order (packaged baseline first): `GUEST-WORKFLOW.md`
- New commit + new Change-Id per issue; do not amend merged Change-Ids
- Do not re-push abandoned/, merged/, or hold/
