# ipv4: initialize MulticastGroupInterface filter mode to include

Status: **merged**.

- Gerrit: https://review.haiku-os.org/c/haiku/+/11791
- Change-Id: `I46c4bf647e930401239999a4c787b2c52e443a61`
- Topic: `ipv4-multicast`
- haiku.git: `8d43504781e3f254c170420ba18c8ca80dbb8c16`
- Merged: 2026-09-18
- Reviewed-by: Augustin Cavalier (waddlesplash)

Do not re-push 11791.

The constructor never set `fFilterMode`. `IsEmpty()` is true only for
include mode with no sources, so an uninitialized mode looks like a
live group. `Clear()` then `LeaveGroup()`s on delete, including the
`GetState` path where `JoinGroup()` just failed. Default is `kInclude`.

Rebuild-only. No userspace test.

This is not the UnblockSource/DropSSM `Add`/`Remove` bug. That is
`ipv4-multicast-filter/` and is still local.

## Jam / overlay (historical)

```
export HAIKU_SRC=/boot/home/Desktop/sources/haiku
export HAIKU_OUTPUT=$HAIKU_SRC/generated
cd $HAIKU_OUTPUT
jam -q ipv4
```

Built binary:

`$HAIKU_OUTPUT/objects/haiku/x86_64/release/add-ons/kernel/network/protocols/ipv4/ipv4`

Overlay that loaded:

`~/config/non-packaged/add-ons/kernel/network/protocols/ipv4`

After merge, drop the overlay and `shutdown -r` to run packaged.
