# ipv4: UnblockSource/DropSSM must remove the source address

Status: **submitted**.

- Gerrit: https://review.haiku-os.org/c/haiku/+/11850
- Change-Id: `I6adf8cfe6d03e5dcfea0ce8cce7d1a01109d6500`
- Topic: `ipv4-multicast`
- Patch set: 1
- Submitted: 2026-09-24

Do not amend 11850 unless a reviewer asks. Do not reuse Change-Id 11791 or 11827.

`UnblockSource()` / `DropSSM()` copied `Add()` from the block/add paths.
`AddressSet::Add()` is a no-op when the source is already present, so
`IP_UNBLOCK_SOURCE` and `IP_DROP_SOURCE_MEMBERSHIP` never change the
filter. Remove the source. After the last include-mode SSM source,
`LeaveGroup()` so the interface does not stay joined when
`ReturnState()` deletes the `GroupInterface`.

Tester: `ipv4_multicast_filter.c` (loopback is enough).

Unpatched: second UNBLOCK or DROP_SOURCE succeeds.
Patched:

```
unblock removed the source
drop-source removed the source
```

## Jam / overlay

```
export HAIKU_SRC=/boot/home/Desktop/sources/haiku
export HAIKU_OUTPUT=$HAIKU_SRC/generated
cd $HAIKU_OUTPUT
jam -q ipv4
```

Binary:

`$HAIKU_OUTPUT/objects/haiku/x86_64/release/add-ons/kernel/network/protocols/ipv4/ipv4`

Overlay:

`~/config/non-packaged/add-ons/kernel/network/protocols/ipv4`

After merge, drop the overlay and `shutdown -r`.
