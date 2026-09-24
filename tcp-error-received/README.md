# tcp-error-received

Free the ICMP-quoted buffer only when `TCPEndpoint::ErrorReceived()`
returns `B_OK` (`B_NET_ERROR_MESSAGE_SIZE` / PMTU). That path does
not queue the buffer and used to return `B_OK` without `free`.

Do **not** `free` on early `B_ERROR` / `B_BAD_DATA` returns.
`device_consumer_thread` already frees those (`udp-receiveerror` KDL).

`tcp_error_received_pmtu` is a path/KDL check: loopback TCP + ICMP
frag-needed at ~50/s. It is not a conclusive leak meter. Stock does
not KDL on this path. Do not put the tester on Gerrit.

Gerrit body (no tester):

```
tcp: free error buffer when ErrorReceived returns B_OK

TCPEndpoint::ErrorReceived() does not queue the buffer. It returns
B_OK for B_NET_ERROR_MESSAGE_SIZE (PMTU) and B_ERROR otherwise.
Returning B_OK without free leaves that buffer unowned.

For other return values, device_consumer_thread retains ownership
and frees the buffer when receive_data() returns an error.

Free only on the B_OK path. Do not free and then return an error.
```

Topic: `tcp`. Overlay:
`~/config/non-packaged/add-ons/kernel/network/protocols/tcp`
Jam target: `tcp`.
Do not stack with abandoned ipv4/udp error_received patches.
