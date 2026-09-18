# Network prefs: keep saved gateway when live route is gone

Status: **local**. Coldfirex/haiku-fixes#1. Related: Haiku #9695 / #16349.

Does not restore the live default route. Pair with
`net-server-reapply-gateway/` for that.

`InterfaceAddressView::_UpdateFields()` fills Gateway from
`GetDefaultGateway()` (live `RTF_DEFAULT`). `ifconfig down` deletes that
route. Address and mask come back on `up` because they live on the
interface object; the gateway does not. The box goes blank. Apply then
writes `interfaces` without a `gateway` line.

This patch:

- Shows the saved static gateway when the live lookup fails.
- On Static Apply, keeps the previous saved gateway if the field is empty.

Jam target: `Network` (`src/preferences/network`). Userspace. No kernel
overlay. Restart Network preferences after install.

## Manual check

1. Static IPv4, `gateway` present in `/boot/system/settings/network/interfaces`.
2. `ifconfig /dev/net/virtio/0 down && sleep 1 && ifconfig /dev/net/virtio/0 up`
3. Open Network → IPv4. Gateway must still show the saved address.
4. Apply. The `gateway` line must still be in `interfaces`.

`ping 1.1.1.1` still fails until the net_server patch is also running.
