# tcp-error-received

Free the ICMP-quoted buffer only when `TCPEndpoint::ErrorReceived()`
returns `B_OK` (`B_NET_ERROR_MESSAGE_SIZE` / PMTU). That path does
not queue the buffer and used to return `B_OK` without `free`.

Do **not** `free` on early `B_ERROR` / `B_BAD_DATA` returns.
`device_consumer_thread` already frees those (`udp-receiveerror` KDL).

Gerrit body (no tester):

```
tcp: free error buffer when ErrorReceived returns B_OK

device_consumer_thread already frees the buffer when receive_data
returns an error. tcp_error_received early returns are not leaks.

TCPEndpoint::ErrorReceived() does not queue the buffer. It returns
B_OK for B_NET_ERROR_MESSAGE_SIZE (PMTU) and B_ERROR otherwise.
Returning B_OK without free leaves that buffer unowned.

Free only on that B_OK path. Do not free and then return an error.
```

Topic: `tcp`. Overlay:
`~/config/non-packaged/add-ons/kernel/network/protocols/tcp`
Jam target: `tcp`.
Do not stack with ipv4/udp error_received patches.
