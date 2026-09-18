# arp: free request_buffer when the cache entry is destroyed

Status: **submitted**.

- Gerrit: https://review.haiku-os.org/c/haiku/+/11789
- Change-Id: `I14706b727db2fe8be38681096793dbf067d703d0`
- Topic: `arp`
- Submitted: 2026-09-18

`arp_entry` holds a template ARP request in `request_buffer` while
resolution is in progress. Some failure and removal paths destroy the
entry without `delete_request_buffer()`, so that buffer leaks. The
destructor already waits for the timer; free the template there.

Rebuild-only. No userspace flooder.

This is **not** https://dev.haiku-os.org/ticket/18816. That ticket is
the reject-never-cleared-on-learn bug in `arp-reject-learn/`.

## Jam / overlay (guest)

`jam -q arp` builds the userspace `arp` command. The kernel module is:

```
export HAIKU_SRC=/boot/home/Desktop/sources/haiku
cd $HAIKU_SRC/generated
jam -q '<module>arp'
```

Overlay that actually loads:

`~/config/non-packaged/add-ons/kernel/network/datalink_protocols/arp`

Do not use `on-haiku.sh go` (copies into `drivers/bin`). Reboot after
the overlay. `listimage | grep arp` must show the non-packaged path.

Smoke: ping works with the overlay loaded. `ifconfig down` drops the
live default route on a static interface; that is unrelated (see
https://github.com/Coldfirex/haiku-fixes/issues/1).
