# tcp-error-received

Gerrit [11847](https://review.haiku-os.org/c/haiku/+/11847).

Free the ICMP-quoted buffer in `TCPEndpoint::ErrorReceived()` just
before `return B_OK` (PMTU / `B_NET_ERROR_MESSAGE_SIZE`). That path
does not queue the buffer.

Do not free in `tcp_error_received()` after `ErrorReceived()` — that
assumes the method is a leaf. Do not free on `B_ERROR`.

Gerrit body (no tester):

```
tcp: free error buffer when ErrorReceived returns B_OK

TCPEndpoint::ErrorReceived() does not queue the buffer. It returns
B_OK for B_NET_ERROR_MESSAGE_SIZE (PMTU) and B_ERROR otherwise.
Returning B_OK without free leaves that buffer unowned.

Free in ErrorReceived() before that B_OK, at the last successful
handler. tcp_error_received() must not assume ErrorReceived() is a
leaf. Do not free on B_ERROR; the caller still owns the buffer.
```

Topic: `tcp`. Same Change-Id as 11847. Amend PS2; do not open a new change.
Overlay: `~/config/non-packaged/add-ons/kernel/network/protocols/tcp`
Jam target: `tcp`.
