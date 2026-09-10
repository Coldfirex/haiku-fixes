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
```

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

Virtio, TCP, ICMP error-reply, and the other ARP changes are stack
error paths. They have no userspace flooder in this tree; confirm with
a rebuild and the failing path.
