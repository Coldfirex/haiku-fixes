# ipv4: free IP_MULTICAST_IF address in protocol destructor

Status: **merged**.

- Gerrit: https://review.haiku-os.org/c/haiku/+/11827
- Change-Id: `I646a3c8c43d42a4c6669dbf7e3871492c97770ec`
- Topic: `ipv4-multicast`
- haiku.git: `04c75f4b79d7f663840ed365c632222bba3bab86`
- Merged: 2026-09-22
- Reviewed-by: Augustin Cavalier (waddlesplash)
- Tested-by: Commit checker robot

Do not re-push 11827. Do not apply this patch on current master.

`IP_MULTICAST_IF` stored a heap sockaddr on `ipv4_protocol`.
The destructor only deleted `raw`. `ipv4_init_protocol()` already
NULLs the pointer.

After merge, drop any leftover overlay:

```
rm -f ~/config/non-packaged/add-ons/kernel/network/protocols/ipv4
shutdown -r
```
