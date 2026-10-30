# virtio_net: use user_memcpy for multicast add/remove

Status: **local**.

ETHER_ADDMULTI and ETHER_REMMULTI copy or compare the user buffer with
plain memcpy/memcmp. Other ioctl cases in the same function already use
user_memcpy. This can fault under SMAP.

Copy the address into a kernel buffer first.

## Overlay

`~/config/non-packaged/add-ons/kernel/drivers/network/virtio_net`

Not Gerrit yet.
