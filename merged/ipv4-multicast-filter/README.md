# ipv4: UnblockSource/DropSSM must remove the source address

Status: **merged**.

- Gerrit: https://review.haiku-os.org/c/haiku/+/11850
- Change-Id: `I6adf8cfe6d03e5dcfea0ce8cce7d1a01109d6500`
- Topic: `ipv4-multicast`
- Patch set: 6
- haiku.git: `e6ecc742c177359506a572769de7ea62b341a00c`
- Merged: 2026-10-05, submitted by korli (Code-Review +2)

Do not re-push. Do not apply on current master.

`UnblockSource()` and `DropSSM()` called `Add()` after the source was already in the set, so the source never left the filter. Remove it. After the last include-mode source, leave the group. Same copy in `ipv6/multicast.cpp`. IPv6 setsockopt still does not call those functions.

The setsockopt lock is 11883, not this change.
