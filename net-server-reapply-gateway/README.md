# net_server: restore static default route on IFF_UP

Status: **local**. Coldfirex/haiku-fixes#1. Related: Haiku #9695 / #16349.

Pair with `network-prefs-gateway-fallback/` so the Network panel does
not show or persist a blank Gateway field.

`ifconfig down` → kernel `WentDown()` → `invalidate_routes()` drops
`RTF_DEFAULT`. `ifconfig up` only rebuilds host + subnet routes.
`_ConfigureInterface()` is what adds `AddDefaultRoute()` from the
saved profile, and it does not run on IFF_UP.

Watch `B_NETWORK_INTERFACE_CHANGED`. On a down-to-up transition, if
the profile is static and still has a gateway, add that route again.
DHCP / `auto_config` is left alone.

Jam target: `net_server`. Userspace server. Restart net_server (or
reboot) after install. No kernel overlay.

```
cd net-server-reapply-gateway
make
./net_server_reapply_gateway
# or
./net_server_reapply_gateway /dev/net/virtio/0 192.168.250.254
```

Unpatched: exit 2, `route` has no default.
Patched: default via the saved gateway is back.

The test brings the NIC down and up.
