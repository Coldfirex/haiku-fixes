# arp: free request_buffer when the cache entry is destroyed

Status: **merged**.

- Gerrit: https://review.haiku-os.org/c/haiku/+/11789
- Change-Id: `I14706b727db2fe8be38681096793dbf067d703d0`
- Topic: `arp`
- haiku.git: `6c10ad5bd40b85d1a1fa07b2cfcabd43a9eb5033`
- Merged: 2026-09-18
- Reviewed-by: Augustin Cavalier (waddlesplash)

Do not re-push 11789.

`arp_entry` holds a template ARP request in `request_buffer` while
resolution is in progress. Some failure and removal paths destroy the
entry without `delete_request_buffer()`, so that buffer leaks. The
destructor already waits for the timer; free the template there.

Rebuild-only. No userspace flooder.

This is **not** https://dev.haiku-os.org/ticket/18816. That ticket is
the reject-never-cleared-on-learn bug in `arp-reject-learn/`.

## Jam / overlay (historical)

`jam -q arp` builds the userspace `arp` command. The kernel module is:

```
export HAIKU_SRC=/boot/home/Desktop/sources/haiku
cd $HAIKU_SRC/generated
jam -q '<module>arp'
```

Overlay that loaded:

`~/config/non-packaged/add-ons/kernel/network/datalink_protocols/arp`

After merge, drop the overlay and `shutdown -r` to run packaged.
`ifconfig down` drops the live default route on a static interface;
that is unrelated (see https://github.com/Coldfirex/haiku-fixes/issues/1).
