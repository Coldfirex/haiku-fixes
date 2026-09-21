# ipv4: free IP_MULTICAST_IF address in protocol destructor

Status: **submitted**.

- Gerrit: https://review.haiku-os.org/c/haiku/+/11827
- Change-Id: `I646a3c8c43d42a4c6669dbf7e3871492c97770ec`
- Topic: `ipv4-multicast`
- Patch set: 1
- Submitted: 2026-09-21
- Code-Review: +2 waddlesplash

Do not amend 11827. Do not reuse Change-Id 11791.

`IP_MULTICAST_IF` stores a heap sockaddr on `ipv4_protocol`.
`setsockopt` already deletes the previous pointer when the option is
replaced or cleared. The destructor only deleted `raw`, so close leaked
the address. `ipv4_init_protocol()` already sets the pointer to NULL.

Smoke: `gcc -o /tmp/ipv4_multicast_if /tmp/ipv4_multicast_if.c -lnetwork`
then set/get `127.0.0.1` and close. Link with `-lnetwork` on Haiku.

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
