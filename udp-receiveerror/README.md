# udp-receiveerror

Free the ICMP-quoted buffer on UDP `ReceiveError` / `DeliverError`
early returns.

`error_received()` takes ownership. QUENCH/default already
`free` + `B_OK`. Unicast `DeliverError()` already `free` + `B_OK`.
Early paths must do the same. Returning `B_BAD_VALUE` / `B_ERROR`
after `free` lets `device_consumer_thread` (`loop consumer`) free
again. That KDL is a General Protection Exception in
`stack::free_buffer` with `rbx = 0xdeadbeefdeadbeef`.

Gerrit commit text (copy this; do not attach the tester):

```
udp: free error buffer on early returns

icmp_receive_data hands the packet to the protocol error_received
hook and does not free it. UDP leaked it on:

* DeliverError for broadcast/multicast
* ReceiveError if the buffer was short, had no domain support, or
  the UDP header read failed

DeliverError() already frees unicast buffers and returns B_OK.
udp_error_received() already frees QUENCH and unknown errors and
returns B_OK. Early paths must do the same: free, then B_OK.
Returning an error after free lets a caller free again.
```

In-code comment is `error_received() takes ownership of the buffer.`

Do not fold ipv4/ipv6/tcp error_received into this change.
Do not put `udp_receiveerror_leak.c` on the Gerrit change.
Do not Gerrit until the free+B_OK overlay is retested.

Topic: `udp`. Overlay:
`~/config/non-packaged/add-ons/kernel/network/protocols/udp`
Jam target: `udp`.
