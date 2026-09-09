# Haiku networking notes

One folder per issue. The patch and its test (if any) live together.
These are notes and reproducers, not a Haiku Gerrit submission.
Do not send the generated patches to review.haiku-os.org as-is.

```
udp-deliverdata/     DeliverData clone leak when FIFO is full
udp-receiveerror/    ReceiveError / DeliverError early-return leak
virtio-tx-freelist/  TX BufInfo leak + mutex teardown on init fail
virtio-free-id/      device id leak on publish_device fail
tcp-spawn-abort/     listen-queue child leak when _Spawn fails
icmp-error-reply/    reply buffer leak if get_domain/prepend fails
```

## Apply a patch

From a Haiku source tree:

```
git apply /path/to/<folder>/<name>.patch
```

## Run a userspace test (Haiku)

```
cd udp-deliverdata
gcc -O2 -o udp_deliverdata_leak udp_deliverdata_leak.c -lnetwork
./udp_deliverdata_leak 60
```

Unpatched: `used pages` climbs after the socket FIFO fills.
Patched: pages flatten.

`udp-receiveerror` needs raw ICMP (`SOCK_RAW`). Default target is `127.0.0.1`.

Virtio, TCP, and ICMP error-reply changes are stack error paths. They have
no userspace flooder in this tree; confirm with a rebuild and the failing
path.
