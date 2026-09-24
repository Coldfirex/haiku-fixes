# udp-receiveerror

Free the ICMP-quoted buffer on UDP `ReceiveError` / `DeliverError`
early returns. `error_received()` takes ownership; QUENCH/default and
unicast `DeliverError()` already free.

Gerrit commit text (copy this; do not attach the tester):

```
udp: free error buffer on early returns

icmp_receive_data hands the packet to the protocol error_received
hook and does not free it. UDP leaked it on:

* DeliverError for broadcast/multicast
* ReceiveError if the buffer was short, had no domain support, or
  the UDP header read failed

DeliverError() already frees unicast buffers. udp_error_received()
already frees QUENCH and unknown errors.
```

In-code comment is `error_received() takes ownership of the buffer.`
Not “transfers ownership” — the caller hands it in.

Do not fold ipv4/ipv6/tcp error_received into this change.
Do not put `udp_receiveerror_leak.c` on the Gerrit change.

Topic: `udp`. Overlay:
`~/config/non-packaged/add-ons/kernel/network/protocols/udp`
Jam target: `udp`.
