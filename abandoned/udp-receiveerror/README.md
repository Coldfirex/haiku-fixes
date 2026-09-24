# udp-receiveerror (abandoned)

Do not Gerrit. Do not apply on current master.

Early `ReceiveError` / `DeliverError` returns were treated as a leak
because `icmp_receive_data` does not free after `error_received`.
That missed the next owner: `device_consumer_thread` (`loop consumer`)
calls `stack::free_buffer` when `receive_data` returns an error.

Evidence:

* Stock 60s ICMP dest-unreach / 3-byte inner UDP: pages 116299 → 132134.
* Overlay free+B_OK 60s: pages 106481 → 148683 (worse).
* First overlay (free then B_BAD_VALUE): PANIC General Protection
  Exception, `rbx = 0xdeadbeefdeadbeef`, `free_buffer` +
  `device_consumer_thread`.

`used_pages` under a raw ICMP flood is slab noise, not a UDP leak.

Lesson: if the hook returns an error, do not also `free` the buffer
unless a caller is shown *not* to free on that status. QUENCH already
`free` + `B_OK`; that path is separate.

The `.patch` and tester stay here as a record only.
