# tcp-error-received

Status: **merged** [Gerrit 11847](https://review.haiku-os.org/c/haiku/+/11847) / `6c12327e`.
Do not re-push. Do not apply on current master.

- Change-Id: `I59d4f5a2d2cc247be9daa95817aee54e2bac7343`
- Topic: `tcp`
- Code-Review: +2 korli (Jérôme Duval)
- Merged: 2026-09-28

TCPEndpoint::ErrorReceived() does not queue the buffer. It returns
B_OK for B_NET_ERROR_MESSAGE_SIZE (PMTU) and B_ERROR otherwise.
Returning B_OK without free leaves that buffer unowned.

Free in ErrorReceived() before that B_OK, at the last successful
handler. tcp_error_received() must not assume ErrorReceived() is a
leaf. Do not free on B_ERROR; the caller still owns the buffer.

Tester `tcp_error_received_pmtu.c` is a record only.
